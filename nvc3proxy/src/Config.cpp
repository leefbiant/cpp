#include "Config.h"

CConfig g_oConfig;

CConfig::CConfig()
{
	
}

CConfig::~CConfig()
{
}

int CConfig::ArgsParse(int argc, char **argv)
{
	char* pPos = NULL;
	char sArg[50];
	if(argc < 4 || argc%2 == 0)
		return -1;

	for(int i = 1; i < argc; i += 2)
	{
		if(NULL == argv[i+1] )
			continue;
		if(strlen(argv[i+1]) > 50)
			continue;
		memset(sArg, 0, 50);
		strcpy(sArg, argv[i+1]);

		if(strcmp("-id", argv[i]) == 0)
		{	
			m_stArgs.dwID = atoi(sArg);
			//printf("id = %d\r\n", m_stArgs.dwID);
		}
		else if(strcmp("-tcp", argv[i]) == 0)
		{
			pPos = strstr(sArg, ":");
			if(NULL == pPos)
				return -2;

			pPos[0]=0;
			m_stArgs.dwHostIp = ntohl(inet_addr(sArg));
			m_stArgs.wHostPort = atoi(pPos+1);
			//printf("host ip = %d\r\nhost port = %d\r\n", m_stArgs.dwHostIp, m_stArgs.wTcpPort);
		}
		else if(strcmp("-cs", argv[i]) == 0)
		{
			pPos = strstr(sArg, ":");
			if(NULL == pPos)
				return -2;

			pPos[0]=0;
			m_stArgs.dwCsIp = ntohl(inet_addr(sArg));
			m_stArgs.dwCsPort = atoi(pPos+1);
			//printf("host ip = %d\r\nhost port = %d\r\n", m_stArgs.dwHostIp, m_stArgs.wTcpPort);
		}
	}

	return 0;
}

