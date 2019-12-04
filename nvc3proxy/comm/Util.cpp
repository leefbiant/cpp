#include "Util.h"
#include "Log.h"


//////////////////////////////////////////////////////////////////////////
CTimer* CTimer::m_pInstance = NULL;
CTimer::CTimer()
{

}

CTimer::~CTimer()
{
	map<uint32, TIMER_INFO_t*>::iterator it = m_mapTimers.begin();
	for(; it != m_mapTimers.end(); it++)
	{
		timer_delete(it->second->tid);
		delete it->second;
	}
	m_mapTimers.clear();
}

CTimer* CTimer::GetInstance()
{
	if (NULL == m_pInstance)
	{
		m_pInstance = new CTimer();
	}

	return m_pInstance;
}

int CTimer::SetTimer(ITimer* pTimer, uint32 dwID, uint32 dwSecond)
{
	if(NULL == pTimer || dwID <= 0 || dwSecond <= 0)
		return -1;

	TIMER_INFO_t *pInfo = new TIMER_INFO_t;
	if(NULL == pInfo)
		return -1;
	memset(pInfo, 0, sizeof(TIMER_INFO_t));
	pInfo->pTimer   = pTimer;
	pInfo->dwID     = dwID;
	pInfo->bInto    = false;

	timer_t tid = 0;
	struct sigevent se;
	struct itimerspec ts, ots;
	memset(&se, 0, sizeof(struct sigevent));
	se.sigev_notify          = SIGEV_THREAD;
	se.sigev_notify_function = OnHandle;
	//	se.sigev_value.sival_int = uiID;   //作为handle()的参数
	se.sigev_value.sival_ptr = pInfo;

	if(timer_create(CLOCK_REALTIME, &se, &tid) < 0)
	{	
		delete pInfo;
		pInfo = NULL;
		return   -1;	
	}

	ts.it_value.tv_sec     = dwSecond;	
	ts.it_value.tv_nsec    = 0;
	ts.it_interval.tv_sec  = dwSecond;
	ts.it_interval.tv_nsec = 0;

	if(timer_settime(tid, TIMER_ABSTIME, &ts, &ots) < 0)
	{	
		delete pInfo;
		pInfo = NULL;

		timer_delete(tid);
		return   -1;
	}

	KillTimer(dwID);
	pInfo->tid = tid;
	m_mapTimers[dwID] = pInfo;

	return dwID;
}

void CTimer::KillTimer(uint32 dwID)
{
	map<uint32, TIMER_INFO_t*>::iterator it = m_mapTimers.find(dwID);
	if(it != m_mapTimers.end())
	{
		timer_delete(it->second->tid);
		delete it->second;
		m_mapTimers.erase(it);
	}
}

void  CTimer::OnHandle(union sigval v)
{
	TIMER_INFO_t* pInfo = (TIMER_INFO_t*)v.sival_ptr;
	if(NULL != pInfo && !pInfo->bInto)
	{
		pInfo->bInto = true;
		pInfo->pTimer->OnTimer(pInfo->dwID);
		pInfo->bInto = false;
	}
}

//////////////////////////////////////////////////////////////////////////
CIPQuery::CIPQuery()
{
	m_data=NULL;

}

CIPQuery::~CIPQuery()
{
	if(m_data!=NULL)
	{
		free(m_data);
		m_data = NULL;
	}

}

// 功能：将IP所有数据读入全局数组data
// 参数：file 文件字符串
// 返回值： 0 成功 

int CIPQuery::init(const char* file)
{
	if(file==NULL)
	{
		return 1;
	}

	unsigned long  fileSize;    //文件长度
	FILE* fp;

	fp = fopen((char*)file,"rb");
	if(fp == NULL)
		return 1;

	//获取文件长度
	fseek(fp,   0,   SEEK_END);  
	fileSize   =   ftell(fp); 

	if(NULL!=m_data)
	{
		free(m_data);
		m_data=NULL;
	}

	//将纯真IP数据库中的所有数据读入数组
	m_data=(char*)malloc(fileSize* sizeof(char));
	fseek(fp,0,SEEK_SET);
	fread(m_data,1,fileSize,fp);
	if(m_data == NULL)
	{
		return 2;
	}

	fclose(fp);
	fp=NULL;

	return 0;
}

//根据IP生成查询信息
int CIPQuery::getqueryinfo(const char* ip,char* infoout,const int querytype)
{
	if (ip==NULL||infoout==NULL)
	{
		return -1;
	}

	char  *d=".";
	char  strip[30]={0};
	strcpy(strip,ip);

	char* p1=strtok(strip,d);
	if (p1!=NULL) 
	{	
		strcpy(infoout,p1);
		strcat(infoout,".");
		p1=strtok(NULL,d);
	}
	else return -1;

	if (p1!=NULL) 
	{	
		strcat(infoout,p1);
		strcat(infoout,".");
		p1=strtok(NULL,d);
	}
	else return -1;

	if (querytype==1)
	{
		if (p1!=NULL)
		{
			strcat(infoout,p1);
			strcat(infoout,".*");
		}
	}
	else
	{
		strcat(infoout,"*.*");
	}

	return 0;
}

//查询IP地址
int CIPQuery::queryip(const char* ip,IPINFO &infoout)
{
	if(NULL==ip||NULL==m_data)
	{
		return -1;
	}

	memset(&infoout,0,sizeof(IPINFO));
	char  strtst[250] = {0};
	char  strip[30] = {0};
	char  *pt=strtst;
	char  *d=",";
	if (getqueryinfo(ip,strip)!=0)
	{
		return -1;
	}

	char* p1=strstr((char*)m_data,(char*)strip);
	if (p1==NULL) 
	{
		getqueryinfo(ip,strip,QueryType2);
		p1=strstr((char*)m_data,(char*)strip);
		if (p1==NULL) 
		{
			return -1;
		}
	}

	char *p2=strstr(p1,"\n");
	if (p2==NULL)
	{
		return -1;
	}

	while (p1!=p2) 
	{
		*pt++=*p1++;
	}

	//printf("pt=%s\r\n",strtst);

	char* p3=strtok(strtst,d);
	if (p3!=NULL) 
	{	
		p3=strtok(NULL,d);
	}
	else
	{
		return -1;
	}

	if (p3!=NULL)
	{
		strcpy(infoout.Operators,p3);
		p3=strtok(NULL,d);
	}
	else
	{
		return -1;
	}

	if (p3!=NULL)
	{
		strcpy(infoout.Province,p3);
		p3=strtok(NULL,d);
	}

	if (p3!=NULL)
	{
		strcpy(infoout.city,p3);
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
CSem::CSem():
m_sem(NULL)
{
	m_sem = new sem_t;
	if(0 != sem_init(m_sem, 0, 0))
	{
		Logger.Log(ERROR, "sem_init failed");
	}
}

CSem::~CSem()
{
	if(0 != sem_destroy(m_sem))
	{
		Logger.Log(ERROR, "sem_destroy failed");
	}

	if(NULL != m_sem)
	{
		delete m_sem;
		m_sem = NULL;
	}
}

int CSem::Wait()
{
	return sem_wait(m_sem);
}

int CSem::timedwait()
{
	struct timespec ts;
	ts.tv_sec=time(NULL)+1;   // 重点
        ts.tv_nsec=0;
        return sem_timedwait(m_sem, &ts);
}
int CSem::Post()
{
	return sem_post(m_sem);
}

int CSem::GetValue()
{
	int iValue = 0;
	sem_getvalue(m_sem, &iValue);

	return iValue;
}

//////////////////////////////////////////////////////////////////////////
CThread::CThread(CThreadPool* pThreadPool):
m_pThreadPool(pThreadPool),
m_pTask(NULL)
{
	m_bExit = false;
}

CThread::~CThread()
{
	m_pThreadPool = NULL;
	m_pTask		  = NULL;
}

bool CThread::Start()
{
	if(pthread_create(&m_threadid, NULL, Callback, (void*)this) != 0)
	{
		Logger.Log(ERROR, "pthread_create failed");

		return false;
	}

	return true;
}

bool CThread::Stop()
{
	m_bExit = true;
	m_oSem.Post();
	pthread_join(m_threadid, NULL);
	return true;
}

void CThread::AddTask(CTask* pTask)
{
	m_pTask = pTask;
	if (m_oSem.GetValue() <= 0)
	{
		m_oSem.Post();
	}
}

int CThread::Run()
{
	for (;;)
	{
		m_oSem.Wait();
		if(m_bExit)
			break;
		//printf("threadid = %d\r\n", m_threadid);
		if (NULL != m_pTask &&  m_pTask->m_fCallback )
		{
			m_pTask->m_fCallback((void*)m_pTask, m_pThreadPool->GetFlag());
			m_pTask = NULL;

			m_pThreadPool->PushThread(this);
		}		
	}

	return 0;
}

void* CThread::Callback(void *pParam)
{
	CThread* pThread = (CThread*)pParam;
	if (NULL != pThread)
	{
		pThread->Run();
	}

	return (void*)NULL;
}

//////////////////////////////////////////////////////////////////////////
CThreadPool::CThreadPool()
{

}

CThreadPool::~CThreadPool()
{

}

bool CThreadPool::Init(uint32 dwThreadNum, uint16 wFlag)
{
	if(dwThreadNum <= 0)
		return false;
	m_wFlag = wFlag;

	for (uint32 i = 0; i < dwThreadNum; i++)
	{
		CThread* pThread = new CThread(this);
		if (NULL != pThread)
		{
			pThread->Start();
			m_oThreadQ.SetObject(pThread);
		}
	}

	return true;
}

void CThreadPool::PushThread(CThread* pThread)
{
	if (NULL == pThread)
		return;

	CTask* pTask = NULL;
	if (ERR_SUCCESS == m_oTaskQ.GetObject(&pTask) && NULL != pTask)
	{
		pThread->AddTask(pTask);
	} 
	else
	{
		m_oThreadQ.SetObject(pThread);
	}		
}

void CThreadPool::SubmitTask(CTask* pTask)
{
	if(NULL == pTask)
		return;

	CThread* pThread = NULL;
	if (ERR_SUCCESS == m_oThreadQ.GetObject(&pThread) && NULL != pThread)
	{
		pThread->AddTask(pTask);
	}
	else
	{
		m_oTaskQ.SetObject(pTask);
	}
}

void CThreadPool::CancelTask(uint16 iState)
{

}

char* intip2char(int intip)
{
	in_addr addr = {0};
	addr.s_addr  = intip;
	return inet_ntoa(addr);
}


int	LoadFilterKeyWords(string& strFilterWords)
{
	FILE *pFile = NULL;
	pFile  = fopen("./filterwords.txt", "r");
	if (NULL != pFile)
	{
		strFilterWords = ",";
		int ret = 0;
		char buf[1024] = {0};
		while((ret = fread(buf, 1, 1023, pFile)) > 0)
		{
			buf[ret] = '\0';
			strFilterWords += buf;
		}
		strFilterWords += ",";
		fclose(pFile);
		Logger.Log(INFO, "filter words:%s", strFilterWords.c_str());
	}
	return 0;
}

int	FilterKeyWords(uint8* pKeyWords, uint32 dwKeyLen, uint8* pContent, uint32 pContentLen)
{
	if (NULL == pKeyWords || 0 == dwKeyLen || NULL == pContent || 0 == pContentLen)
		return -1;
	bool isReset = false;

	//连续5个数字也过滤
	uint8* pChar = pContent;
	uint32 cnt = 0;
	//uint32 matchLen = 0;
	while( *pChar )
	{
		if( *pChar >= '0' && *pChar <= '9')
			cnt++;
		else 
			cnt = 0;
			
		if( cnt >= 5 )
		{
			pChar = pContent;
			while( *pChar )
			{
				if( *pChar >= '0' && *pChar <= '9')
					*pChar = '*';
				pChar++;
			}

			Logger.Log(DEBUG, "FilterKeyWords find Err msg[%s]!", pContent);
			isReset = true;
			break;
			//return true;
		}
		pChar++;
		//matchLen++;
	}

	//
	char *p1 = NULL;
	char *p2 = NULL;
	char *pFind = NULL;
	p1 = (char*)pKeyWords;
	while (p1 < ((char*)pKeyWords + dwKeyLen))
	{
		p2 = strstr(p1, ",");
		if (NULL == p2)
			break;
		if (p1 != p2)
		{
			p2[0] = '\0';
			int keylen = p2 - p1;
			while (1)
			{
				pFind = strcasestr((char*)pContent, p1);
				if (NULL != pFind)
				{
					for (int i = 0; i < keylen; i++)
					{
						pFind[i] = '*';
						isReset = true;
					}
				}
				else
					break;
			}
			p2[0] = ',';
		}
		p1 = p2 + 1;
	}
	return isReset == false?0:1;
}


//得到通传参数如字段的值
//"imei=4600130006454310&nimei=356261051993846&pf=AND_16&pfs=AND_4.1.2&md=pb_nvc_anzhi"
string GetMyCol(char *Datain,char *Names )
{
	char DataOut[1024] = {0};
	int buf_len = 1024;

	char ColNames[100]={0};
	strcpy(ColNames,Names);
	strcat(ColNames,"=");
	char *ptr1=NULL;
	ptr1=strstr(Datain,ColNames);
	
	//如果可以找到字段名
	if (ptr1!=NULL)
	{
		ptr1+=strlen(ColNames);
		char *ptr2=strstr(ptr1,"&");		
		char *p=DataOut;

		//如果找得到"&"
		if (ptr2!=NULL)
		{
            if(ptr2-ptr1 >= buf_len)
            {
				strcpy(DataOut,"");
                return DataOut;
            }
            
			while (ptr1!=ptr2) 
			{
				*p++=*ptr1++;
			}
		}
		else
		{
            int i = 0;
			while (*ptr1!='\0'&&*ptr1!='\r'&&*ptr1!='\n'&&*ptr1!=' ') 
			{
				*p++=*ptr1++;

                i++;
                if(i >= buf_len)
                {
					strcpy(DataOut,"");
                    return DataOut;
                }
			}
		}

		*p='\0';
		ptr2=NULL;
	}
	else
	{
		strcpy(DataOut,"");
		return DataOut;
	}
	ptr1=NULL;
	
	return DataOut;
}




#ifdef USdebug
int g_logfd = 2;
int g_maxx_log_size = (1024*1024*300);
const char*  g_logname= NULL;
#define MAX_LOG_SIZE  		1024*1024*300
int init_log(const char* process)
{

	if(access("./log", F_OK) != 0 )
	{
		mkdir("./log", S_IXUSR|S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
	}
	char log_base_name[128] = {0};
	char log_name[64] = {0};
	strncpy(log_base_name, process, 127);
	char *pbasename = basename(log_base_name);

	snprintf(log_name,63, "./log/%s_%d.log", pbasename, getpid());
	
	int fd = open(log_name, O_CREAT|O_RDWR|O_APPEND, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
	if( fd < 0 )
	{
		debug("open file ./log/client.log failed:%s",  strerror(errno));
		return -1;
	}
	debug_init(log_name, fd, MAX_LOG_SIZE);
	return 0;	
}

void debug_init(const char* logname, int fd, int logsize)
{
	if( fd > 0 ) g_logfd = fd;
	if(logsize > 0 ) g_maxx_log_size= logsize;
	if(logname) g_logname =strdup(logname);
	return ;
}
char* GetCurDateTimeStr(char *pszDest, const int nMaxOutSize)
{
    if (pszDest == NULL ) return NULL;
	
	struct timeval stLogTv;
	gettimeofday(&stLogTv, NULL);
	struct tm strc_now = *localtime(&stLogTv.tv_sec);
    snprintf(pszDest, nMaxOutSize, "%04d-%02d-%02d %02d:%02d:%02d:%03ld:%03ld"
            , strc_now.tm_year + 1900, strc_now.tm_mon + 1, strc_now.tm_mday
            , strc_now.tm_hour, strc_now.tm_min, strc_now.tm_sec, stLogTv.tv_usec/1000,  stLogTv.tv_usec%1000);
   

    return pszDest;
}
void shift_log()
{
	if(!g_logname)return;

	struct stat stat_buf;
	if( lstat(g_logname, &stat_buf) != 0 )
	{
		return ;
	}
	if(!S_ISREG(stat_buf.st_mode))
	{
		return ;
	}
	if( stat_buf.st_size < g_maxx_log_size)return;

	char log_backup[256] = {0};
	snprintf(log_backup, 255, "%s.bak", g_logname);

	close(g_logfd);
	rename(g_logname, log_backup);
	g_logfd = open(g_logname, O_CREAT|O_RDWR|O_APPEND, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
	return ;
	
}

void debug_log(const char* pszFileName, const int  nLine, const char* func, const char *pszInfo, ...)
{
	char printMsg[512] = {0};
	char writebuf[1024] = {0};
	/* Timestamps... */
	char timedatestamp[32] = {0};
	GetCurDateTimeStr(timedatestamp, sizeof(timedatestamp));
	va_list argptr;
	va_start(argptr, pszInfo);
	vsnprintf(printMsg, sizeof(printMsg)-1, pszInfo, argptr);
	va_end(argptr);

	
	snprintf(writebuf, 1023, "[%s]|%d|%ld(%s:%d:%s):%s",timedatestamp, getpid(),syscall(SYS_gettid),pszFileName, nLine, func, printMsg);
	if(printMsg[strlen(printMsg)-1] != '\n')
	{
		strncat(writebuf, "\n", 1023);
	}
	write(g_logfd, writebuf, strlen(writebuf));
	return shift_log();
}
#endif

int gzcompress(Bytef *data, uLong ndata,Bytef *zdata, uint32 &nzdata)
{
	z_stream c_stream;
	int err = 0;
	if( NULL == data || ndata <= 0 )return -1;

	c_stream.zalloc = NULL;
	c_stream.zfree = NULL;
	c_stream.opaque = NULL;
	
	c_stream.next_in  = data;
	c_stream.avail_in  = ndata;
	c_stream.next_out = zdata;
	c_stream.avail_out  = nzdata;
	
	//只有设置为MAX_WBITS + 16才能在在压缩文本中带header和trailer
	if(deflateInit2(&c_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
					MAX_WBITS + 16, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK) return -1;

	err = deflate(&c_stream, Z_FINISH);
	deflateEnd(&c_stream);
	if( err !=  Z_STREAM_END)
	{
		return -1;
	}
	nzdata = c_stream.total_out;
	return 0;

}

int gzdecompress(Byte *zdata, uint32 nzdata, Byte *data, uint32& ndata)
{
	int err = 0;
	z_stream d_stream = {0}; /* decompression stream */
	d_stream.zalloc = NULL;
	d_stream.zfree = NULL;
	d_stream.opaque = NULL;
	
	d_stream.next_in  = zdata;
	d_stream.avail_in = nzdata;
	d_stream.next_out = data;
	d_stream.avail_out = ndata;
	//只有设置为MAX_WBITS + 16才能在解压带header和trailer的文本
    if(inflateInit2(&d_stream, MAX_WBITS + 16) != Z_OK) return -1;
    err = inflate(&d_stream, Z_NO_FLUSH);
    inflateEnd(&d_stream);
    if( err != Z_STREAM_END )
    {
    	 return -1;
    }	 
    ndata = d_stream.total_out;
  	return 0;
}


