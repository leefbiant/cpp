#include "SvrFrame.h"
#include "Log.h"

#ifndef SO_ORIGINAL_DST
#define SO_ORIGINAL_DST  80
#endif  
CSvrFrame::CSvrFrame():
m_oUserMgr(this),
m_oModuleMgr(this)
{
	m_iEpollFd		= 0;
	m_stEpollEvs	= NULL;

	m_iListenFd		= 0;
	m_iCpuNum		= sysconf(_SC_NPROCESSORS_CONF);
}

CSvrFrame::~CSvrFrame()
{
	if (m_iListenFd > 0)
	{
		close(m_iListenFd);
		m_iListenFd = -1;
	}
	

	if (NULL != m_stEpollEvs)
	{
		delete []m_stEpollEvs;
		m_stEpollEvs = NULL;
	}
}

bool CSvrFrame::Init()
{
	m_iEpollFd = epoll_create(MAX_SOCKET_NUM);
	if (m_iEpollFd <= 0)
	{
		Logger.Log(ERROR, "epoll_create failed:%s", strerror(errno));
		printf("epoll_create failed:%s\n", strerror(errno));
		return false;
	}

	m_stEpollEvs = new struct epoll_event[MAX_SOCKET_NUM];
	if (NULL == m_stEpollEvs)
	{
		Logger.Log(ERROR, "new struct epoll_event failed");
		printf("new struct epoll_event failed\n");
		return false;
	}

	if (!m_oReadThreadPool.Init(m_iCpuNum, IO_READ))
	{
		Logger.Log(ERROR, "m_oReadThreadPool.Init");
		printf("m_oReadThreadPool.Init\n");
		return false;
	}

	if (!m_oWriteThreadPool.Init(m_iCpuNum, IO_WRITE))
	{
		Logger.Log(ERROR, "m_oWriteThreadPool.Init");
		printf("m_oWriteThreadPool.Init\n");
		return false;
	}

	if (!StartListen(m_iListenFd))
	{
		Logger.Log(ERROR, "StartListen failed");
		printf("StartListen failed\n");
		return false;
	}
	
	pthread_t threadid;
	if(pthread_create(&threadid, NULL, IoEvent, (void*)this) != 0)
	{
		Logger.Log(ERROR, "IoEvent, pthread_create failed");
		printf("IoEvent, pthread_create failed\n");
		return false;
	}

	if( 0 != m_oUserMgr.Init())
	{
		return false;
	}

	CTimer::GetInstance()->SetTimer(this, TIMER_CHECK_TIMEOUT, MAX_CHECK_TIMEOUT);

	return true;
}

bool CSvrFrame::StartListen(int32& m_iListenFd)
{
	static uint16 index = 0;
	struct sockaddr_in stTcpSvrAddr;
	memset(&stTcpSvrAddr, 0, sizeof(struct sockaddr_in));
	stTcpSvrAddr.sin_family      = AF_INET;
	stTcpSvrAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	stTcpSvrAddr.sin_port        = htons(g_oConfig.m_stArgs.wHostPort + index++);

	if((m_iListenFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{	
		Logger.Log(ERROR, "create listen socket failed!");

		return false;
	}

	if (!SetNonblock(m_iListenFd, true))
	{
		close(m_iListenFd);
		return false;
	}

	if (!SetReuseAddr(m_iListenFd, 1))
	{
		close(m_iListenFd);
		return false;
	}

	if (!SetLinger(m_iListenFd, 0))
	{
		close(m_iListenFd);
		return false;
	}

	CListener* pListener  = new CListener();
	pListener->m_eIoType  = IO_TCP;
	pListener->m_iSockFd  = m_iListenFd;
	if (!AddFd(m_iListenFd, EPOLLIN |EPOLLET| EPOLLERR, pListener))
	{
		close(m_iListenFd);
		return false;
	}

	if(bind(m_iListenFd, (struct sockaddr*)&stTcpSvrAddr, (socklen_t)sizeof(struct sockaddr_in)) < 0) 
	{
		Logger.Log(ERROR,"listen socket bind failed!");
		close(m_iListenFd);

		return false;
	}

	if(listen(m_iListenFd, 10) < 0) 
	{
		Logger.Log(ERROR,"listen socket listen failed!");
		close(m_iListenFd);

		return false;
	}

	return true;
}

bool CSvrFrame::ConnectTo(CUser* pUser, struct sockaddr_in& stTcpSvrAddr)
{

	int iConnFd = 0;
	if((iConnFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{	
		Logger.Log(ERROR, "create Connect socket failed!");
		return false;
	}
	if (!SetNonblock(iConnFd, true))
	{
		Logger.Log(NOTIC, "SetNonblock failed!!!");
		close(iConnFd);
		return false;
	}

	if (-1 == connect(iConnFd, (struct sockaddr *)&stTcpSvrAddr, sizeof(struct sockaddr)))
	{
		if (errno != EINPROGRESS) 
		{
			Logger.Log(NOTIC, "connect data server failed!!!");
			close(iConnFd);
			return false;
		}
	}
	else
	{
		//has connected
		pUser->m_bconned  = true;
		Logger.Log(INFO, "connecter server ip =%s port=%d sucess",  intip2char(stTcpSvrAddr.sin_addr.s_addr), ntohs(stTcpSvrAddr.sin_port));
	}

	if (!SetReuseAddr(iConnFd, 1))
	{
		Logger.Log(NOTIC, "SetReuseAddr failed!!!");
		close(iConnFd);
		return false;
	}

	pUser->m_dwIp  = stTcpSvrAddr.sin_addr.s_addr;
	pUser->m_wPort = stTcpSvrAddr.sin_port;
	pUser->SetIoState(IO_BUSY, true);
	pUser->m_eIoType     = IO_TCP;
	pUser->m_iSockFd     = iConnFd;
	pUser->m_fCallback   = IoHandle;
	if (!AddFd(iConnFd, EPOLLIN| EPOLLOUT| EPOLLET| EPOLLERR, pUser))
	{
		Logger.Log(ERROR, "Add fd[%d] epoll failed process exit....",iConnFd);
		sleep(1);
		exit(0);
	}
	return true;
}

int CSvrFrame::AcceptNew(int32 m_iListenFd)
{
	int iConnFd = 0;
	struct sockaddr_in stClientAddr;
	socklen_t  iCliLen = sizeof(struct sockaddr_in);

	while(true)
	{
		iConnFd = accept(m_iListenFd, (sockaddr *)&stClientAddr,  &iCliLen);
		if(iConnFd < 0)
		{
			if (EAGAIN == errno)
			{
				break;
			}

			Logger.Log(ERROR, "accept error:%s", strerror(errno));
			return ERR_FAILED;
		}

		else
		{
			
			if(!SetNonblock(iConnFd, true))
			{
				close(iConnFd);
				continue;
			}

			
			static uint32 dwSessionId = 1;

			CMsgBase* pMsgModConn = (CMsgModConn*)CMsgBase::NewMsg(CMD_USER_LOGIN);
			pMsgModConn->m_llSrcUid   = ++dwSessionId;
			
			m_oModuleMgr.SetMsg(pMsgModConn);

/*			
			CModule*  pModule = m_oModuleMgr.GetIdleModule();
			pModule->m_dwIp  = stClientAddr.sin_addr.s_addr;
			pModule->m_wPort = stClientAddr.sin_port;
			
			pModule->SetIoState(IO_BUSY, true);
			pModule->m_eIoType     = IO_TCP;
			pModule->m_iSockFd     = iConnFd;
			pModule->m_fCallback   = IoHandle;
			if (!AddFd(iConnFd, EPOLLIN| EPOLLOUT| EPOLLET| EPOLLERR, pModule))
			{
				Logger.Log(ERROR, "Add fd[%d] epoll failed process exit....",iConnFd);
				sleep(1);
				exit(0);
			}
			
			m_oModuleMgr.AddModule(pModule);

*/
			Logger.Log(INFO, "have new connect session[%u] fd = %d dst[%s:%d] ", 
				dwSessionId, iConnFd, intip2char(stClientAddr.sin_addr.s_addr), ntohs(stClientAddr.sin_port));
		}
	}

	return ERR_SUCCESS;
}
void CSvrFrame::OnTimer(const uint32 dwID)
{
	switch(dwID)
	{
	case TIMER_CHECK_TIMEOUT:
		{
			m_oUserMgr.SetMsg(NULL);
		}
		break;
	default:
		break;
	}
}

void CSvrFrame::SigNotify(int signo)
{
	if(SIGUSR1 == signo)
	{
	}
	else if(SIGUSR2 == signo)
	{
		Logger.Log(INFO, "=== task info, read:[%d, %d], write:[%d, %d] ===", \
			m_oReadThreadPool.GetTaskCount(), m_oReadThreadPool.GetThreadCount(), \
			m_oWriteThreadPool.GetTaskCount(), m_oWriteThreadPool.GetThreadCount());
	}
	else if(SIGRTMIN == signo)
	{
	}
	else if (SIGRTMIN + 1 == signo)
	{
		static bool bSet = false;
		Logger.SetLogLevel(DEBUG_LEVEL, bSet);
		bSet = !bSet;
	}
}

int CSvrFrame::Run()
{
	return m_oModuleMgr.DispatchMsg();
}

void CSvrFrame::SubmitTask(CIoHandler* pIoHandler, uint16 wFlag)
{
	switch(wFlag)
	{
	case IO_READ:
		{
			if(!pIoHandler->IsIoState(IO_READING, true))
			{
				m_oReadThreadPool.SubmitTask(pIoHandler);					
			}
		}
		break;
	case IO_WRITE:
		{
			if(!pIoHandler->IsIoState(IO_WRITING, true))
			{
				m_oWriteThreadPool.SubmitTask(pIoHandler); 						
			}
		}
		break;
	default:
		break;
	}
}

void* CSvrFrame::IoEvent(void* pParam)
{
	CSvrFrame* pSvrFrame = (CSvrFrame*)pParam;
	if(NULL == pSvrFrame)
		return (void*)NULL;

	Logger.Log(INFO, "start epoll_wait");

	for(;;)
	{
		int iRet = epoll_wait(pSvrFrame->m_iEpollFd, pSvrFrame->m_stEpollEvs, MAX_SOCKET_NUM, 1000);
		if(-1 == iRet) 
		{
			if(EBADF == errno || EINVAL == errno || EFAULT == errno)
			{
				Logger.Log(ERROR, "epoll_wait error:%s", strerror(errno));
			}
		}

		for( int i = 0; i < iRet; i++ )
		{
			CIoHandler* pIoHandler = (CIoHandler*)pSvrFrame->m_stEpollEvs[i].data.ptr;
			if(NULL == pIoHandler)
				continue;

			if(pSvrFrame->m_stEpollEvs[i].events & EPOLLIN)
			{
				if (pIoHandler->m_iSockFd == pSvrFrame->m_iListenFd)
				{
					//accept
					pSvrFrame->AcceptNew(pIoHandler->m_iSockFd);
				}
				else
				{
					//recv	
					pSvrFrame->SubmitTask(pIoHandler, IO_READ);
				}								
			}

			if(pSvrFrame->m_stEpollEvs[i].events & EPOLLOUT)
			{
				//send
				pSvrFrame->SubmitTask(pIoHandler, IO_WRITE);
			}

			if(pSvrFrame->m_stEpollEvs[i].events & EPOLLERR ) 
			{
				//error
				Logger.Log(ERROR, "socket:%d have error:%s", pIoHandler->m_iSockFd, strerror(errno));
			}
		}//end of for
	}

	return (void*)NULL;
}

void CSvrFrame::IoHandle(void* pParam, uint16 wFlag)
{
	CIoHandler* pIoHandler = (CIoHandler*)pParam;
	if (NULL != pIoHandler)
	{
		switch(wFlag)
		{
		case IO_READ:
			{
				pIoHandler->Read();
				pIoHandler->SetIoState(IO_READING, false);
			}
			break;
		case IO_WRITE:
			{
				pIoHandler->Write();
				pIoHandler->SetIoState(IO_WRITING, false);
			}
			break;
		default:
			break;
		}	
	}
}

bool CSvrFrame::AddFd(int iSockFd, int iEvent, void* ptr /* = NULL */)
{
	struct epoll_event ev = {0};
	ev.events			  = iEvent;	
	ev.data.ptr		      = ptr;//note: ev.data is union
	if (epoll_ctl(m_iEpollFd,  EPOLL_CTL_ADD, iSockFd, &ev) < 0)
	{
		Logger.Log(ERROR, "add socket to epoll error:%s", strerror(errno));

		return false;
	}

	return true;
}

bool CSvrFrame::DelFd(int iSockFd)
{
	struct epoll_event ev = {0};
	if(epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, iSockFd, &ev) < 0)
	{
		Logger.Log(ERROR, "delete socket from epoll error:%s", strerror(errno));

		return false;
	}

	return true;
}

bool CSvrFrame::CloseFd(int& iSockFd)
{
	if( iSockFd > 0 )
	{
		close(iSockFd);
		iSockFd = -1;
	}

	return true;
}

bool CSvrFrame::SetNonblock(int iSockFd, bool bIsNonBlock)
{
	int iFlag = fcntl( iSockFd, F_GETFL, 0);
	if(iFlag < 0) 
	{
		Logger.Log(ERROR, "socket:%d from fcntl error:%s", iSockFd, strerror(errno));

		return false;
	}

	if(bIsNonBlock)
	{
		if(fcntl(iSockFd, F_SETFL, iFlag | O_NONBLOCK) < 0)
		{
			Logger.Log(ERROR, "socket:%d from fcntl error:%s", iSockFd, strerror(errno));

			return false;
		}
	}
	else
	{
		if(fcntl( iSockFd, F_SETFL, iFlag & ~O_NONBLOCK) < 0)
		{
			Logger.Log(ERROR, "socket:%d from fcntl error:%s", iSockFd, strerror(errno));

			return false;
		}
	}

	return true;
}

bool CSvrFrame::SetReuseAddr(int iSockFd, int iOption)
{
	//reuse the address
	int opt = iOption;	//on
	if(setsockopt(iSockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		Logger.Log(ERROR, "the socket reuse the address failed!");

		return false;
	}

	return true;
}

bool CSvrFrame::SetLinger(int iSockFd, int iOption)
{
	struct linger stLinger;
	stLinger.l_onoff  = iOption;//0 = off, nozero = on
	stLinger.l_linger = 0;	//timeout
	if(setsockopt(iSockFd, SOL_SOCKET, SO_LINGER, &stLinger, sizeof(struct linger)) < 0) 
	{
		Logger.Log(ERROR, "the socket SetLinger failed!");

		return false;
	}

	return true;
}
