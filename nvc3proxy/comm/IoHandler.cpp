#include "IoHandler.h"
#include "Log.h"

#define CAS __sync_bool_compare_and_swap 
CIoHandler::CIoHandler()
{
	Reset();
	
}

CIoHandler::~CIoHandler()
{

}

void CIoHandler::Reset()
{
	m_iSockFd     = -1;
	m_eIoType     = (IO_TYPE)-1;
	m_tActiveTime = time(NULL);

	memset(&m_stSendBuffer, 0, sizeof(BUFFER_t));
	memset(&m_stRecvBuffer, 0, sizeof(BUFFER_t));
	m_wIoState = 0;
	m_dwSessionId = 0;
}

void CIoHandler::Close(int iErrCode)
{
	m_tActiveTime = time(NULL);
	memset(&m_stSendBuffer, 0, sizeof(BUFFER_t));
	memset(&m_stRecvBuffer, 0, sizeof(BUFFER_t));
}

void CIoHandler::SetIoState(uint16 iState, bool bSet)
{
#if 0
	CAutoLock oLock(m_oLockState.GetHandle());
	if (bSet)
	{
		m_wIoState |= iState;
	} 
	else
	{
		m_wIoState &= ~iState;
	}
#else
	uint16 oldstat = 0;
	uint16 newstat = 0;
	if (bSet)
	{
		do
		{
			oldstat = m_wIoState;
			newstat = oldstat | iState;
			if(!CAS(&m_wIoState, oldstat, newstat))
			{
				//Logger.Log(NOTIC, "Set stat failed m_wIoState[%p] newstat[%p]", m_wIoState, newstat);
				continue;
			}
			break;
		}while(1);
	} 
	else
	{
		do
		{
			oldstat = m_wIoState;
			newstat = oldstat & ~iState;
			/**/
			if(!CAS(&m_wIoState, oldstat, newstat))
			{
				//Logger.Log(NOTIC, "Set stat failed m_wIoState[%p] newstat[%p]", m_wIoState, newstat);
				continue;
			}
			break;
			
		}while(1);
	}
#endif	
}

bool CIoHandler::IsIoState(uint16 iState, bool bSet /*= false*/)
{
#if 0
	CAutoLock oLock(m_oLockState.GetHandle());
	if (m_wIoState & iState)
	{
		return true;
	}
	else if (bSet)
	{
		m_wIoState |= iState;
	}

	return false;
#else
	uint16 oldstat = 0;
	uint16 newstat = 0;
	do
	{
		oldstat = m_wIoState;
		if (oldstat & iState)
		{
			return true;
		}
		else if (bSet)
		{
			newstat = oldstat | iState;
			if(!CAS(&m_wIoState, oldstat, newstat))
			{
				//Logger.Log(NOTIC, "Set stat failed m_wIoState[%p] newstat[%p]", m_wIoState, newstat);
				continue;
			}
			return false;
		}
		else
			return false;
	}while(1);
#endif	
}


int CIoHandler::SendPkg()
{
	int iErr = IO_FAILED;

	// send data
	while (m_iSockFd >= 0  && m_stSendBuffer.iSize < m_stSendBuffer.iTotal)
	{
		int iRet = send(m_iSockFd, m_stSendBuffer.cBuffer + m_stSendBuffer.iSize, m_stSendBuffer.iTotal - m_stSendBuffer.iSize, 0);
		if (iRet < 0)
		{		
			if (EAGAIN == errno)
			{			
				//buffer is full
				iErr = IO_EAGAIN;
				break;
			}

			Logger.Log(DEBUG, "socket:%d send error:%s", m_iSockFd, strerror(errno));
			iErr = IO_CLOSE;
			break;
		}
		else if(iRet == 0)
		{
		}
		else
		{
			iErr = IO_SUCCESS;
			m_stSendBuffer.iSize += iRet;
			if (m_stSendBuffer.iSize == m_stSendBuffer.iTotal)
			{
				m_stSendBuffer.iSize  = 0;
				m_stSendBuffer.iTotal = 0;
				break;
			}
		}
	}

	return iErr ;
}

int CIoHandler::RecvPkg()
{
	int iErr = IO_FAILED;

	// recv data
	while( m_iSockFd >= 0 && m_stRecvBuffer.iSize < MAX_BUFFER_SIZE )
	{
		int iRet = recv(m_iSockFd, m_stRecvBuffer.cBuffer + m_stRecvBuffer.iSize, MAX_BUFFER_SIZE - m_stRecvBuffer.iSize, 0);
		if(iRet == 0)
		{
			//Logger.Log(ERROR, "socket:%d session[%u] peer shutdown", m_iSockFd, m_dwSessionId);
			iErr = IO_CLOSE;
			break;
		}
		else if(iRet < 0) 
		{	
			int err = errno;
			if( EAGAIN == err )
			{
				iErr          = IO_EAGAIN;
				m_tActiveTime = time(NULL);
				break;
			}
			//Logger.Log(ERROR, "socket:[%d] session[%u] recv error[%d]:%s", m_iSockFd, m_dwSessionId, err, strerror(err));
			iErr = IO_CLOSE;
			break;
		} 
		else
		{
			iErr = IO_SUCCESS;
			m_stRecvBuffer.iSize += iRet;
			m_tActiveTime         = time(NULL);
		}
	}

	return iErr;
}
