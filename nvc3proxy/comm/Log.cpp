#include "Log.h"

CLogger Logger;

CLogger::CLogger()
{
	m_wLevel	= ALL_LEVEL&(~DEBUG_LEVEL);	
	m_dwLogID	= 0;
	m_dwIP		= 0;
	m_wPort		= 0;
	m_dwSockFd  = -1;

	m_dwIndexL = m_dwIndexS	= 0;
	m_pLogFile = m_pStatFile = NULL;
}

CLogger::~CLogger()
{
	if (NULL != m_pLogFile)
	{
		fclose(m_pLogFile);
		m_pLogFile = NULL;
	}

	if (NULL != m_pStatFile)
	{
		fclose(m_pStatFile);
		m_pStatFile = NULL;
	}
}

bool CLogger::Init(uint32 dwID, uint32 dwIP, uint16 wPort)
{
	m_dwLogID	= dwID;
	m_dwIP		= dwIP;
	m_wPort		= wPort;

	if (dwIP > 0 && wPort > 0)
	{
		ConnServer();
	}

	pthread_t threadid;
	if(0 != pthread_create(&threadid, NULL, CLogger::ThreadCallback, (void *)this))
		return false;

	return true;
}

void CLogger::SetLogLevel(uint16 wLevel, bool bSet)
{
	if (bSet)
	{
		m_wLevel |= wLevel;
	} 
	else
	{
		m_wLevel &= ~wLevel;
	}

	Logger.Log(NOTIC, "### the log level is 0x%04x, 0x%04x, 0x%04x ###", m_wLevel, wLevel, bSet);
}

const char* CLogger::GetLevelString(LOG_LEVEL eLevel)
{
	switch(eLevel)
	{
	case EMERG_LEVEL:
		return "[EMERG]";
	case ALERT_LEVEL:
		return "[ALERT]";
	case ERROR_LEVEL:
		return "[ERROR]";
	case NOTIC_LEVEL:
		return "[NOTIC]";
	case INFO_LEVEL:
		return "[INFO ]";
	case STAT_LEVEL:
		return "[STAT ]";
	case DEBUG_LEVEL:
		return "[DEBUG]";
	default:
		return "[UNKNOWN]";
	}
}

void * CLogger::ThreadCallback(void *pParam)
{
	CLogger *pLogger = (CLogger *)pParam;
	if(NULL == pLogger)
		return NULL;

	while(1)
	{
		pLogger->m_oSem.Wait();
		do 
		{
			LOG_INFO* stInfo = NULL;
			if(ERR_SUCCESS != pLogger->m_oLogQ.GetObject(&stInfo) && NULL == stInfo)
				break;

			if (pLogger->m_dwIP == 0 || pLogger->m_wPort == 0)
			{
				pLogger->Write(stInfo);
				pLogger->m_oIdleQ.SetObject(stInfo);
			} 
			else
			{
				int nSend = send(pLogger->m_dwSockFd, stInfo->cBuffer, stInfo->wLen, 0);
				if (nSend <= 0)
				{
					if(errno != EAGAIN && errno != EINTR)
					{
						pLogger->Write(stInfo);
						pLogger->m_oIdleQ.SetObject(stInfo);
						pLogger->ConnServer();
					}
					else
					{
						pLogger->m_oLogQ.SetObject(stInfo);
					}
				}
				else
				{
					pLogger->m_oIdleQ.SetObject(stInfo);
				}
			}			

		} while (true);
	}

	return pParam;
}

int  CLogger::Log(const char *pFileName, unsigned int nLine, LOG_LEVEL eLevel, const char *pBuffer, ...)
{
	if(NULL == pFileName || NULL == pBuffer || (eLevel&m_wLevel) != eLevel)
		return -1;

	LOG_INFO* stInfo = NULL;
	if (ERR_SUCCESS != m_oIdleQ.GetObject(&stInfo))
	{
		stInfo = new LOG_INFO();
	}
	stInfo->eLevel = eLevel;
	stInfo->tTime  = time(NULL);
	memset(stInfo->cBuffer, 0x00, sizeof(stInfo->cBuffer));
	int nHeads = 0;
	if (STAT_LEVEL != eLevel)
	{
		nHeads = sprintf(stInfo->cBuffer, "[%s:%d] ", pFileName, nLine);
	}

	va_list sp;
	memset(&sp, 0x00, sizeof(va_list));
	va_start(sp, pBuffer);
	int nBodys = vsnprintf(stInfo->cBuffer + nHeads, MAX_LOG_BUFFER - nHeads, pBuffer, sp);
	va_end(sp);

	int nLen = nHeads + nBodys;
	if(nLen < 0)
	{
		m_oIdleQ.SetObject(stInfo);
		return -1;
	}
	else if (nLen >= MAX_LOG_BUFFER)
	{
		nLen = MAX_LOG_BUFFER-1;
	}
	stInfo->cBuffer[nLen] = '\n';
	stInfo->wLen = nLen+1;
	
	m_oLogQ.SetObject(stInfo);
	if (m_oSem.GetValue() <= 0)
	{
		m_oSem.Post();
	}
	
	return nLen;
}

bool CLogger::ConnServer()
{
	if(m_dwSockFd > 0)
	{
		close(m_dwSockFd);
	}

	m_dwSockFd = socket(AF_INET, SOCK_STREAM, 0);
	if(m_dwSockFd <= 0)
	{
		return false;
	}

	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	bzero(&addr, addrlen);
	addr.sin_family		 = AF_INET;
	addr.sin_port		 = htons(m_wPort);
	addr.sin_addr.s_addr = m_dwIP;

	if(0 != connect(m_dwSockFd, (sockaddr *)&addr, addrlen))
	{		
		close(m_dwSockFd);
		m_dwSockFd = 0;
		return false;
	}
	int iFlag = fcntl(m_dwSockFd, F_GETFL, 0);
	if(-1 == iFlag)
	{
		close(m_dwSockFd);
		m_dwSockFd = 0;
		return false;
	}
	if(-1 == fcntl(m_dwSockFd, F_SETFL, iFlag | O_NONBLOCK))
	{
		close(m_dwSockFd);
		m_dwSockFd = 0;
		return false;
	}

	return true;
}

int CLogger::Write(LOG_INFO *pLogInfo)
{
	if(NULL == pLogInfo)
		return -1;

	struct timeval stLogTv;
	gettimeofday(&stLogTv, NULL);
	struct tm *pLogTime = localtime(&pLogInfo->tTime);
	if(NULL == pLogTime)
		return 0;

	int nNums    = -1;
	int nHeads = 0;
	FILE *pFile = NULL;
	char cFullPath[MAX_LOG_PATH] = {0};
	char cHeads[256] = {0};

	if (STAT_LEVEL == pLogInfo->eLevel)
	{
		sprintf(cFullPath, "%s_%04d%02d%02d%02d_%02d.txt", DEFAULT_STAT_PATH, pLogTime->tm_year + 1900, pLogTime->tm_mon+1, pLogTime->tm_mday, pLogTime->tm_hour, m_dwLogID);

		nHeads = sprintf(cHeads, "%d-%02d-%02d %02d:%02d:%02d,", 
			pLogTime->tm_year + 1900, pLogTime->tm_mon + 1, pLogTime->tm_mday, pLogTime->tm_hour, pLogTime->tm_min, pLogTime->tm_sec);

		if (NULL == m_pStatFile)
		{
			pFile = m_pStatFile = fopen(cFullPath, "a+");
		} 
		else if(pLogTime->tm_hour != (int32)m_dwIndexS)
		{			
			fclose(m_pStatFile);

			m_dwIndexS = pLogTime->tm_hour;
			pFile = m_pStatFile = fopen(cFullPath, "a+");
		}
		else
		{
			pFile = m_pStatFile;
		}
	} 
	else
	{
		uint32 dwIndex = pLogTime->tm_hour/6 + 1;
		sprintf(cFullPath, "%s_%02d_%02d-%02d_%d.log", DEFAULT_LOG_PATH, pLogTime->tm_mon+1, pLogTime->tm_mday, dwIndex, m_dwLogID);

		nHeads = sprintf(cHeads, "[%d] %s [%02d-%02d %02d:%02d:%02d:%03ld:%03ld] ",
			m_dwLogID, GetLevelString(pLogInfo->eLevel), pLogTime->tm_mon+1, pLogTime->tm_mday, pLogTime->tm_hour, pLogTime->tm_min, pLogTime->tm_sec, stLogTv.tv_usec/1000,  stLogTv.tv_usec%1000);

		if (NULL == m_pLogFile)
		{
			pFile = m_pLogFile = fopen(cFullPath, "a+");
		} 
		else if(dwIndex != m_dwIndexL)
		{			
			fclose(m_pLogFile);

			m_dwIndexL = dwIndex;
			pFile = m_pLogFile = fopen(cFullPath, "a+");
		}
		else
		{
			pFile = m_pLogFile;
		}
	}
	
	if(NULL != pFile)
	{
		if (nHeads > 0)
		{
			nNums += fwrite(cHeads, 1, nHeads, pFile);
		}

		if (pLogInfo->wLen > 0)
		{
			nNums += fwrite(pLogInfo->cBuffer, 1, pLogInfo->wLen, pFile);
		}

		if(nNums > 0)
			fflush(pFile);
	}

	return nNums;
}


#ifdef __CONN__
int g_statfd = 2;
const char*  g_stat_logname= NULL;
uint32 g_dwid = 0;
const char* getlogname(char* logname)
{
	struct timeval stLogTv;
	gettimeofday(&stLogTv, NULL);
	struct tm strc_now = *localtime(&stLogTv.tv_sec);
	
	snprintf(logname,63, "%s_%04d%02d%02d%02d_IM_%u.log", DEFAULT_STAT_PATH,
		strc_now.tm_year + 1900, strc_now.tm_mon + 1, strc_now.tm_mday,strc_now.tm_hour, g_dwid);
	return logname;
}
void debug_init(const char* logname, int fd)
{
	if( fd > 0 ) g_statfd  = fd;
	if(logname) g_stat_logname = strdup(logname);
	return ;
}

int init_stat_log(uint32 dwid)
{

	g_dwid = dwid;
	char log_name[128] = {0};

	getlogname(log_name);
		
	int fd = open(log_name, O_CREAT|O_RDWR|O_APPEND, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
	if( fd < 0 )
	{
		printf("open file ./log/client.log failed:%s",  strerror(errno));
		return -1;
	}
	debug_init(log_name, fd);
	return 0;	
}


char* DateTimeStr(char *pszDest, const int nMaxOutSize)
{
    if (pszDest == NULL ) return NULL;
	
	struct timeval stLogTv;
	gettimeofday(&stLogTv, NULL);
	struct tm strc_now = *localtime(&stLogTv.tv_sec);
    snprintf(pszDest, nMaxOutSize, "%04d-%02d-%02d %02d:%02d:%02d"
            , strc_now.tm_year + 1900, strc_now.tm_mon + 1, strc_now.tm_mday
            , strc_now.tm_hour, strc_now.tm_min, strc_now.tm_sec);
   

    return pszDest;
}
void shift_stat_log()
{
	if(!g_stat_logname)return;

	struct stat stat_buf;
	if( lstat(g_stat_logname, &stat_buf) != 0 )
	{
		return ;
	}
	if(!S_ISREG(stat_buf.st_mode))
	{
		return ;
	}
	char log_name[128] = {0};
	if( strcmp(g_stat_logname, getlogname(log_name)) >= 0  )return;
	Logger.Log(NOTIC, "g_logname[%s] log_name[%s] start new log",g_stat_logname, log_name);	

	close(g_statfd);
	g_stat_logname = strdup(log_name);
	g_statfd = open(g_stat_logname, O_CREAT|O_RDWR|O_APPEND, S_IRUSR|S_IWUSR|S_IWGRP|S_IRGRP|S_IROTH);
	return ;
	
}

void stat_log(const char* pszFileName, const int  nLine, const char* func, const char *pszInfo, ...)
{
	char printMsg[512] = {0};
	char writebuf[1024] = {0};
	/* Timestamps... */
	char timedatestamp[32] = {0};
	DateTimeStr(timedatestamp, sizeof(timedatestamp));
	va_list argptr;
	va_start(argptr, pszInfo);
	vsnprintf(printMsg, sizeof(printMsg)-1, pszInfo, argptr);
	va_end(argptr);

	
	snprintf(writebuf, 1023, "%s,%s",timedatestamp, printMsg);
	if(printMsg[strlen(printMsg)-1] != '\n')
	{
		strncat(writebuf, "\n", 1023);
	}
	write(g_statfd, writebuf, strlen(writebuf));
	return shift_stat_log();
}
#endif

