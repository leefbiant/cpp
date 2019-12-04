#include "Common.h"
#include "Log.h"
#include "SvrFrame.h"

CSvrFrame* g_pSvrFrame = NULL;
void SigHandler(int signo)
{
	if (NULL != g_pSvrFrame)
	{
		g_pSvrFrame->SigNotify(signo);
	}
}

static int InitDaemon()
{
	struct rlimit rlim;
	/* raise open files */
	rlim.rlim_cur = 20000;
	rlim.rlim_max = 20000;
	setrlimit(RLIMIT_NOFILE, &rlim);
	/* allow core dump */
	rlim.rlim_cur = RLIM_INFINITY;
	rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);


	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	struct sigaction act;
	act.sa_handler = SigHandler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_RESTART;

	if(sigaction(SIGUSR1, &act, NULL) < 0) 
	{
		return -1;
	}
	if(sigaction(SIGUSR2, &act, NULL) < 0) 
	{
		return -1;
	}
	if (sigaction(SIGRTMIN, &act, NULL) < 0)
	{
		return -1;
	}
	if (sigaction(SIGRTMIN + 1, &act, NULL) < 0)
	{
		return -1;
	}
	if (sigaction(SIGRTMIN + 2, &act, NULL) < 0)
	{
		return -1;
	}
	if (sigaction(SIGRTMIN + 3, &act, NULL) < 0)
	{
		return -1;
	}
	//return 0;
	return daemon (1, 1);
}

int main(int argc, char **argv)
{
	InitDaemon();

	if ( 0 != g_oConfig.ArgsParse(argc, argv) )
	{
		printf("config load failed");
		return -1;
	}

	if(!Logger.Init(g_oConfig.m_stArgs.dwID, 0, 0))
	{		
		printf("log init failed!!!\r\n");
		return -1;
	}

	CSvrFrame oSvrFrame;
	if (!oSvrFrame.Init())
	{
		printf("SvrFrame init failed!!!\r\n");
		return -1;
	}
	g_pSvrFrame = &oSvrFrame;

	printf("%s is starting\r\n", argv[0]);
	oSvrFrame.Run();
	return 0;
}
