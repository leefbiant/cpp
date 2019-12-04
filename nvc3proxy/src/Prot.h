
#ifndef _PROT_H
#define _PROT_H

#include "Util.h"
#include "Common.h"
#include "base64.h"


typedef struct
{
	int32	sock;
	IO_TYPE	IoType;
	uint32	dwIp;
	uint16	wPort;
	uint32	session;
	FunCallback fCallback;
}MODULE_INFO_t;


class CModule;
class CMsgBase
{
public:
	CMsgBase();
	virtual ~CMsgBase();

	virtual CMsgBase* Clone();

public:
	static int NewMsgFromBuf(uint8* pBuf, int32 &dwSize, CMsgBase** ppOutMsg);
	static int GetBufFromMsg(uint8* pBuf, int32 &dwSize, CMsgBase* pInMsg);
	static CMsgBase* NewMsg(uint16 wCmd);
	static void DelMsg(CMsgBase* pMsg);

protected:
	virtual int Parse(uint8* pBuf, int32& dwSize);
	virtual int Pack(uint8* pBuf, int32& dwSize);
	void UpdateLen(uint8* pBuf, uint32 dwLen);
public:	
	void UpdateSession(uint32 Session);

	
public:
	//head
	uint32		m_wVer;			/*协议版本*/
	uint32		m_dwLen;		/*包长度，包含包头*/
	uint16		m_wSeq;			/*包序号*/
	uint16		m_wCmd;			/*命令字*/
	uint64		m_llSrcUid;		/*发送方ID*/
	uint64		m_llDesUid;		/*接收方ID*/
	uint32		m_Reserve;

	uint32		m_Session;
	uint8*      m_pmsg;

public:
	CModule*	m_pRoute;
};


class CMsgHttpResp : public CMsgBase
{
public:
	CMsgHttpResp();
	~CMsgHttpResp();
protected:
	virtual int Parse(uint8* pBuf, int32& dwSize);
	virtual int Pack(uint8* pBuf, int32& dwSize);	
public:
	string m_HttpResp;
};


class CMsgPong : public CMsgBase
{
public:
	CMsgPong();
	~CMsgPong();
protected:
	virtual int Parse(uint8* pBuf, int32& dwSize);
	virtual int Pack(uint8* pBuf, int32& dwSize);	
public:
	char m_pongbuf[4];
};


class CMsgModConn : public CMsgBase
{
public:
	CMsgModConn();
	~CMsgModConn();
protected:
	virtual int Parse(uint8* pBuf, int32& dwSize);
	virtual int Pack(uint8* pBuf, int32& dwSize);	
public:
	MODULE_INFO_t			m_ModuleInfo;
};
#endif

