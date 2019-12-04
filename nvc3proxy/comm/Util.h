#ifndef _UTIL_H
#define _UTIL_H

#include "Common.h"
#include <sys/syscall.h> 
#include <zlib.h>
class CLock
{
public:
	CLock()
	{
		pthread_mutex_init(&m_mutex, NULL);
	}

	~CLock()
	{
		pthread_mutex_destroy(&m_mutex);
	}

public:
	inline void Lock()
	{
		pthread_mutex_lock(&m_mutex);
	}

	inline void UnLock()
	{
		pthread_mutex_unlock(&m_mutex);
	}

	inline pthread_mutex_t* GetHandle()
	{
		return &m_mutex;
	}

private:
	pthread_mutex_t m_mutex;
};

class CAutoLock
{
public:
	CAutoLock(pthread_mutex_t* mutex)
	{
		m_mutex = mutex;
		pthread_mutex_lock(m_mutex);
	}
	~CAutoLock()
	{
		pthread_mutex_unlock(m_mutex);
	}

private:
	pthread_mutex_t* m_mutex;
};

//////////////////////////////////////////////////////////////////////////
class ITimer
{
public:
	ITimer(){};
	virtual ~ITimer(){};

	virtual void OnTimer(const uint32 dwID) = 0;
};

class CTimer
{	
	typedef struct
	{
		ITimer*		pTimer;
		uint32		dwID;
		bool		bInto;
		timer_t		tid;
	}TIMER_INFO_t;

protected:
	CTimer();
public:
	virtual ~CTimer();

public:
	static CTimer* GetInstance();
	int SetTimer(ITimer* pTimer, uint32 dwID, uint32 dwSecond);
	void KillTimer(uint32 dwID);

protected:
	static void  OnHandle(union sigval v);

private:
	map<uint32, TIMER_INFO_t*>	m_mapTimers;
	static CTimer*				m_pInstance;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////
#define STR_LEN  50

//生成查询IP信息的类型
#define  QueryType1    1
#define  QueryType2    2

//IP记录由country、area两部分组成
typedef struct IPINFO
{
	char Province[STR_LEN];
	char city[STR_LEN];
	char Operators[STR_LEN];

}IPINFO;


class CIPQuery  
{

public:
	CIPQuery();
	virtual ~CIPQuery();

	// 功能：将IP库中的所有数据读入全局数组data
	// 参数：file 文件字符串
	// 返回值： 0 成功 
	int init(const char* file);

	// 根据IP生成查询信息
	int getqueryinfo(const char* ip,char* infoout,const int querytype=QueryType1);

	//查询IP地址
	int queryip(const char* ip,IPINFO &infoout);

private:
	char* m_data;
};

//////////////////////////////////////////////////////////////////////////
#ifndef CFG_OBJECT_POOL
template<typename T>
class CObjectPool
{
public:
	typedef list<T*> Elements;
	typedef typename list<T*>::iterator Iterator;
public:
	CObjectPool();
	virtual ~CObjectPool();

public:
	int		Init(int iMaxSize);
	void	Clear();
	int		SetObject(T* pInObj);
	int		GetObject(T** ppOutObj);
	
	int		GetNum();	

private:
	Elements			m_stObjectQ;
	//pthread_mutex_t		m_Lock;
	CLock				m_oLock;
	uint32				m_dwMaxSize;
	uint32				m_dwMySize;
};

template<typename T>
CObjectPool<T>::CObjectPool()
{
	m_dwMaxSize = 0;
	m_dwMySize  = 0;
	//pthread_mutex_init(&m_Lock,NULL);
}

template<typename T>
CObjectPool<T>::~CObjectPool()
{
	Clear();
	//pthread_mutex_destroy(&m_Lock);
}

template<typename T>
void CObjectPool<T>::Clear()
{
	//CAutoLock lock(&m_Lock);
	CAutoLock lock(m_oLock.GetHandle());
	T* pObject = NULL;

	Iterator it = m_stObjectQ.begin();
	for(; it!= m_stObjectQ.end(); it++)
	{	
		pObject = (T*)*it;
		delete pObject;
		pObject = NULL;
	}

	m_stObjectQ.clear();

}

template<typename T>
int CObjectPool<T>::Init(int iMaxSize)
{
	Clear();
	m_dwMaxSize = iMaxSize;
	m_dwMySize  = 0;
	return ERR_SUCCESS;
}

template<typename T>
int CObjectPool<T>::SetObject(T* pInObj)
{
	if(pInObj == NULL)
		return ERR_FAILED;

	//CAutoLock lock(&m_Lock);
	CAutoLock lock(m_oLock.GetHandle());
	//if(m_dwMySize >= m_dwMaxSize)
	//if(m_stObjectQ.size() >= m_dwMaxSize)
	//{
	//	Iterator it = m_stObjectQ.begin();
	//	T* p = (T*)*it;
	//	delete p;
	//	m_stObjectQ.erase(it);
	//	//	return ERR_NO_MORE_SPACE;
	//}
	m_stObjectQ.push_back(pInObj);
	m_dwMySize++;

	return ERR_SUCCESS;
}

template<typename T>
int CObjectPool<T>::GetObject(T** ppOutObj)
{
	//CAutoLock lock(&m_Lock);
	CAutoLock lock(m_oLock.GetHandle());
	Iterator it = m_stObjectQ.begin();
	if(m_stObjectQ.end() == it)
		return ERR_FAILED;

	*ppOutObj = (T*)*it;
	m_stObjectQ.erase(it);
	m_dwMySize--;

	return ERR_SUCCESS;
}

template<typename T>
int CObjectPool<T>::GetNum()
{
	//CAutoLock lock(&m_Lock);
	CAutoLock lock(m_oLock.GetHandle());
	//return m_stObjectQ.size();
	return m_dwMySize;
}

#else

typedef void (*CallbackDel)(void* pVoid);
template<typename T>
class CObjectPool
{
	struct Node_t
	{
		T*		pData;
		Node_t*	pNext;
	};

public:
	CObjectPool(CallbackDel cb = NULL);
	virtual ~CObjectPool();

public:
	int		Init(int iMaxSize);
	void	Clear();
	int		SetObject(T* pInObj);
	int		GetObject(T** ppOutObj);
	int 	GetObjectAll(vector<T*>& ppOutObjList);
	T* 		GetObjectByPos(uint32 pos);
	int		GetNum();	

private:
	CallbackDel	m_cbDel;
	Node_t *m_pHead, *m_pTail, *m_pIdle;
	CLock		m_oLock;
	uint32		m_dwMaxSize;
	uint32		m_dwMySize;
};

template<typename T>
CObjectPool<T>::CObjectPool(CallbackDel cb /*= NULL*/):
m_cbDel(cb)
{
	m_pHead		= NULL;
	m_pTail		= NULL;
	m_pIdle		= NULL;
	m_dwMaxSize = 0;
	m_dwMySize  = 0;
}

template<typename T>
CObjectPool<T>::~CObjectPool()
{
	Clear();
}

template<typename T>
void CObjectPool<T>::Clear()
{
	CAutoLock lock(m_oLock.GetHandle());
	while(NULL != m_pHead)
	{
		Node_t* pCur = m_pHead;
		m_pHead = m_pHead->pNext;
		if (NULL == m_cbDel)
		{
		delete pCur->pData;
		} 
		else
		{
			m_cbDel(pCur->pData);
		}
		delete pCur;
	}

	while(NULL != m_pIdle)
	{
		Node_t* pCur = m_pIdle;
		m_pIdle = m_pIdle->pNext;
		delete pCur;
	}

	m_pHead = m_pTail = m_pIdle = NULL;
	m_dwMySize = 0;
}

template<typename T>
int CObjectPool<T>::Init(int iMaxSize)
{
	Clear();
	m_dwMaxSize = iMaxSize;
	m_dwMySize  = 0;
	return ERR_SUCCESS;
}

template<typename T>
int CObjectPool<T>::SetObject(T* pInObj)
{
	if(pInObj == NULL)
		return ERR_FAILED;

	CAutoLock lock(m_oLock.GetHandle());
	/*if(m_dwMaxSize > 0 && m_dwMySize >= m_dwMaxSize)
	{
		Node_t* pCur   = m_pHead;
		m_pHead        = m_pHead->pNext;
		delete pCur->pData;
		pCur->pData    = pInObj;
		pCur->pNext    = NULL;
		m_pTail->pNext = pCur;
		m_pTail        = pCur;

		return ERR_SUCCESS;
	}*/

	Node_t* pCur = NULL;
	if (NULL == m_pIdle)
	{
		pCur = new Node_t();
	} 
	else
	{
		pCur = m_pIdle;
		m_pIdle = m_pIdle->pNext;
	}
	pCur->pData  = pInObj;
	pCur->pNext  = NULL;

	if (NULL == m_pTail)
	{
		m_pHead = m_pTail = pCur;
	} 
	else
	{
		m_pTail->pNext = pCur;
		m_pTail = pCur;
	}
	m_dwMySize++;

	return ERR_SUCCESS;
}

template<typename T>
int CObjectPool<T>::GetObject(T** ppOutObj)
{
	CAutoLock lock(m_oLock.GetHandle());
	if(NULL == m_pHead)
		return ERR_FAILED;

	*ppOutObj = (T*)m_pHead->pData;
	Node_t* pCur = m_pHead;
	if (m_pHead == m_pTail)
	{
		m_pHead = m_pTail = NULL;
	} 
	else
	{		
		m_pHead = m_pHead->pNext;
	}
	m_dwMySize--;

	pCur->pData = NULL;
	pCur->pNext = NULL;
	if (NULL == m_pIdle)
	{
		m_pIdle = pCur;
	} 
	else
	{
		pCur->pNext = m_pIdle;
		m_pIdle = pCur;
	}

	return ERR_SUCCESS;
}
template<typename T>
int CObjectPool<T>::GetObjectAll(vector<T*>& ppOutObjList)
{
	CAutoLock lock(m_oLock.GetHandle());
	if(NULL == m_pHead)
		return ERR_FAILED;

	while( NULL != m_pHead)
	{
		ppOutObjList.push_back((T*)m_pHead->pData);
		Node_t* pCur = m_pHead;
		if (m_pHead == m_pTail)
		{
			m_pHead = m_pTail = NULL;
		} 
		else
		{		
			m_pHead = m_pHead->pNext;
		}
		m_dwMySize--;

		pCur->pData = NULL;
		pCur->pNext = NULL;
		if (NULL == m_pIdle)
		{
			m_pIdle = pCur;
		} 
		else
		{
			pCur->pNext = m_pIdle;
			m_pIdle = pCur;
		}
	}
	return ERR_SUCCESS;
}

template<typename T>
T* CObjectPool<T>::GetObjectByPos(uint32  pos)
{
	CAutoLock lock(m_oLock.GetHandle());
	if( pos >  m_dwMySize )return NULL;
	Node_t* pCur = m_pHead;
	while( --pos > 0  )pCur = pCur->pNext;
	
	T* ppOutObj = (T*)pCur->pData;
	return ppOutObj;
}


template<typename T>
int CObjectPool<T>::GetNum()
{
	//CAutoLock lock(m_oLock.GetHandle());
	return m_dwMySize;
}

template<typename T>
class CObjectPooLockFree
{
	struct Node_t
	{
		T*		pData;
		Node_t*	pNext;
	};

public:
	CObjectPooLockFree(CallbackDel cb = NULL);
	virtual ~CObjectPooLockFree();

public:
	int		Init(int iMaxSize);
	void	Clear();
	int		SetObject(T* pInObj);
	int		GetObject(T** ppOutObj);
	int 	GetObjectAll(vector<T*>& ppOutObjList);
	T* 		GetObjectByPos(uint32 pos);
	int		GetNum();	

private:
	CallbackDel	m_cbDel;
	Node_t *m_pHead, *m_pTail;
	uint32		m_dwMaxSize;
	uint32		m_dwMySize;
};

template<typename T>
CObjectPooLockFree<T>::CObjectPooLockFree(CallbackDel cb /*= NULL*/):
m_cbDel(cb)
{
	m_pHead		= m_pTail = new Node_t();
	m_pHead->pNext = NULL;
	m_dwMaxSize = 0;
	m_dwMySize  = 0;
}

template<typename T>
CObjectPooLockFree<T>::~CObjectPooLockFree()
{
	Clear();
	delete m_pHead;
}

template<typename T>
void CObjectPooLockFree<T>::Clear()
{
	while(m_pTail != m_pHead)
	{
		Node_t* pCur = m_pHead;
		m_pHead = m_pHead->pNext;
		if (NULL == m_cbDel)
		{
		delete pCur->pData;
		} 
		else
		{
			m_cbDel(pCur->pData);
		}
		delete pCur;
	}
	m_dwMySize = 0;
}

template<typename T>
int CObjectPooLockFree<T>::Init(int iMaxSize)
{
	Clear();
	m_dwMaxSize = iMaxSize;
	m_dwMySize  = 0;
	return ERR_SUCCESS;
}

template<typename T>
int CObjectPooLockFree<T>::SetObject(T* pInObj)
{
	if(pInObj == NULL)
		return ERR_FAILED;

	Node_t* pCur = NULL;
	pCur = new Node_t();
	pCur->pData  = NULL;
	pCur->pNext  = NULL;

	m_pTail->pNext = pCur;
	m_pTail->pData = pInObj;
	m_pTail = pCur;

	__sync_fetch_and_add(&m_dwMySize,1);

	return ERR_SUCCESS;
}

template<typename T>
int CObjectPooLockFree<T>::GetObject(T** ppOutObj)
{
	if(m_pTail == m_pHead)
		return ERR_FAILED;
		
	Node_t* pCur = m_pHead;
	*ppOutObj = (T*)pCur->pData;
	m_pHead = pCur->pNext;
	__sync_sub_and_fetch(&m_dwMySize,1);
	delete pCur;
	return ERR_SUCCESS;
}

template<typename T>
int CObjectPooLockFree<T>::GetObjectAll(vector<T*>& ppOutObjList)
{
	if(m_pTail == m_pHead)
		return ERR_FAILED;
		
	while( m_pTail != m_pHead)
	{
		Node_t* pCur = m_pHead;
		ppOutObjList.push_back((T*)pCur->pData);
		m_pHead = pCur->pNext;
		__sync_sub_and_fetch(&m_dwMySize,1);
		delete pCur;
	}
	return ERR_SUCCESS;
}

template<typename T>
T* CObjectPooLockFree<T>::GetObjectByPos(uint32  pos)
{
	if( 0 == pos || NULL == m_pHead || pos >  m_dwMySize )
	{
		return NULL;
	}
	Node_t* pCur = m_pHead;
	while( --pos > 0 &&  pCur )
	{
		pCur = pCur->pNext;
	}
	if( pCur )
	{
		T* ppOutObj = (T*)pCur->pData;
		return ppOutObj;
	}
	return NULL;
}


template<typename T>
int CObjectPooLockFree<T>::GetNum()
{
	return m_dwMySize;
}
#endif


//////////////////////////////////////////////////////////////////////////
class CSem
{
public:
	CSem();
	~CSem();

public:
	int Wait();
	int timedwait();
	int Post();
	int  GetValue();

private:
	sem_t *m_sem;
};

//////////////////////////////////////////////////////////////////////////
class CTask
{
public:
	CTask(){};
	virtual ~CTask(){};

public:
	FunCallback m_fCallback;
};

//////////////////////////////////////////////////////////////////////////
class CThreadPool;
class CThread
{
public:
	CThread(CThreadPool* pThreadPool);
	virtual ~CThread();

public:
	bool Start();
	bool Stop();

	void AddTask(CTask* pTask);

protected:
	int Run();
	static void* Callback(void* pParam);

private:
	pthread_t		m_threadid;
	CThreadPool*	m_pThreadPool;
	CTask*			m_pTask;
	CSem			m_oSem;
	bool			m_bExit;
};

//////////////////////////////////////////////////////////////////////////
class CThreadPool
{
public:
	CThreadPool();
	virtual ~CThreadPool();

public:
	bool	Init(uint32 dwThreadNum, uint16 wFlag);
	uint16	GetFlag(){return m_wFlag;}
	void	PushThread(CThread* pThread);
	void	SubmitTask(CTask* pTask);
	void	CancelTask(uint16 iState);
	int		GetTaskCount(){return m_oTaskQ.GetNum();}
	int		GetThreadCount(){return m_oThreadQ.GetNum();}

private:
	uint16					m_wFlag;
	CObjectPool<CThread>	m_oThreadQ;
	CObjectPool<CTask>		m_oTaskQ;
};

char* intip2char(int intip);
int	LoadFilterKeyWords(string& strFilterWords);
int	FilterKeyWords(uint8* pKeyWords, uint32 dwKeyLen, uint8* pContent, uint32 pContentLen);

string GetMyCol(char *Datain,char *Names );


inline string getCmdDesc(uint16 wCmd)
{
    switch(wCmd & ~CMD_INNER_FLAGE)
    {
		case CMD_USER_LOGIN:  //用户登录
			return "CMD_USER_LOGIN";
		case CMD_USER_LOGIN_ACK:  //
			return "CMD_USER_LOGIN_ACK";
		case CMD_USER_LOGOUT:  //用户注销
			return "CMD_USER_LOGOUT";
		case CMD_USER_LOGOUT_ACK:  //
			return "CMD_USER_LOGOUT_ACK";
		case CMD_USER_FORCE_DOWN:  //==用户被迫下线通知
			return "CMD_USER_FORCE_DOWN";
		case CMD_ROOM_NTF_USER_REFRESH:  //==房间内用户信息更新通知
			return "CMD_ROOM_NTF_USER_REFRESH";
		case CMD_USER_VOD_ACCEPT:  //主播点歌确认
			return "CMD_USER_VOD_ACCEPT";
		case CMD_USER_VOD_ACCEPT_ACK:  //
			return "CMD_USER_VOD_ACCEPT_ACK";
		case CMD_USER_HEARTBEAT:  //客户端心跳
			return "CMD_USER_HEARTBEAT";
		case CMD_USER_HEARTBEAT_ACK:  //
			return "CMD_USER_HEARTBEAT_ACK";
		case CMD_USER_MSG_SEND:  //用户发房间消息
			return "CMD_USER_MSG_SEND";
		case CMD_USER_MSG_SEND_ACK:  //
			return "CMD_USER_MSG_SEND_ACK";
		case CMD_USER_MSG_RECV:  //==用户接收消息
			return "CMD_USER_MSG_RECV";
		case CMD_USER_REFRESH:	//刷新用户信息
			return "CMD_USER_REFRESH";
		case CMD_USER_REFRESH_ACK:	//
			return "CMD_USER_REFRESH_ACK";
		case CMD_USER_VOD_RECV:  //==主播收到用户点歌请求
			return "CMD_USER_VOD_RECV";
		case CMD_USER_VOD_ACCEPT_NTF:  //==主播确认通知
			return "CMD_USER_VOD_ACCEPT_NTF";
		case CMD_USER_WHISPER_SEND:  //用户发送悄悄话
			return "CMD_USER_WHISPER_SEND";
		case CMD_USER_WHISPER_SEND_ACK:  //
			return "CMD_USER_WHISPER_SEND_ACK";
		case CMD_ROOM_LOGIN:  //进入房间
			return "CMD_ROOM_LOGIN";
		case CMD_ROOM_LOGIN_ACK:  //
			return "CMD_ROOM_LOGIN_ACK";
		case CMD_ROOM_LOGOUT:  //退出房间
			return "CMD_ROOM_LOGOUT";
		case CMD_ROOM_LOGOUT_ACK:  //
			return "CMD_ROOM_LOGOUT_ACK";
		case CMD_ROOM_GET_USER_LISTINFO:  //拉取房间用户信息列表
			return "CMD_ROOM_GET_USER_LISTINFO";
		case CMD_ROOM_GET_USER_LISTINFO_ACK:  //
			return "CMD_ROOM_GET_USER_LISTINFO_ACK";
		case CMD_ROOM_STAT_REFRESH:  //房间直播状态更新
			return "CMD_ROOM_STAT_REFRESH";
		case CMD_ROOM_STAT_REFRESH_ACK:  //
			return "CMD_ROOM_STAT_REFRESH_ACK";
		case CMD_ROOM_NTF_USERLIST_CHG:  //==用户登录房间通知
			return "CMD_ROOM_NTF_USERLIST_CHG";
		case CMD_ROOM_LOCK_NTF:  //==锁定房间通知
			return "CMD_ROOM_LOCK_NTF";
		case CMD_ROOM_NTF_STAT_REFRESH:  //==房间直播状态更新通知
			return "CMD_ROOM_NTF_STAT_REFRESH";
		case CMD_ROOM_BAN_USER_MSG:  //禁止用户发言
			return "CMD_ROOM_BAN_USER_MSG";
		case CMD_ROOM_BAN_USER_MSG_ACK:  //
			return "CMD_ROOM_BAN_USER_MSG_ACK";
		case CMD_ROOM_BAN_USER_NTF:  //==禁止用户发言通知
			return "CMD_ROOM_BAN_USER_NTF";
		case CMD_ROOM_KICKOFF_USER:  //踢出房间
			return "CMD_ROOM_KICKOFF_USER";
		case CMD_ROOM_KICKOFF_USER_ACK:  //
			return "CMD_ROOM_KICKOFF_USER_ACK";
		case CMD_ROOM_KICKOFF_USER_NTF:  //==踢出房间通知
			return "CMD_ROOM_KICKOFF_USER_NTF";
		case CMD_GET_ROOM_ATTR: //拉取房间属性
			return "CMD_GET_ROOM_ATTR";
		case CMD_GET_ROOM_ATTR_ACK: //
			return "CMD_GET_ROOM_ATTR_ACK";
		case CMD_CS_GET_ROOM_ANNC:	//	获取房间公告
			return "CMD_CS_GET_ROOM_ANNC";
		case CMD_CS_GET_ROOM_ANNC_ACK:	//	
			return "CMD_CS_GET_ROOM_ANNC_ACK";
		case CMD_CS_GET_FILTERWORDS:	//拉取敏感字列表
			return "CMD_CS_GET_FILTERWORDS";
		case CMD_CS_GET_FILTERWORDS_ACK:	//
			return "CMD_CS_GET_FILTERWORDS_ACK";
		case CMD_CS_GET_ROOM_LIMITLIST: //	获取房间禁言/踢人列表
			return "CMD_CS_GET_ROOM_LIMITLIST";
		case CMD_CS_GET_ROOM_LIMITLIST_ACK: //
			return "CMD_CS_GET_ROOM_LIMITLIST_ACK";
		case CMD_CS_ROOMSTAT_SYNC:	//同步房间直播状态
			return "CMD_CS_ROOMSTAT_SYNC";
		case CMD_CS_ROOMSTAT_SYNC_ACK:	//
			return "CMD_CS_ROOMSTAT_SYNC_ACK";
		case CMD_CS_LIMITSTAT_SYNC: //房间权限操作同步
			return "CMD_CS_LIMITSTAT_SYNC";
		case CMD_CS_LIMITSTAT_SYNC_ACK: //
			return "CMD_CS_LIMITSTAT_SYNC_ACK";
		case CMD_CS_GET_ROOM_USERCOUNT: //	获取房间用户人数
			return "CMD_CS_GET_ROOM_USERCOUNT";
		case CMD_CS_GET_ROOM_USERCOUNT_ACK: //
			return "CMD_CS_GET_ROOM_USERCOUNT_ACK";
		case CMD_CS_ROOM_LOCK:	//	超级管理员后台锁闭房间
			return "CMD_CS_ROOM_LOCK";
		case CMD_CS_ROOM_LOCK_ACK:	//	
			return "CMD_CS_ROOM_LOCK_ACK";
		case CMD_CS_ROOM_TITLE_SYNC:	//送礼之星(头衔同步)
			return "CMD_CS_ROOM_TITLE_SYNC";
		case CMD_CS_ROOM_TITLE_SYNC_ACK:	//
			return "CMD_CS_ROOM_TITLE_SYNC_ACK";
		case CMD_CS_SEND_SYSMSG:	//	CS发送系统消息
			return "CMD_CS_SEND_SYSMSG";
		case CMD_CS_SEND_SYSMSG_ACK:	//
			return "CMD_CS_SEND_SYSMSG_ACK";
		case CMD_CS_FILTERWORDS_SYNC:	//	同步敏感字列表
			return "CMD_CS_FILTERWORDS_SYNC";
		case CMD_CS_FILTERWORDS_SYNC_ACK:	//
			return "CMD_CS_FILTERWORDS_SYNC_ACK";
		case CMD_CS_ROOM_VISTERCOUNT_SYNC:	//	同步假人数策略
			return "CMD_CS_ROOM_VISTERCOUNT_SYNC";
		case CMD_CS_ROOM_VISTERCOUNT_SYNC_ACK:	//
			return "CMD_CS_ROOM_VISTERCOUNT_SYNC_ACK";
		case CMD_CS_APPOINT_ROOMADMIN:	//	提升管理员
			return "CMD_CS_APPOINT_ROOMADMIN";
		case CMD_CS_APPOINT_ROOMADMIN_ACK:	//
			return "CMD_CS_APPOINT_ROOMADMIN_ACK";
		case CMD_MODULE_LOGIN:
			return "CMD_MODULE_LOGIN";
		case CMD_MODULE_LOGIN_ACK:
			return "CMD_MODULE_LOGIN_ACK";
		case CMD_GET_ROOM_POS:
			return "CMD_GET_ROOM_POS";
		case CMD_GET_ROOM_POS_ACK:
			return "CMD_GET_ROOM_POS_ACK";	
		case CMD_SYS_CLEAR_ROOM_POS:
			return "CMD_SYS_CLEAR_ROOM_POS";	
		case CMD_SYS_RESET_IDC:
			return "CMD_SYS_RESET_IDC";	
		case CMD_SYS_CLEAN_ROOM:
			return "CMD_SYS_CLEAN_ROOM";	
		case CMD_FORWARD_REQUEST:
			return "CMD_FORWARD_REQUEST";	
		case CMD_FORWARD_REQUEST_ACK:
			return "CMD_FORWARD_REQUEST_ACK";	
		case CMD_SYS_CLOSE_USER:
			return "CMD_SYS_CLOSE_USER";	
		case CMD_SEND_AUDITMSG:
			return "CMD_SEND_AUDITMSG";		
		case CMD_SEND_AUDITMSG_ACK:
			return "CMD_SEND_AUDITMSG_ACK";		
		case CMD_CS_ISSUED_AUDITMSG:
			return "CMD_CS_ISSUED_AUDITMSG";	
		case CMD_CS_ISSUED_AUDITMSG_ACK:
			return "CMD_CS_ISSUED_AUDITMSG_ACK";	
		case CMD_CS_USER_LIMIT_ALL:
			return "CMD_CS_USER_LIMIT_ALL";	
		case CMD_CS_USER_LIMIT_ALL_ACK:
			return "CMD_CS_USER_LIMIT_ALL_ACK";
		case CMD_IM_P2P_MSGSEND:
			return "CMD_IM_P2P_MSGSEND";
		case CMD_IM_P2P_MSGSEND_ACK:
			return "CMD_IM_P2P_MSGSEND_ACK";
		case CMD_IM_P2P_MSGRECV:
			return "CMD_IM_P2P_MSGRECV";
		case CMD_IM_P2P_MSGRECV_ACK:
			return "CMD_IM_P2P_MSGRECV_ACK";
		case CMD_IM_FAMILY_MSGSEND:
			return "CMD_IM_FAMILY_MSGSEND";
		case CMD_IM_FAMILY_MSGSEND_ACK:
			return "CMD_IM_FAMILY_MSGSEND_ACK";
		case CMD_IM_FAMILY_MSGRECV:
			return "CMD_IM_FAMILY_MSGRECV";
		case CMD_IM_FAMILY_GETRECORD:
			return "CMD_IM_FAMILY_GETRECORD";
		case CMD_GET_P2PMSG:
			return "CMD_GET_P2PMSG";
		case CMD_CS_MUTUALFANS_REFRESH:
			return "CMD_CS_MUTUALFANS_REFRESH";
		case CMD_CS_MUTUALFANS_REFRESH_ACK:
			return "CMD_CS_MUTUALFANS_REFRESH_ACK";
		case CMD_GET_EACHFANS:
			return "CMD_GET_EACHFANS";
		case CMD_SYNC_EACHFANS:
			return "CMD_SYNC_EACHFANS";
		case CMD_CS_GETUSEREACHFANS:
			return "CMD_CS_GETUSEREACHFANS";
		case CMD_CS_GETUSEREACHFANS_ACK:
			return "CMD_CS_GETUSEREACHFANS_ACK";
		case CMD_SYNC_ROOM_INFO:
			return "CMD_SYNC_ROOM_INFO";
		case CMD_SYNC_ROOM_INFO_ACK:
			return "CMD_SYNC_ROOM_INFO_ACK";
		case CMD_SYS_CLEAN_ROOM_USER:
			return "CMD_SYS_CLEAN_ROOM_USER";
		case CMD_SYS_SYNC_ANCHOR_NOTIFY:
			return "CMD_SYS_SYNC_ANCHOR_NOTIFY";
		case CMD_SYNC_FAMILY_INFO:
			return "CMD_SYNC_FAMILY_INFO";
		case CMD_LOGIN_FAMILY:
			return "CMD_LOGIN_FAMILY";
		case CMD_LOGIN_FAMILY_ACK:
			return "CMD_LOGIN_FAMILY_ACK";
		case CMD_LOGOUT_FAMILY:
			return "CMD_LOGOUT_FAMILY";
		case CMD_LOGOUT_FAMILY_ACK:
			return "CMD_LOGOUT_FAMILY_ACK";
		case CMD_FAMILY_CREATE:
			return "CMD_FAMILY_CREATE";
		case CMD_FAMILY_CREATE_ACK:
			return "CMD_FAMILY_CREATE_ACK";
		case CMS_TOSYNC_ROOMINFO:
			return "CMS_TOSYNC_ROOMINFO";
		case CMD_NEW_HEARTBEAT:
			return "CMD_NEW_HEARTBEAT";
		case CMD_CS_IM_FAMILY_SYSMSG:
			return "CMD_CS_IM_FAMILY_SYSMSG";
		case CMD_CS_IM_FAMILY_SYSMSG_ACK:
			return "CMD_CS_IM_FAMILY_SYSMSG_ACK";	
		case CMD_SYNC_IOS_USERINFO:
			return "CMD_SYNC_IOS_USERINFO";
		default:
			return "UNKNOW CMD";

    }

	return "ERROR CMD";
}


//#define USdebug
#ifdef USdebug
void debug_log(const char* pszFileName, const int  nLine, const char* func, const char *pszInfo, ...);
#define debug(fmt, args...) debug_log( __FILE__, __LINE__, __func__, fmt , ##args)
void debug_init(const char* logname, int fd, int logsize);
int init_log(const char* process);
#endif

//2014-02-21
/* Compress gzip data */
/* data 原数据 ndata 原数据长度 zdata 压缩后数据 nzdata 压缩后长度 */
int gzcompress(Bytef *data, uLong ndata, Bytef *zdata, uint32 &nzdata);
/* Uncompress gzip data */
/* zdata 数据 nzdata 原数据长度 data 解压后数据 ndata 解压后长度 */
int gzdecompress(Byte *zdata, uint32 nzdata, Byte *data, uint32& ndata);

#endif


