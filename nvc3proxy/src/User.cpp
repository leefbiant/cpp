#include "User.h"
#include "Log.h"
#include "Config.h"
#include "SvrFrame.h"
#include <linux/sockios.h>

#define CHECK_CONN 												\
do{																\
	if( !m_bconned )											\
	{															\
		if( 0 != Connect())										\
		{														\
			CMsgBase* pMsg = CMsgBase::NewMsg(CMD_SYS_CLOSE_USER);						\
			pMsg->m_Reserve  = m_dwSessionId;						\
			pMsg->m_Session  = m_mouleid;					\
			m_pMgr->SetMsg(pMsg);								\
			return iRet;										\
		}														\
	}															\
}while(0)
//////////////////////////////////////////////////////////////////////////
CUser::CUser(CUserMgr* pMgr):
m_pMgr(pMgr),
m_oP2pMsgQ(ClearMsgQ)
{
	m_tOnlineTime = 0;
}

CUser::~CUser()
{
	Clean();
	pthread_mutex_destroy(&m_mutex);
}

void CUser::Clean()
{
	m_bconned = false;
	m_mouleid = 0;
	m_uid     = 0;
}
void CUser::Reset()
{
	m_tOnlineTime   = time(NULL);
	m_oP2pMsgQ.Clear();
	Clean();
	CIoHandler::Reset();
}

int CUser::Connect()
{
	uint32 err = 0;
	uint32 errlen = sizeof(err);

	if ( getsockopt(m_iSockFd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) 
	{
		Logger.Log(ERROR, "getsockopt socket[%d] faile[%s]", m_iSockFd, strerror(errno));
		return -1;
	}
	if( 0 != err )
	{
		Logger.Log(ERROR, "getsockopt socket[%d] get a err[%d]", m_iSockFd, err);
		return -1;
	}
	m_bconned = true;

	Logger.Log(NOTIC, "user[%llu] Session[%u] modid[%u] to CServer sucess", m_uid, m_dwSessionId, m_mouleid);

	return 0;
}
int CUser::Read()
{
	
	int iRet = IO_SUCCESS;
	if( IsIoState(IO_FINIT, false) ||  m_iSockFd < 0 )
	{
		//Logger.Log(ERROR, "This is a Err uid session[%u] iostat[%p] socket[%d]", m_dwSessionId, IO_FINIT, m_iSockFd);	
		return iRet;
	}
	CHECK_CONN;

	do 
	{
		iRet = RecvPkg();
		if (m_stRecvBuffer.iSize > 0)
		{
			int dwRead = 0;
			int dwSize = m_stRecvBuffer.iSize;
			uint8* p   = m_stRecvBuffer.cBuffer;
			do
			{
				CMsgBase* pMsg = NULL;
				int iErr = CMsgBase::NewMsgFromBuf(p, dwSize, &pMsg);
				if (ERR_SUCCESS == iErr)
				{
					pMsg->m_Reserve = m_dwSessionId;
					if (ERR_SUCCESS == HandlePkg(pMsg))
					{
						CMsgBase::DelMsg(pMsg);
					}

					if (dwRead + dwSize == (int)m_stRecvBuffer.iSize)
					{
						m_stRecvBuffer.iSize = 0;
						break;
					}
				} 
				else if (ERR_NO_MORE_DATA == iErr)
				{
					if (p != m_stRecvBuffer.cBuffer)
					{
						int dwLen = m_stRecvBuffer.iSize - dwRead;
						memmove(m_stRecvBuffer.cBuffer, p, dwLen);
						m_stRecvBuffer.iSize = dwLen;
					}
					else
					{
						if (m_stRecvBuffer.iSize >= MAX_BUFFER_SIZE)
						{
							Logger.Log(NOTIC, "the msg size is more than the max buffer");
							m_stRecvBuffer.iSize = 0;
						}
					}
					break;
				}
				else
				{
					m_stRecvBuffer.iSize = 0;
					break;
				}

				p      += dwSize;
				dwRead += dwSize;
				dwSize  = m_stRecvBuffer.iSize - dwRead;

			} while (dwRead < (int)m_stRecvBuffer.iSize);
		}

		if (IO_CLOSE == iRet)
		{
			Logger.Log(NOTIC, "CServer has close socket:[%d] user[%llu] session[%u] modid[%u]",  
				m_iSockFd, m_uid, m_dwSessionId, m_mouleid);
				
			SetIoState(IO_FINIT, true);
			CMsgBase* pMsg = CMsgBase::NewMsg(CMD_SYS_CLOSE_USER);
			pMsg->m_Session  = m_mouleid;
			pMsg->m_Reserve = m_dwSessionId;
			m_pMgr->SetMsg(pMsg);
		}
		
	} while (IO_SUCCESS == iRet);

	return ERR_SUCCESS;
}

int CUser::Write()
{

	int iRet = IO_SUCCESS;
	if( IsIoState(IO_FINIT, false) || m_iSockFd < 0 )
	{
			
		return iRet;
	}
	CHECK_CONN;
	do 
	{
		if (0 == m_stSendBuffer.iSize && 0 == m_stSendBuffer.iTotal )
		{
			CMsgBase* pMsg = NULL;
			if (ERR_SUCCESS != m_oP2pMsgQ.GetObject(&pMsg) && NULL == pMsg)
				break;

			int dwSize = MAX_BUFFER_SIZE;
			int iErr = pMsg->GetBufFromMsg(m_stSendBuffer.cBuffer, dwSize, pMsg);
			
			
			if (ERR_SUCCESS != iErr)
			{
				Logger.Log(ERROR, "GetBufFromMsg failed!");
				continue;
			}

			m_stSendBuffer.iTotal = dwSize;
		} 
		iRet = SendPkg();

	} while (IO_SUCCESS == iRet);

	return ERR_SUCCESS;
}

int CUser::SetP2pMsg(CMsgBase* pMsg)
{
	pMsg->UpdateSession(0);
	int iErr = m_oP2pMsgQ.SetObject(pMsg);
	if (ERR_SUCCESS == iErr && m_bconned )
	{
		m_pMgr->SubmitTask(this, IO_WRITE);
	}	

	return iErr;
}

bool CUser::IsTimeout(uint32 dwMaxTime)
{
	if(m_tActiveTime <= 0)
		return false;
	else if ((time(NULL) - m_tActiveTime) > (int32)dwMaxTime)
	{
		return true;
	}

	return false;
}
void CUser::ClearMsgQ(void* pVoid)
{
	CMsgBase::DelMsg((CMsgBase*)pVoid);
}

int CUser::HandlePkg(CMsgBase* pMsg)
{
	
	if(CMD_SYS_HEARTBEAT != pMsg->m_wCmd && CMD_SYS_HEARTBEAT_ACK != pMsg->m_wCmd)
	{
		Logger.Log(DEBUG, "user session[%u] modid[%u] cmd:0x%04x[%s], from user:%lld to user:%lld", 
		    m_dwSessionId, m_mouleid, pMsg->m_wCmd, getCmdDesc(pMsg->m_wCmd).c_str(), pMsg->m_llSrcUid, pMsg->m_llDesUid);
	}
    
	//note: if the pMsg is continue to be used, please remember to return ERR_FAILED
	int iErr = ERR_SUCCESS;

	pMsg->UpdateSession(m_dwSessionId);	
	switch(pMsg->m_wCmd)
	{
	
		default:
		{
			CModule* pModule = m_pMgr->GetModule(m_mouleid);
			if( pModule )
			{
				pModule->SetModuleMsg(pMsg);
				iErr = RET_FAILED;
			}
			else
			{
				Logger.Log(ERROR, "Err user[%llu] session[%u] cmd[%p] not find modid[%u]", 
		   				 m_uid, m_dwSessionId, pMsg->m_wCmd, m_mouleid);
			}
		}
		break;
	}

	return iErr;
}

CUserMgr::CUserMgr(CSvrFrame* pSvrFrame):
m_pSvrFrame(pSvrFrame)
{

}

CUserMgr::~CUserMgr()
{

}

int CUserMgr::Init()
{
	pthread_t threadid;
	if(pthread_create(&threadid, NULL, Callback, (void*)this) != 0)
	{
		Logger.Log(ERROR, "CUserMgr pthread_create failed");
		return -1;
	}	
	return 0;
}

CUser* CUserMgr::GetIdleUser()
{
	CUser* pUser = NULL;
	
	if (ERR_SUCCESS == m_oIdleUserQ.GetObject(&pUser) &&  NULL != pUser)
	{
		if( pUser->IsIoState(IO_READING, false) || pUser->IsIoState(IO_WRITING, false))
		{
			Logger.Log(ERROR,"GetIdleUser failed user[%p]  is IO_READING or IO_READING iostat[%p]", pUser,  pUser->GetIOStat());

			m_oIdleUserQ.SetObject(pUser);
			pUser = new CUser(this);
		}	
		else
		{
			Logger.Log(DEBUG,"GetIdleUser success user[%p] iostat[%p]", pUser,  pUser->GetIOStat());
		}
	}
	else
	{
		pUser = new CUser(this);
	}
	
	pUser->Reset();
	pUser->SetIoState(IO_BUSY, true);

	return pUser;
}

int CUserMgr::SetIdleUser(CUser* pUser)
{
	CUser* pWebUser = (CUser*)pUser;
	if (pWebUser->IsIoState(IO_BUSY))
	{
		pWebUser->SetIoState(IO_BUSY, false);
		return m_oIdleUserQ.SetObject(pWebUser);
	}
	
	Logger.Log(ERROR, "the user session[%u] io state is not busy", pUser->m_dwSessionId);
	return ERR_FAILED;
}

void CUserMgr::CheckTimeout(uint32 dwMaxTime)
{
	MAPSESSION::iterator it = m_mapServerUser.begin();
	for (; it != m_mapServerUser.end(); it++)
	{
		MAPUSER* pModUserMap  = (MAPUSER*)it->second;
		MAPUSER ::iterator it1 = pModUserMap->begin();
		for (; it1 != pModUserMap->end(); it1++)
		{
			CUser* pUser = (CUser*)it1->second;
			if (pUser && pUser->IsTimeout(dwMaxTime))
			{
				Logger.Log(NOTIC, "server user[%llu]  session[%u] mod[%u] timeout  online time:%ds", 
					 pUser->m_uid, pUser->m_dwSessionId, pUser->m_mouleid, pUser->GetOnlineTime());
				pUser->SetIoState(IO_FINIT, true);
				CMsgBase* pMsg = CMsgBase::NewMsg(CMD_SYS_CLOSE_USER);
				pMsg->m_Session = pUser->m_mouleid;
				pMsg->m_Reserve = pUser->m_dwSessionId;
				SetMsg(pMsg);
			}
		}
	}
	
}

void CUserMgr::SubmitTask(CIoHandler* pIoHandler, uint16 wFlag)
{
	m_pSvrFrame->SubmitTask(pIoHandler, wFlag);
}

void CUserMgr::SetMsg(CMsgBase* pMsg)
{
	m_oMsgQ.SetObject(pMsg);
	if (m_oSem.GetValue() <= 0)
	{
		m_oSem.Post();
	}
}

void* CUserMgr::Callback(void* pParam)
{
	CUserMgr* pMgr = (CUserMgr*)pParam;
	if(NULL != pMgr)
		pMgr->DispatchMsg();
	return (void*)NULL;
}

int CUserMgr::DispatchMsg()
{
	static time_t now_time = time(0);
	for (;;)
	{
		if( time(0) - now_time >  30 )
		{
			now_time = time(0);
			CheckTimeout(MAX_CHECK_TIMEOUT);
		}
		
		m_oSem.timedwait();
		
		int i_task = 0;
		vector<CMsgBase*> pMsgList;
		while( !i_task )
		{
			if( ERR_FAILED == m_oMsgQ.GetObjectAll(pMsgList))break;
			vector<CMsgBase*>::iterator it = pMsgList.begin();
			for(; it != pMsgList.end(); it++)
			{
				CMsgBase* pMsg = (CMsgBase*)*it;
				if( NULL == pMsg)continue;
				if (ERR_SUCCESS == HandlePkg(pMsg))
				{
					delete pMsg;
				}
			}
			if( !pMsgList.empty() )i_task++;
			pMsgList.clear();
		}
		Stat();
	}
	return ERR_SUCCESS;
}

void CUserMgr::SetModMgrMsg(CMsgBase* pMsg)
{
	m_pSvrFrame->m_oModuleMgr.SetMsg(pMsg);
}

CModule* CUserMgr::GetModule(uint32 modid)
{
	return m_pSvrFrame->m_oModuleMgr.GetModuleByMid(modid);
}

int CUserMgr::HandlePkg(CMsgBase* pMsg)
{
	int iErr = ERR_SUCCESS;
	Logger.Log(NOTIC, "CUserMgr::HandlePkg cmd:0x%04x[%s], from user:%lld to user:%lld user session[%u]" , 
		    pMsg->m_wCmd, getCmdDesc(pMsg->m_wCmd).c_str(), pMsg->m_llSrcUid, pMsg->m_llDesUid, pMsg->m_Reserve);
		    
	switch( pMsg->m_wCmd)
	{
		case CMD_SYS_CLOSE_USER:
		case CMD_SYS_CLOSE_MOD:
		case CMD_SYS_MOD_CLOSE_USER:
		{
			iErr = HandleCloseUser(pMsg);
			break;
		}
		case CMD_USER_LOGIN:
		{
   			iErr = HandleUserLogin(pMsg);
   			break;
   		}	 
		default:
		{
			iErr = HandleProxyMsg(pMsg);
			break;
		}
	}
	return iErr;
}

int CUserMgr::HandleCloseUser(CMsgBase* pMsg)
{
	int iErr = ERR_SUCCESS;
	DelUser(pMsg);
	return iErr;
}

int CUserMgr::HandleUserLogin(CMsgBase* pMsg)
{
	Logger.Log(NOTIC, "HandleUserLogin user:%llu", pMsg->m_llSrcUid);
	sockaddr_in dstaddr;

	dstaddr.sin_family       = AF_INET;
	dstaddr.sin_addr.s_addr = htonl(g_oConfig.m_stArgs.dwCsIp);
	dstaddr.sin_port 		= htons(g_oConfig.m_stArgs.dwCsPort);

	CUser* pUser = GetIdleUser();
	static uint32 dwSessionId = 1;
	pUser->m_dwSessionId  = ++dwSessionId;
	pUser->m_uid 			= pMsg->m_llSrcUid;
	
	Logger.Log(NOTIC, "start user[%llu] Session[%u] to ser [%s:%d]", 
					pUser->m_uid, pUser->m_dwSessionId, intip2char(dstaddr.sin_addr.s_addr), ntohs(dstaddr.sin_port));
	
	if (!m_pSvrFrame->ConnectTo(pUser, dstaddr))
	{
		Logger.Log(ERROR, "user[%llu] Session[%u]  to ser [%s:%d] failed", 
					pUser->m_uid, pUser->m_dwSessionId, intip2char(dstaddr.sin_addr.s_addr), ntohs(dstaddr.sin_port));
		SetIdleUser(pUser);
		return ERR_SUCCESS;
	}

	pUser->SetP2pMsg(pMsg);
	AddUser(pUser);

	return ERR_FAILED;
	
}

int CUserMgr::HandleProxyMsg(CMsgBase* pMsg)
{
	int iErr = ERR_SUCCESS;
	CModule* pModule = pMsg->m_pRoute;

	MAPSESSION::iterator it = m_mapServerUser.find(pModule->m_dwSessionId);
	if( it == m_mapServerUser.end() )
	{
		Logger.Log(INFO, "Err not find modusermap from modid[%u]", pModule->m_dwSessionId);
	}
	else
	{
		MAPUSER* pModUserMap  = (MAPUSER*)it->second;
		MAPUSER ::iterator it1 = pModUserMap->find(pMsg->m_Reserve);
		if (it1 == pModUserMap->end())
		{
			Logger.Log(ERROR, "Err not find user session[%u] if from mod id[%u] msg cmd[%p]", 
				pMsg->m_Reserve, pModule->m_dwSessionId, pMsg->m_wCmd);
			
			CMsgBase* pMsg = CMsgBase::NewMsg(CMD_SYS_SVR_CLOSE_USER);
			pMsg->m_llDesUid   = pMsg->m_llSrcUid;
			pMsg->m_Session    = pModule->m_dwSessionId;
			pMsg->m_Reserve    = pMsg->m_Reserve;
			pModule->SetModuleMsg(pMsg);
		}
		else
		{
			CUser* pUser = it1->second;
			Logger.Log(INFO, "Sucess find user[%llu] session[%u] msg cmd[%p] from modid[%u]",
				pUser->m_uid, pUser->m_dwSessionId, pMsg->m_wCmd, pModule->m_dwSessionId);
			
			pUser->SetP2pMsg(pMsg);
			iErr = ERR_FAILED;
		}
	}

	return iErr;
}

int CUserMgr::AddUser(CUser* pUser)
{

	MAPSESSION::iterator it = m_mapServerUser.find(pUser->m_mouleid);
	if( it != m_mapServerUser.end() )
	{
		MAPUSER* pModUserMap  = (MAPUSER*)it->second;
		MAPUSER ::iterator it1 = pModUserMap->find(pUser->m_dwSessionId);
		if (it1 != pModUserMap->end())
		{
			Logger.Log(ERROR, "Add user[%llu] session[%u] to mod[%u] but user has in the mod ",  
					pUser->m_uid, pUser->m_dwSessionId, pUser->m_mouleid);
		}
		else
		{
			(*pModUserMap)[pUser->m_dwSessionId] = pUser;
			Logger.Log(INFO, "Add user[%llu] session[%u] to mod[%u] ",  
				pUser->m_uid, pUser->m_dwSessionId, pUser->m_mouleid);
		}
	}
	else
	{
		MAPUSER* pModUserMap = new MAPUSER;
		(*pModUserMap)[pUser->m_dwSessionId] = pUser;
		m_mapServerUser[pUser->m_mouleid]  = pModUserMap;
		Logger.Log(INFO, "Add user[%llu] session[%u] add new mod[%u] ",  
				pUser->m_uid, pUser->m_dwSessionId, pUser->m_mouleid);
	}

	return ERR_SUCCESS;
}


void CUserMgr::DelSessionUser(MAPUSER* pModUserMap, uint32 session, bool bmod)
{
	CUser* pUser = NULL;
	if( session )
	{
		MAPUSER::iterator it = pModUserMap->find(session);
		if (it != pModUserMap->end())
		{
			pUser = it->second;
			Logger.Log(NOTIC, "del user[%llu] session[%u] modid[%u] ", pUser->m_uid, pUser->m_dwSessionId, pUser->m_mouleid);


			if( !bmod )
			{
				CModule* pModule = GetModule(pUser->m_mouleid);
				if( pModule )
				{
					CMsgBase* pMsg = CMsgBase::NewMsg(CMD_SYS_SVR_CLOSE_USER);
					pMsg->m_llDesUid   = pUser->m_uid;
					pMsg->m_Session    = pUser->m_mouleid;
					pMsg->m_Reserve    = pUser->m_dwSessionId;
					pModule->SetModuleMsg(pMsg);
				}
			}
			
			pUser->SetIoState(IO_FINIT, true);
			if( m_pSvrFrame->CloseFd(pUser->m_iSockFd))
			{
				SetIdleUser(pUser);
			}
			pModUserMap->erase(it);
		}
		else
		{
			Logger.Log(NOTIC, "Err del user not find user of sesion[%u]", session);			
		}
	}
	else
	{
		MAPUSER::iterator it = pModUserMap->begin();
		for (; it != pModUserMap->end(); )
		{
			pUser = it->second;
			Logger.Log(NOTIC, "del user[%llu] session[%u] modid[%u] ", pUser->m_uid, pUser->m_dwSessionId, pUser->m_mouleid);

			pUser->SetIoState(IO_FINIT, true);
			if( m_pSvrFrame->CloseFd(pUser->m_iSockFd))
			{
				SetIdleUser(pUser);
			}
			pModUserMap->erase(it++);
		}
	}
}

int CUserMgr::DelUser(CMsgBase* pMsg)
{	
	Logger.Log(NOTIC, "DelUser modid[%u]  session[%u]",  pMsg->m_Session, pMsg->m_Reserve);

	if( pMsg->m_Session )
	{
		MAPSESSION::iterator it = m_mapServerUser.find(pMsg->m_Session);
		if(  it != m_mapServerUser.end())
		{
			DelSessionUser(it->second, pMsg->m_Reserve, pMsg->m_wCmd == CMD_SYS_MOD_CLOSE_USER);
			if( it->second->empty() )
			{
				Logger.Log(NOTIC, "DelUser del mod[%u]",  pMsg->m_Session);
				m_mapServerUser.erase(it);
			}
		}
		else
		{
			Logger.Log(ERROR, "DelUser not find mod [%u]",  pMsg->m_Session);
		}
	}
	else
	{
		Logger.Log(ERROR, "Err DelUser modid[%u]  session[%u] modid is 0",  pMsg->m_Session, pMsg->m_Reserve);
	}
	return ERR_SUCCESS;
}


void CUserMgr::Stat( )
{
	uint32 timeNow = time(0);
	static time_t run_terval = timeNow;//业务主循环运行间隔

	if( timeNow - run_terval < 15 )return;
	run_terval = timeNow;

	uint32 modNum = 0;
	uint32 cliNum = 0;
	
	MAPSESSION::iterator it = m_mapServerUser.begin();
	for( ; it != m_mapServerUser.end(); it++ )
	{
		modNum++ ;
		cliNum += it->second->size();
	}
	
	Logger.Log(NOTIC, "STAT mod num[%u] client num[%u]", modNum, cliNum);
	
}
