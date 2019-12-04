#ifndef _CONFIG_H
#define _CONFIG_H

#include "Common.h"

struct Args_t
{	
	//it is host byte
	uint32	dwID;
	uint32	dwHostIp;
	uint16	wHostPort;

	uint32  dwCsIp;
	uint16	dwCsPort;

};

class CConfig
{
public:
	CConfig();
	virtual ~CConfig();

public:
	int ArgsParse(int argc, char **argv);

public:
	Args_t	m_stArgs;
};

extern CConfig g_oConfig;

#endif

