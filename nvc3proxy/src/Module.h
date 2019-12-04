#ifndef _MODULE_H
#define _MODULE_H

#include "IoHandler.h"
#include "Prot.h"
#include "websocket.h"

class CModuleMgr;

class CModule : public CIoHandler
{
	friend class CModuleMgr;

public:
	CModule(CModuleMgr* pMgr, MODULE_TYPE mt);
	virtual ~CModule();

public:
	int Read();
	int Write();

	void			Reset();
	void			Reset2();
	int				SetModuleMsg(CMsgBase* pMsg);
	bool			IsTimeout(uint32 dwMaxTime);

public:
	void HandleHeartbeat();
	
protected:
	virtual int HandlePkg(CMsgBase* pMsg);
	static void ClearMsgQ(void* pVoid);
	int handshake(int& dwRead, uint8* p,  int32& dwSize);
	int WS_Pack(CMsgBase* pMsg, char* pMsgbuf, uint64 Msglen);
protected:
	CModuleMgr*				m_pMgr;

private:
	CObjectPool<CMsgBase>		m_oModuleMsgQ;
	Handshake	m_Handshake;
	Frame		m_Frame;
};

class CSvrFrame;
class CModuleMgr
{
	typedef map<uint32,CModule*>		MAPMODULE;//mid or fd


public:
	CModuleMgr(CSvrFrame* pSvrFrame);
	virtual ~CModuleMgr();

public:	
	CModule*	GetIdleModule();
	int			SetIdleModule(CModule* pModule);
	inline int	GetIdleModuleNum(){return m_oIdleModuleQ.GetNum();}
	
	int		AddModule(CModule* pModule);
	int		DelModule(CModule* pModule);
	CModule* GetModuleByMid(uint32 dwMid);

	void	SubmitTask(CIoHandler* pIoHandler, uint16 wFlag);
	void	SetMsg(CMsgBase* pMsg);
	int		DispatchMsg();
	void 	Stat();
	void	CheckTimeout(uint32 dwMaxTime);
	void	Statistics();
	void	OnTimer(const uint32 dwID);

protected:
	void 	SetUserMgrMsg(CMsgBase* pMsg);
	int		HandlePkg(CMsgBase* pMsg);

private:	
	int 	HandleModConn(CMsgBase* pMsg);
	int 	HandleCloseModUser(CMsgBase* pMsg);
	int 	HandleModCloseUser(CMsgBase* pMsg);
	int 	HandleErrModUser(CMsgBase* pMsg);
private:
	CSvrFrame*				m_pSvrFrame;	

	CObjectPool<CModule>	m_oIdleModuleQ;
	CObjectPool<CMsgBase>	m_oMsgQ;
	CSem					m_oSem;

	uint32					m_dwCreateModuleNums;
	MAPMODULE				m_mapAuthModule;

};

#endif
