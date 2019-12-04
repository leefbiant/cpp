#ifndef _STRUCT_H
#define _STRUCT_H

#include "Common.h"

typedef struct
{
	uint32	dwMid;
	uint16	wType;
	uint32	dwIp;
	uint16	wPort;
	uint32	dwOnline;
}MODULE_INFO_t;

class CMsgBase
{
public:
	uint8* msg;
	uint32 len;
	
public:
	CMsgBase():msg(NULL),len(0)
	{
	}
	~CMsgBase()
	{
		if( msg )
		{
			delete[] msg ;
			msg = NULL;
		}
	}
	static int NewMsgFromBuf(uint8* p , uint32& size, CMsgBase** pMsg)
	{
		*pMsg = new CMsgBase;
		(*pMsg)->msg = new uint8[size + 1];
		memcpy((*pMsg)->msg, p, size);
		(*pMsg)->len = size;
		size = 0;
		return 0;
	}

	static int GetBufFromMsg(uint8* p , uint32& size, CMsgBase* pMsg)
	{
		memcpy(p, pMsg->msg, pMsg->len);
		size = pMsg->len;
		return 0;
	}

	static void DelMsg(CMsgBase* pMsg)
	{
		delete pMsg;
		return ;
	}
	

 
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


#endif


