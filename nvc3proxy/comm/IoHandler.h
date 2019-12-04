#ifndef _IOHANDLER_H
#define _IOHANDLER_H

#include "Common.h"
#include "Util.h"

struct BUFFER_t
{
	uint32	iSize;
	uint32	iTotal;
	uint8	cBuffer[MAX_BUFFER_SIZE];
};

class CIoHandler : public CTask
{
public:
	CIoHandler();
	virtual ~CIoHandler();

public:
	void SetIoState(uint16 iState, bool bSet);
	bool IsIoState(uint16 iState, bool bSet = false);
	uint16 GetIOStat(){return m_wIoState;};
	virtual void Reset();
	virtual void Close(int iErrCode);
	virtual int Read()  = 0;
	virtual int Write() = 0;

protected:
	int SendPkg();
	int RecvPkg();

public:
	int			m_iSockFd;	
	IO_TYPE		m_eIoType;
	uint32		m_dwIp;
	uint16		m_wPort;
	uint32		m_dwSessionId;
	uint16		m_wIoState;
protected:
	BUFFER_t	m_stSendBuffer, m_stRecvBuffer;
	time_t		m_tActiveTime;

private:
	
	CLock		m_oLockState;
};

class CListener : public CIoHandler
{
public:
	CListener(){}
	virtual ~CListener(){}

public:
	virtual int Read(){return IO_SUCCESS;}
	virtual int Write(){return IO_SUCCESS;}
};

#endif
