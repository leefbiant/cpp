#ifndef WEB_SOCKET_H_
#define WEB_SOCKET_H_
#include "Log.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#define prepare(x) do {if (x) { free(x); x = NULL; }} while(0)
struct Handshake 
{
	bool			handshake_ok;
	int     		upgrade_websocket;
	int     		connection_upgrade; 
	uint32 			headlen;
	char          * websocket_key;
	char          * websocket_version;
	char          * get_url;
	char          * host_name;
	char          * origin;
	char          * protocols;
	Handshake():
	handshake_ok(false),
	upgrade_websocket(0),
	connection_upgrade(0),
	headlen(0),
	websocket_key(NULL),
	websocket_version(NULL),
	get_url(NULL),
	host_name(NULL),
	origin(NULL),
	protocols(NULL)
	{
	;
	}
	void clean()
	{
		handshake_ok = false;
		upgrade_websocket = connection_upgrade = 0;
		headlen =0;
		prepare(websocket_key);
		prepare(websocket_version);
		prepare(get_url);
		prepare(host_name);
		prepare(origin);
		prepare(protocols);
		
	}
	
	~Handshake()
	{
		clean();
	}
};

struct Frame 
{
	bool	       has_fin;
	short          op_code;
	bool	       is_masked;
	uint32 		   header_size;
	uint64	       payload_size;
	char           mask[4];
	char 		   payload[32*1024];

	Frame():
	has_fin(0),
	op_code(0),
	is_masked(0),
	payload_size(0)
	{
		memset(mask, 0x00, 4);
		memset(payload, 0x00, sizeof(payload));
	}

	void clean()
	{
		has_fin = is_masked = false;
		op_code = 0;
		payload_size = 0;
		memset(mask, 0x00, 4);
	}
	~Frame()
	{
		clean();
	}
};	



int ws_parse_handshake(const char* input_frame, int32& dwsize, struct Handshake *hs, char *reply);
int ws_get_msg(char* input_frame, int32& dwsize, struct Handshake *hs, struct Frame* fm);
#endif

