#include "Prot.h"
#include "Log.h"

#include "Common.h"


CMsgBase::CMsgBase():m_pmsg(NULL)
{
	m_wVer     = PROTOCOL_VERSION_0x1001;
	m_dwLen    = PROTOCOL_HEAD_LEN;
	m_wSeq     = 0;
	m_wCmd	   = CMD_UNKNOW;
	m_llSrcUid = 0;
	m_llDesUid = 0;
	m_Reserve = 0;

	m_pRoute = NULL;
	m_Session = 0;
}

CMsgBase::~CMsgBase()
{
	if( m_pmsg )
	{
		delete[] m_pmsg;
		m_pmsg = NULL;
	}
}

CMsgBase* CMsgBase::Clone()
{
	return NULL;
}

int CMsgBase::NewMsgFromBuf(uint8* pBuf, int32 &dwSize, CMsgBase** ppOutMsg)
{
	if (NULL == pBuf || dwSize <= 0)
	{
		Logger.Log(ERROR, "new msg from buf failed, bad parameter! dwSize[%d]",dwSize);
		return ERR_FAILED;
	}

	uint8* p = pBuf;

	if(dwSize < (int32)PROTOCOL_HEAD_LEN)return ERR_NO_MORE_DATA;
	
	uint32 wVer		= ntohl(*((uint32*)p));
	p+=4;
	uint32 dwLen	= ntohl(*((uint32*)p));
	p+=4;
	//uint16 wSeq	= ntohs(*((uint16*)p));
	p+=2;
	uint16 wCmd		= ntohs(*((uint16*)p));
	p+=2;


	switch(wVer)
	{
		case PROTOCOL_VERSION_0x1001:
		case PROTOCOL_VERSION_0x2001:
		case PROTOCOL_VERSION_0x2002:
		case PROTOCOL_VERSION_0x2003:
		case PROTOCOL_VERSION_0x2004:
		case PROTOCOL_VERSION_0x2005:
		
			break;
		default:
			Logger.Log(ERROR, "parse data, the ver:0x%04x is error! cmd:0x%04x", wVer, wCmd);
			return ERR_PROTOCOL;
			break;
	}


	if(dwSize < (int32)dwLen) 
	{
		Logger.Log(ERROR, "parse data, the buffer size:%d is less than the msg size:%d, cmd:0x%04x",dwSize, dwLen, wCmd);
		return ERR_NO_MORE_DATA;
	}

	//Logger.Log(ERROR, "parse data, cmd:0x%04x, len:%d, size:%d", wCmd, dwLen, dwSize);
	*ppOutMsg = new CMsgBase;

	int iErr = (*ppOutMsg)->Parse(pBuf, dwSize);
	if(ERR_SUCCESS != iErr)
	{
		Logger.Log(ERROR, "Msg parse failed, cmd:0x%04x", wCmd);
		CMsgBase::DelMsg(*ppOutMsg);
		*ppOutMsg = NULL;
		return iErr;
	}

	return ERR_SUCCESS;
	
}

int CMsgBase::GetBufFromMsg(uint8* pBuf, int32 &dwSize, CMsgBase* pInMsg)
{
	if (NULL == pBuf || NULL == pInMsg)
	{
		Logger.Log(ERROR, "get buf from msg failed, bad parameter!");
		return ERR_FAILED;
	}

	switch(pInMsg->m_wVer)
	{
		case PROTOCOL_VERSION_0x1001:
		case PROTOCOL_VERSION_0x2001:
		case PROTOCOL_VERSION_0x2002:
		case PROTOCOL_VERSION_0x2003:
		case PROTOCOL_VERSION_0x2004:
		case PROTOCOL_VERSION_0x2005:
			break;
		default:
			Logger.Log(ERROR, "GetBufFromMsg, the ver:0x%04x is error! cmd:[0x%04x] m_llDesUid:%lld m_llSrcUid:%lld", 
				pInMsg->m_wVer, pInMsg->m_wCmd, pInMsg->m_llSrcUid, pInMsg->m_llDesUid);
			return ERR_FAILED;
			break;
	}

	if(dwSize < (int32)pInMsg->m_dwLen)
	{
		Logger.Log(DEBUG, "pack data, the buffer size:%d is less than the msg size:%d, cmd:[0x%04x]  m_llSrcUid:%lld m_llDesUid:%lld ",\
			dwSize, pInMsg->m_dwLen, pInMsg->m_wCmd, pInMsg->m_llSrcUid, pInMsg->m_llDesUid);	

		return ERR_NO_MORE_SPACE;
	}

	return pInMsg->Pack(pBuf, dwSize);
}

CMsgBase* CMsgBase::NewMsg(uint16 wCmd)
{
	CMsgBase * pMsg = NULL;

	uint16 wBaseCmd =  wCmd  &  (~CMD_INNER_FLAGE);
	switch(wBaseCmd)
	{
	case CMD_SYS_CLOSE_USER:
		pMsg = new CMsgBase();
		break;
	case CMD_SYS_CLOSE_MOD:
		pMsg = new CMsgBase();
		break;	
	case CMD_WEBUSER_HTTPRESP:
		pMsg = new CMsgHttpResp();
		break;	
	case CMD_WEBUSER_PONG:
		pMsg = new CMsgPong();
		break;	
	case CMD_SYS_MOD_CONN:
		pMsg = new CMsgModConn();
		break;
	case CMD_SYS_MOD_CLOSE_USER:
		pMsg = new CMsgBase();
		break;
	case CMD_SYS_SVR_CLOSE_USER:
		pMsg = new CMsgBase();
		break;	
	case CMD_USER_LOGIN:
		pMsg = new CMsgBase();
		break;
	default:
		return NULL;
	}

	pMsg->m_wCmd = wCmd;

	return pMsg;
}

void CMsgBase::DelMsg(CMsgBase* pMsg)
{
	if(NULL == pMsg)
		return;
	
	delete pMsg;
	pMsg = NULL;
	
}

int CMsgBase::Parse(uint8* pBuf, int32 &dwSize)
{
	uint8* p   = pBuf;
	m_wVer	   = ntohl(*((uint32*)p));
	p += 4;
	m_dwLen	   = ntohl(*((uint32*)p));
	p += 4;
	m_wSeq	   = ntohs(*((uint16*)p));
	p += 2;
	m_wCmd	   = ntohs(*((uint16*)p));
	p += 2;
	m_llSrcUid = ntohll(*((uint64*)p));
	p += 8;
	m_llDesUid = ntohll(*((uint64*)p));
	p += 8;

	m_Reserve = ntohl(*((uint32*)p));
	p += 4;
	
	if( m_dwLen  > 0 )
	{
		m_pmsg = new uint8[m_dwLen+1];
		memcpy( m_pmsg, p, m_dwLen-PROTOCOL_HEAD_LEN);
		p+= (m_dwLen-PROTOCOL_HEAD_LEN);
	}

	dwSize = (int32)(p - pBuf );
	
	return ERR_SUCCESS;
}

int CMsgBase::Pack(uint8* pBuf, int32 &dwSize)
{
	uint8* p      = pBuf;
	*((uint32*)p) = htonl(m_wVer);
	p += 4;
	*((uint32*)p) = htonl(m_dwLen);
	p += 4;
	*((uint16*)p) = htons(m_wSeq);
	p += 2;
	*((uint16*)p) = htons(m_wCmd);
	p += 2;
	*((uint64*)p) = htonll(m_llSrcUid);
	p += 8;
	*((uint64*)p) = htonll(m_llDesUid);
	p += 8;

	*((uint32*)p) = htonl(m_Reserve);
	p += 4;	

	dwSize = (int32)(p - pBuf);
	if( m_dwLen-PROTOCOL_HEAD_LEN > 0 )
	{	
		memcpy(p, m_pmsg, m_dwLen-PROTOCOL_HEAD_LEN);
		p += (m_dwLen-PROTOCOL_HEAD_LEN);
		dwSize = (int32)(p - pBuf);
	}
	UpdateLen(pBuf, dwSize);
	
	return ERR_SUCCESS;
}
void CMsgBase::UpdateLen(uint8* pBuf, uint32 dwLen)
{
	*((uint32*)(pBuf + 4)) = htonl(dwLen);
}

void CMsgBase::UpdateSession(uint32 Session)
{
	m_Reserve = Session;
}


CMsgHttpResp::CMsgHttpResp()
{
}
CMsgHttpResp::~CMsgHttpResp()
{
}
int CMsgHttpResp::Parse(uint8* pBuf, int32& dwSize)
{
	return 0;
}
int CMsgHttpResp::Pack(uint8* pBuf, int32& dwSize)
{
	uint8* p = pBuf;
	if( (int32)m_HttpResp.length() > dwSize )
	{
		return ERR_NO_MORE_DATA;
	}
	memcpy(p, m_HttpResp.c_str(), m_HttpResp.length());
	
	dwSize = m_HttpResp.length();  
	m_dwLen = dwSize;
	return ERR_SUCCESS;
}

CMsgPong::CMsgPong()
{
	memset(m_pongbuf, 0x00, sizeof(m_pongbuf));
}

CMsgPong::~CMsgPong()
{
	
}

int CMsgPong::Parse(uint8* pBuf, int32& dwSize)
{
	return ERR_SUCCESS;
}

int CMsgPong::Pack(uint8* pBuf, int32& dwSize)
{
	dwSize = 0;  
	return ERR_SUCCESS;
}


CMsgModConn::CMsgModConn()
{
	
}

CMsgModConn::~CMsgModConn()
{
	
}

int CMsgModConn::Parse(uint8* pBuf, int32& dwSize)
{
	return ERR_SUCCESS;
}

int CMsgModConn::Pack(uint8* pBuf, int32& dwSize)
{
	dwSize = 0;  
	return ERR_SUCCESS;
}
