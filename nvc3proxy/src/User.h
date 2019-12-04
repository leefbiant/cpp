#ifndef _USER_H
#define _USER_H

#include "IoHandler.h"
#include "Prot.h"


class CUserMgr;
class CUser : public CIoHandler
{
	friend class CUserMgr;
	
public:
	CUser(CUserMgr* pMgr);
	virtual ~CUser();
	void Clean();

public:
	virtual int 	Read();
	virtual int 	Write();

	virtual void	Reset();
	int 			Connect();
	int				SetP2pMsg(CMsgBase* pMsg);
	inline time_t	GetOnlineTime(){return time(NULL) - m_tOnlineTime;}
	inline uint32	GetSessionId(){return m_dwSessionId;}
	bool			IsTimeout(uint32 dwMaxTime);
	bool			IsValidMsg(CMsgBase* pMsg);
protected:
	virtual int 	HandlePkg(CMsgBase* pMsg);
	static  void 	ClearMsgQ(void* pVoid);
	
protected:
	CUserMgr*						m_pMgr;
	pthread_mutex_t  				m_mutex;
public:
	CObjectPooLockFree<CMsgBase>	m_oP2pMsgQ;
	time_t							m_tOnlineTime;
	bool							m_bconned;
	uint32 							m_mouleid;
	uint64 							m_uid;
};

class CSvrFrame;
class CModuleMgr;
class CUserMgr
{
	typedef map<uint64,CUser*>		MAPUSER; //uid 
	typedef map<uint64,MAPUSER*>	MAPSESSION; //modid
public:
	CUserMgr(CSvrFrame* pSvrFrame);
	virtual ~CUserMgr();

public:
	CSvrFrame*		GetSvrFrame(){return m_pSvrFrame;}
	CUser*			GetIdleUser();
	int 			Init();
	inline int		GetIdleUserNum(){return m_oIdleUserQ.GetNum();}
	void			CheckTimeout(uint32 dwMaxTime);
	int 			AddUser(CUser* pUser);
	int 			SetIdleUser(CUser* pUser);
	int 			DispatchMsg();
	void 			SetMsg(CMsgBase* pMsg);
	void			SubmitTask(CIoHandler* pIoHandler, uint16 wFlag);
	void 			SetModMgrMsg(CMsgBase* pMsg);
	CModule* 		GetModule(uint32 modid);
private:
	
	int 			HandlePkg(CMsgBase* pMsg);
	int 			HandleCloseUser(CMsgBase* pMsg);
	int 			HandleUserLogin(CMsgBase* pMsg);
	int 			HandleProxyMsg(CMsgBase* pMsg);
	
	static void* 	Callback(void* pParam);	
	int 			DelUser(CMsgBase* pMsg);
	void 			DelSessionUser(MAPUSER* pModUserMap, uint32 session, bool bmod = false);
	void 			Stat();
private:
	CSvrFrame*				m_pSvrFrame;
	CObjectPool<CUser>		m_oIdleUserQ;
	CSem					m_oSem;
	MAPSESSION				m_mapServerUser;
	CObjectPool<CMsgBase>	m_oMsgQ;
};

#endif
