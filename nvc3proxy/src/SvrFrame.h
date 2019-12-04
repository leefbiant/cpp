#ifndef _SVRFRAME_H
#define _SVRFRAME_H

#include "Common.h"
#include "Util.h"
#include "Config.h"
#include "User.h"
#include "Module.h"

class CSvrFrame : public ITimer
{
public:
	friend class CUserMgr;
	friend class CModuleMgr;
public:
	CSvrFrame();
	virtual ~CSvrFrame();

public:
	bool Init();
	int Run();
	void OnTimer(const uint32 dwID);
	void SigNotify(int signo);

public:
	static bool SetNonblock(int iSockFd, bool bIsNonBlock);
	static bool SetReuseAddr(int iSockFd, int iOption);
	static bool SetLinger(int iSockFd, int iOption);

protected:
	bool StartListen(int32& fd);
	bool ConnectTo(CUser* pModule, struct sockaddr_in& stTcpSvrAddr);
	int  AcceptNew(int32 fd);
	bool AddFd(int iSockFd, int iEvent, void* ptr = NULL);
	bool DelFd(int iSockFd);
	bool CloseFd(int& iSockFd);

	void SubmitTask(CIoHandler* pIoHandler, uint16 wFlag);

protected:
	static void IoHandle(void* pParam, uint16 wFlag);
	static void* IoEvent(void* pParam);

protected:
	CThreadPool				m_oReadThreadPool, m_oWriteThreadPool;
	CUserMgr				m_oUserMgr;
	CModuleMgr				m_oModuleMgr;

private:
	//for epoll
	int					m_iEpollFd;
	int					m_iListenFd;
	
	struct epoll_event* m_stEpollEvs;

	//for sys
	int					m_iCpuNum;
};

#endif
