#include "Module.h"
#include "Log.h"
#include "SvrFrame.h"



//////////////////////////////////////////////////////////////////////////
CModule::CModule(CModuleMgr* pMgr, MODULE_TYPE mt):
m_pMgr(pMgr),
m_oModuleMsgQ(ClearMsgQ)
{

}

void CModule::ClearMsgQ(void* pVoid)
{
	CMsgBase::DelMsg((CMsgBase*)pVoid);
}

CModule::~CModule()
{

}

int CModule::Read()
{

	int iRet = IO_SUCCESS;
	
	
	if( IsIoState(IO_FINIT, false))
	{
		Logger.Log(EMERG, "This is a Err uid modid[%u] iostat[%p]",m_dwSessionId, IO_FINIT);	
		return iRet;
	}
	do 
	{
		iRet = RecvPkg();
		if (m_stRecvBuffer.iSize > 0)
		{
			//Logger.Log(NOTIC, "recv data, size:%d", m_stRecvBuffer.iSize);
			int dwRead = 0;
			int32 dwSize = m_stRecvBuffer.iSize;
			uint8* p   = m_stRecvBuffer.cBuffer;

			do
			{
				if( !m_Handshake.handshake_ok )
				{
					if( ERR_SUCCESS != handshake(dwRead, p, dwSize))
					{
						return ERR_FAILED;
					}
				}
				else
				{
					int nRet = 0;
					/*
					Logger.Log(NOTIC, "start ws_get_msg msgsize[%d] dwRead[%d] dwSize[%d]", 
								m_stRecvBuffer.iSize, dwRead, dwSize);
					*/
					nRet = ws_get_msg((char*)p, dwSize, &m_Handshake, &m_Frame);
					if( ERR_NO_MORE_DATA == nRet )
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
								Logger.Log(NOTIC, "the msg size is more than the max buffer close user");
								iRet = IO_CLOSE;
								break;
							}
						}
						break;
					}
					else if( ERR_FAILED == nRet )
					{
						Logger.Log(ERROR, "websocket_get_msg from web user failed modid[%u] session[%u]", m_dwSessionId, m_dwSessionId);
						SetIoState(IO_FINIT, true);
						CMsgBase* pMsgCloseUser = CMsgBase::NewMsg(CMD_SYS_CLOSE_MOD);
						pMsgCloseUser->m_Session = m_dwSessionId;
						m_pMgr->SetMsg(pMsgCloseUser);
						return ERR_SUCCESS;
					}
					
					// if ping
					if( m_Frame.op_code == NOPOLL_PING_FRAME )
					{
						CMsgPong* pMsg = (CMsgPong*)CMsgBase::NewMsg(CMD_WEBUSER_PONG);
						SetModuleMsg(pMsg);
					}
					
					else //== youshow msg
					{
						p += dwSize;
						dwRead += dwSize;
						dwSize = m_stRecvBuffer.iSize - dwRead;
						CMsgBase* pMsg = NULL;
						int iErr = CMsgBase::NewMsgFromBuf(p, dwSize, &pMsg);
						if (ERR_SUCCESS == iErr)
						{
							pMsg->m_pRoute = this;
							if (ERR_SUCCESS == HandlePkg(pMsg))
							{
								CMsgBase::DelMsg(pMsg);
							}
						}
						else if (ERR_NO_MORE_DATA == iErr)
						{
							// 
							dwSize +=  m_Frame.header_size;
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
									Logger.Log(NOTIC, "the msg size is more than the max buffer close user");
									iRet = IO_CLOSE;
								}
							}
							break;
						}
						else
						{
							m_stRecvBuffer.iSize = 0;
							break;
						}

						
					}
				}
				p      += dwSize;
				dwRead += dwSize;
				dwSize  = m_stRecvBuffer.iSize - dwRead;
				if (dwRead + dwSize == (int)m_stRecvBuffer.iSize)
				{
					m_stRecvBuffer.iSize = 0;
					break;
				}
			} while (dwRead < (int)m_stRecvBuffer.iSize);
		}

		if (IO_CLOSE == iRet)
		{
			Logger.Log(NOTIC, "had closed the socket:[%d]  session[%u]",  m_iSockFd, m_dwSessionId);
			SetIoState(IO_FINIT, true);
			CMsgBase* pMsgCloseUser = CMsgBase::NewMsg(CMD_SYS_CLOSE_MOD);
			pMsgCloseUser->m_Session = m_dwSessionId;
			m_pMgr->SetMsg(pMsgCloseUser);
		}
		
	} while (IO_SUCCESS == iRet);

	return ERR_SUCCESS;
}

int CModule::Write()
{
	
	int iRet = IO_SUCCESS;
	while (IO_SUCCESS == iRet) 
	{
		if (0 == m_stSendBuffer.iSize && 0 == m_stSendBuffer.iTotal)
		{
			CMsgBase* pMsg = NULL;
			if (ERR_SUCCESS != m_oModuleMsgQ.GetObject(&pMsg) && NULL == pMsg)
				break;

			
			int dwSize = MAX_BUFFER_SIZE;
			uint8	cBuffer[MAX_BUFFER_SIZE];
			int iErr = pMsg->GetBufFromMsg(cBuffer, dwSize, pMsg);

		

			if (ERR_SUCCESS != iErr)
			{
				CMsgBase::DelMsg(pMsg);
				continue;
			}
			uint16 Msgcmd = pMsg->m_wCmd;
			//handshake
			if( CMD_WEBUSER_HTTPRESP == Msgcmd )
			{
				memcpy(m_stSendBuffer.cBuffer, cBuffer, dwSize);
				m_stSendBuffer.iTotal = dwSize;
			}
			else
			{
				
				WS_Pack(pMsg, (char*)cBuffer, dwSize);
				Logger.Log(DEBUG, "write msg mod[%u] user session[%u] cmd[%p]  src[%llu] dst[%llu]", 
						m_dwSessionId, pMsg->m_Reserve, pMsg->m_wCmd, pMsg->m_llSrcUid, pMsg->m_llDesUid);
				CMsgBase::DelMsg(pMsg);
			}
			
		} 

		iRet = SendPkg();

	};

	if (IO_CLOSE == iRet)
	{
		Logger.Log(NOTIC, "mod[%u] had closed the socket:%d!", m_dwSessionId, m_iSockFd);
		SetIoState(IO_FINIT, true);
		CMsgBase* pMsgCloseUser = CMsgBase::NewMsg(CMD_SYS_CLOSE_MOD);
		pMsgCloseUser->m_Session = m_dwSessionId;
		m_pMgr->SetMsg(pMsgCloseUser);
	}
	return ERR_SUCCESS;
}

int CModule::WS_Pack(CMsgBase* pMsg, char* pMsgbuf, uint64 Msglen)
{
	int32 header_size = 2;
	char* p = (char*)m_stSendBuffer.cBuffer;
	uint16 Msgcmd = pMsg->m_wCmd;
	
	memset(p, 0x00, 14);
	p[0] |= (1 << 7); //FIN
	if( Msgcmd == CMD_WEBUSER_PONG )
	{
		p[0] |= NOPOLL_PONG_FRAME & 0x0f; //op
	}
	else if( Msgcmd == CMD_WEBUSER_ECHO )
	{
		p[0] |= NOPOLL_TEXT_FRAME & 0x0f; //op
	}
	else
	{	
		p[0] |= NOPOLL_BINARY_FRAME & 0x0f; //op
	}
	
	
	if (Msglen < 126) 
	{
		p[1] |= Msglen;
		p += 2;
	} 
	else if (Msglen < 65535) 
	{
		/* set the next header length is at least 65535 */
		p[1] |= 126;
		p += 2;
		*((uint16*)p) = htons(Msglen);
		p += 2;
		header_size += 2;
	} 
	else 
	{
		p[1] |= 127;
		p += 2;
		*((uint64*)p) = htonll(Msglen);
		p += 8;
		header_size += 8;
	}
	memcpy(p, pMsgbuf, Msglen);
	p += Msglen;
	
	m_stSendBuffer.iTotal = Msglen + header_size;	

	//if(CMD_USER_HEARTBEAT != pMsg->m_wCmd && CMD_USER_HEARTBEAT_ACK != pMsg->m_wCmd)
	//Logger.Log(NOTIC, "<<< write room msg [%p] to user session[%u] uid[%llu] header_size[%d] size[%d] encrypt_type[%d]", pMsg->m_wCmd, m_dwSessionId, m_llUid, header_size, Msglen, m_encrypt_type);

	return 0;		
}
int CModule::handshake(int& dwRead, uint8* p,  int32& dwSize)
{
	char reply[4096] = {0};
	int nRet = 0;
	nRet = ws_parse_handshake((const char*)p, dwSize, &m_Handshake, reply);
	if( ERR_NO_MORE_DATA == nRet )return ERR_NO_MORE_DATA;
	else if( ERR_FAILED == nRet )
	{
		Logger.Log(ERROR, "webuser handshake failed");
		SetIoState(IO_FINIT, true);
		CMsgBase* pMsgCloseUser = CMsgBase::NewMsg(CMD_SYS_CLOSE_MOD);
		pMsgCloseUser->m_Session = m_dwSessionId;
		m_pMgr->SetMsg(pMsgCloseUser);
		return ERR_SUCCESS;
	}
	if ( strlen(reply) == 0 )
	{
		Logger.Log(ERROR, "this is a bug NULL == reply");
		SetIoState(IO_FINIT, true);
		CMsgBase* pMsgCloseUser = CMsgBase::NewMsg(CMD_SYS_CLOSE_MOD);
		pMsgCloseUser->m_Session = m_dwSessionId;
		m_pMgr->SetMsg(pMsgCloseUser);
		return ERR_SUCCESS;
	}
	
	CMsgHttpResp* pMsg = (CMsgHttpResp*)CMsgBase::NewMsg(CMD_WEBUSER_HTTPRESP);
	pMsg->m_HttpResp = reply;
	SetModuleMsg(pMsg);


	dwRead = m_Handshake.headlen;
	if (dwRead + dwSize == (int)m_stRecvBuffer.iSize)
	{
		m_stRecvBuffer.iSize = 0;
	}
	return ERR_SUCCESS;
}

void CModule::Reset()
{
	
	CIoHandler::Reset();
	m_oModuleMsgQ.Clear();
	m_Handshake.clean();
	m_Handshake.clean();
}

void CModule::Reset2()
{
	CIoHandler::Reset();
}


int CModule::SetModuleMsg(CMsgBase* pMsg)
{
	int iErr = m_oModuleMsgQ.SetObject(pMsg);
	if (ERR_SUCCESS == iErr)
	{
		m_pMgr->SubmitTask(this, IO_WRITE);
	}	

	return iErr;
}

bool CModule::IsTimeout(uint32 dwMaxTime)
{
	//Logger.Log(NOTIC, "%d, %d", m_tActiveTime, dwMaxTime);

	if(m_tActiveTime <= 0)
		return false;
	else if ((time(NULL) - m_tActiveTime) > (int32)dwMaxTime)
	{
		return true;
	}

	return false;
}

int CModule::HandlePkg(CMsgBase* pMsg)
{
	//if(CMD_SYS_HEARTBEAT != pMsg->m_wCmd && CMD_SYS_HEARTBEAT_ACK != pMsg->m_wCmd)
	{
		Logger.Log(NOTIC, ">>>>>> mid:%d CModule::HandlePkg cmd:0x%04x[%s], from user:%llu to user:%llu user session[%d]", 
		    m_dwSessionId, pMsg->m_wCmd, getCmdDesc(pMsg->m_wCmd).c_str(), pMsg->m_llSrcUid, pMsg->m_llDesUid, pMsg->m_Reserve);
	}
    
	//note: if the pMsg is continue to be used, please remember to return ERR_FAILED
	int iErr = ERR_SUCCESS;
	
	switch(pMsg->m_wCmd)
	{

		case CMD_USER_HEARTBEAT:
			{
				if( pMsg->m_Reserve == 0 )
				{
					CMsgBase* pHbMsg = new CMsgBase;
					pHbMsg->m_wCmd = CMD_USER_HEARTBEAT_ACK;
					string heartbeatack = "{\"rcode\":0,\"step\":180}";
					
					pHbMsg->m_pmsg = new uint8[heartbeatack.size()+1];
					memcpy(pHbMsg->m_pmsg, heartbeatack.c_str(), heartbeatack.length());
					pHbMsg->m_dwLen += heartbeatack.length();					
					SetModuleMsg(pHbMsg);
					break;
				}
				//set to module mgr
			}
		default:
		{
			Logger.Log(NOTIC, "module handle unkown cmd:0x%04x", pMsg->m_wCmd);
			m_pMgr->SetMsg(pMsg);
			iErr = RET_FAILED;
			break;
		}
		
	}

	return iErr;
}


//////////////////////////////////////////////////////////////////////////
CModuleMgr::CModuleMgr(CSvrFrame* pSvrFrame):m_pSvrFrame(pSvrFrame)
{
	m_dwCreateModuleNums = 0;
}

CModuleMgr::~CModuleMgr()
{

}

CModule* CModuleMgr::GetIdleModule()
{
	CModule* pModule = NULL;
	if (ERR_SUCCESS != m_oIdleModuleQ.GetObject(&pModule))
	{
		pModule = new CModule(this, MT_SERVER);
		m_dwCreateModuleNums++;
	}
	pModule->Reset();

	//static uint32 dwSessionId = 10000;
	
	//pModule->m_dwSessionId = ++dwSessionId; 
	
	return pModule;
}

int CModuleMgr::SetIdleModule(CModule* pModule)
{
	if (pModule->IsIoState(IO_BUSY))
	{
		pModule->SetIoState(IO_BUSY, false);
		pModule->m_iSockFd = -1;

		return m_oIdleModuleQ.SetObject(pModule);		
	}

	Logger.Log(ERROR, "*** the module io state is not busy");
	return ERR_FAILED;
}

void CModuleMgr::CheckTimeout(uint32 dwMaxTime)
{
	
	//for auth module
	MAPMODULE::iterator it = m_mapAuthModule.begin();
	for (; it != m_mapAuthModule.end(); it++)
	{
		CModule* pModule = (CModule*)it->second;
		if (pModule->IsTimeout(dwMaxTime))
		{
			Logger.Log(NOTIC, "*** module:%d is not active for a long time, and to close it", pModule->m_dwSessionId);

			pModule->SetIoState(IO_FINIT, true);
			CMsgBase* pMsgCloseUser = CMsgBase::NewMsg(CMD_SYS_CLOSE_MOD);
			pMsgCloseUser->m_Session = pModule->m_dwSessionId;
			SetMsg(pMsgCloseUser);		
		} 
	}

	
}

void CModuleMgr::Statistics()
{
}

void CModuleMgr::OnTimer(const uint32 dwID)
{
	
}

void CModuleMgr::SubmitTask(CIoHandler* pIoHandler, uint16 wFlag)
{
	m_pSvrFrame->SubmitTask(pIoHandler, wFlag);
}

void CModuleMgr::SetMsg(CMsgBase* pMsg)
{
	m_oMsgQ.SetObject(pMsg);
	if (m_oSem.GetValue() <= 0)
	{
		m_oSem.Post();
	}
}

int CModuleMgr::DispatchMsg()
{
	for (;;)
	{
		m_oSem.timedwait();

		do 
		{
			CMsgBase* pMsg = NULL;
			if(ERR_SUCCESS != m_oMsgQ.GetObject(&pMsg) && NULL == pMsg)
				break;

			if (ERR_SUCCESS == HandlePkg(pMsg))
			{
				CMsgBase::DelMsg(pMsg);
			}

		} while (true);
		
		Stat();
	}

	return ERR_SUCCESS;
}

void CModuleMgr::Stat()
{
	static time_t now_time = time(0);
	if( time(0) - now_time >  30 )
	{
		now_time = time(0);
		CheckTimeout(MAX_CHECK_TIMEOUT);

		Logger.Log(NOTIC, "STAT mod num[%u]", m_mapAuthModule.size());
	}
	
}

void CModuleMgr::SetUserMgrMsg(CMsgBase* pMsg)
{
	m_pSvrFrame->m_oUserMgr.SetMsg(pMsg);
}

int CModuleMgr::HandlePkg(CMsgBase* pMsg)
{
	
	//note: if the pMsg is continue to be used, please remember to return ERR_FAILED
	int iErr = ERR_SUCCESS;	

	switch(pMsg->m_wCmd)
	{
 		case CMD_SYS_MOD_CONN:
			 iErr = HandleModConn(pMsg);
   			 break;
		case CMD_SYS_CLOSE_MOD:
			 iErr = HandleCloseModUser(pMsg);
   			 break;
   		case CMD_SYS_MOD_CLOSE_USER:
			 iErr = HandleModCloseUser(pMsg);
   			 break;	 
		default:
		{
			Logger.Log(NOTIC, "mgr handle  cmd:0x%04x set to usermgr", pMsg->m_wCmd);
			SetUserMgrMsg(pMsg);
			iErr =  ERR_FAILED;
		}
		break;
	}

	return iErr;
}

int CModuleMgr::HandleModConn(CMsgBase* pMsg)
{
	int iErr =  ERR_SUCCESS;
	CMsgModConn* pMsgModConn = (CMsgModConn*)pMsg;
	
			
	CModule*  pModule = GetIdleModule();
	
	pModule->m_iSockFd     				= pMsgModConn->m_ModuleInfo.sock;
	pModule->m_eIoType     				= pMsgModConn->m_ModuleInfo.IoType;
	pModule->m_dwIp  					= pMsgModConn->m_ModuleInfo.dwIp;
	pModule->m_wPort 					= pMsgModConn->m_ModuleInfo.wPort;
	pModule->m_dwSessionId				= pMsgModConn->m_ModuleInfo.session;
	pModule->m_fCallback   				= pMsgModConn->m_ModuleInfo.fCallback;
	pModule->SetIoState(IO_BUSY, true);
	
	if (!m_pSvrFrame->AddFd(pModule->m_iSockFd, EPOLLIN| EPOLLOUT| EPOLLET| EPOLLERR, pModule))
	{
		Logger.Log(ERROR, "Add fd[%d] epoll failed process exit....",pModule->m_iSockFd);
		sleep(1);
		exit(0);
	}
	
	AddModule(pModule);
	
	return iErr;
}


int CModuleMgr::HandleCloseModUser(CMsgBase* pMsg)
{
	int iErr =  ERR_SUCCESS;
	
	Logger.Log(ERROR, "HandleCloseModUser has close mod[%u]", pMsg->m_Session);
	CModule* pModule = GetModuleByMid(pMsg->m_Session);
	if( pModule )
	{
		DelModule(pModule);
		SetUserMgrMsg(pMsg);
		iErr =  ERR_FAILED;
	}
	else
	{
		Logger.Log(ERROR, "HandleCloseModUser has not find mod[%u]", pMsg->m_Session);
	
	}

	return iErr;
}

int CModuleMgr::HandleModCloseUser(CMsgBase* pMsg)
{

	if( pMsg->m_Reserve == 0 )
	{
		Logger.Log(ERROR, "HandleModCloseUser  mod[%u] session[%u]", pMsg->m_Session, pMsg->m_Reserve);
		return ERR_SUCCESS;
	}
	pMsg->m_Session   = pMsg->m_pRoute->m_dwSessionId;
	SetUserMgrMsg(pMsg);
				
	Logger.Log(NOTIC, "HandleModCloseUser has close mod[%u] session[%u]",pMsg->m_Session, pMsg->m_Reserve);
		
	return ERR_FAILED;
}
int CModuleMgr::HandleErrModUser(CMsgBase* pMsg)
{
	int iErr =  ERR_SUCCESS;
	
	Logger.Log(ERROR, "HandleErrModUser has close mod[%u] user[%u]", pMsg->m_Session, pMsg->m_Reserve);
	CModule* pModule = GetModuleByMid(pMsg->m_Session);
	if( pModule )
	{
		
	}
	else
	{
		Logger.Log(ERROR, "HandleCloseModUser has not find mod[%u]", pMsg->m_Reserve);
	
	}

	return iErr;
}



int CModuleMgr::AddModule(CModule* pModule)
{

	MAPMODULE::iterator it2 = m_mapAuthModule.find(pModule->m_dwSessionId);
	if (it2 != m_mapAuthModule.end() )
	{	
		Logger.Log(ERROR, "AddModule but find mod module:%d, socket:%d", pModule->m_dwSessionId, pModule->m_iSockFd);
		if( it2->second == pModule )
		{
			Logger.Log(ERROR, "AddModule  module:%d has exist", pModule->m_dwSessionId, pModule->m_iSockFd);
		}
		sleep(1);
		exit(0);
	}
	else
	{
		m_mapAuthModule[pModule->m_dwSessionId] = pModule;
		Logger.Log(NOTIC, "module:%d, socket:%d, add success", pModule->m_dwSessionId, pModule->m_iSockFd);
	}

	return ERR_SUCCESS;
}

int CModuleMgr::DelModule(CModule* pModule)
{

	MAPMODULE::iterator it = m_mapAuthModule.find(pModule->m_dwSessionId);
	if (it != m_mapAuthModule.end())
	{
		m_mapAuthModule.erase(it);
	}
	else
	{
		Logger.Log(ERROR, "AddModule  module:%d but not find mod[%u]", pModule->m_dwSessionId);
		sleep(1);
		exit(0);
	}
	
	m_pSvrFrame->CloseFd(pModule->m_iSockFd);
	SetIdleModule(pModule);

	return ERR_SUCCESS;
}

CModule* CModuleMgr::GetModuleByMid(uint32 SessionId)
{
	CModule* pModule = NULL;
	MAPMODULE::iterator it = m_mapAuthModule.find(SessionId);
	if (it != m_mapAuthModule.end())
	{
		pModule = (CModule*)it->second;
	}
	return pModule;
}

