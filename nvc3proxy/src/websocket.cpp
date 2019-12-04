#include "websocket.h"
#include "sha1.h"
#include "base64.h"
#ifndef INT_TO_PTR
#define INT_TO_PTR(integer)   ((void*) (long) ((int)integer))
#endif

#define	DEFAULT_PROT_VER 13

void dumpmsg(const char* msg, int size)
{
	char buff[4096] = {0};
	int offset = 0;
	const char* fmt = "%02X%02X%02X%02X  %02X%02X%02X%02X  %02X%02X%02X%02X  %02X%02X%02X%02X\n";
	int block = 0;
	while( block < size )
	{
		offset += snprintf(buff + offset , 4095-offset, fmt,   (unsigned char)msg[0+block],(unsigned char)msg[1+block],(unsigned char)msg[2+block],(unsigned char)msg[3+block],
															   (unsigned char)msg[4+block],(unsigned char)msg[5+block],(unsigned char)msg[6+block],(unsigned char)msg[7+block],
															   (unsigned char)msg[8+block],(unsigned char)msg[9+block],(unsigned char)msg[10+block],(unsigned char)msg[11+block],
															   (unsigned char)msg[12+block],(unsigned char)msg[13+block],(unsigned char)msg[14+block],(unsigned char)msg[15+block]);
		block += 16;
	}
	Logger.Log(INFO, "Msg dump msg legth %d \n%s", size, buff);
	
	int headlen = 4+4+2+2+8+8+4;
	if( size > headlen && size < 4096 )
	{
		memcpy(buff, msg+headlen, size-headlen);
		buff[size-headlen] = '\0';
		Logger.Log(INFO, "jsonbuf len[%d] \n[%s]", size-headlen, buff);
	}
	
}

int ws_getline_from_buf(const char* src, char* line, int size, uint32* offset)
{
	const char* pEnd = NULL;
	if( strlen(src) < *offset )
	{
		return -1;
	}
	if( (pEnd = strstr(src + *offset, "\r\n") ) == NULL )
	{
		return -1;
	}
	pEnd += 2;
	int n = pEnd - (src + *offset);
	if( n > size )
	{
		return -1;
	}
	if( n == 0 )
	{
		return 0;
	}
	strncpy(line, src + *offset, n);
	*offset += n;
	return n;
}

bool ws_cmp (const char * string1, const char * string2)
{
	int iterator;
	if (string1 == NULL && string2 == NULL)
		return true;
	if (string1 == NULL || string2 == NULL)
		return false;

	/* next position */
	iterator = 0;
	while (string1[iterator] && string1[iterator]) {
		if (string1[iterator] != string2[iterator])
			return false;
		iterator++;
	} /* end while */
	
	/* last check, ensure both ends with 0 */
	return string1[iterator] == string2[iterator];
}

bool ws_ncmp (const char * string1, const char * string2, int bytes)
{
	int iterator;
	if (bytes <= 0)
		return false;
	if (string1 == NULL && string2 == NULL)
		return true;
	if (string1 == NULL || string2 == NULL)
		return false;

	/* next position */
	iterator = 0;
	while (string1[iterator] && string1[iterator] && iterator < bytes) {
		if (string1[iterator] != string2[iterator])
			return false;
		iterator++;
	} /* end while */
	
	/* last check, ensure both ends with 0 */
	return iterator == bytes;
}

bool ws_conn_get_http_url (struct Handshake *hs, const char * buffer, int buffer_size,  char ** url)
{
	int          iterator;
	int          iterator2;

	/* check if we already received method */
	if (hs->get_url) 
	{
		return false;
	} /* end if */
	
	/* the get url must have a minimum size: GET / HTTP/1.1\r\n 16 (15 if only \n) */
	if (buffer_size < 15) 
	{
		return false;
	} /* end if */
	
	/* skip white spaces */
	iterator = 4;
	while (iterator <  buffer_size && buffer[iterator] == ' ') 
		iterator++;
	if (buffer_size == iterator) 
	{

		return false;
	} /* end if */

	/* now check url format */
	if (buffer[iterator] != '/') 
	{
		return false;
	}
	
	/* ok now find the rest of the url content util the next white space */
	iterator2 = (iterator + 1);
	while (iterator2 <  buffer_size && buffer[iterator2] != ' ') 
		iterator2++;
	if (buffer_size == iterator2)
	{
		return false;
	} /* end if */
	
	(*url) = (char*)calloc(1,iterator2 - iterator + 1);
	memcpy (*url, buffer + iterator, iterator2 - iterator);


	/* now check final HTTP header */
	iterator = iterator2 + 1;
	while (iterator <  buffer_size && buffer[iterator] == ' ') 
		iterator++;
	if (buffer_size == iterator)
	{
		return false;
	} /* end if */

	/* now check trailing content */
	return ws_cmp (buffer + iterator, "HTTP/1.1\r\n") || ws_cmp (buffer + iterator, "HTTP/1.1\n");
}

bool ws_is_white_space  (char * chunk)
{
	/* do not complain about receive a null refernce chunk */
	if (chunk == NULL)
		return false;
	
	if (chunk[0] == ' ')
		return true;
	if (chunk[0] == '\n')
		return true;
	if (chunk[0] == '\t')
		return true;
	if (chunk[0] == '\r')
		return true;

	/* no white space was found */
	return false;
}

void        ws_trim  (char * chunk, int * trimmed)
{
	int    iterator;
	int    iterator2;
	int    end;
	int    total;

	/* perform some environment check */
	if (chunk == NULL)
		return;

	/* check empty string received */
	if (strlen (chunk) == 0) {
		if (trimmed)
			*trimmed = 0;
		return;
	}

	/* check the amount of white spaces to remove from the
	 * begin */
	iterator = 0;
	while (chunk[iterator] != 0) {
		
		/* check that the iterator is not pointing to a white
		 * space */
		if (! ws_is_white_space (chunk + iterator))
			break;
		
		/* update the iterator */
		iterator++;
	}

	/* check for the really basic case where an empty string is found */
	if (iterator == (int)strlen (chunk)) {
		/* an empty string, trim it all */
		chunk [0] = 0;
		if (trimmed)
			*trimmed = iterator;
		return;
	} /* end if */

	/* now get the position for the last valid character in the
	 * chunk */
	total   = strlen (chunk) -1;
	end     = total;
	while (chunk[end] != 0) {
		
		/* stop if a white space is found */
		if (! ws_is_white_space (chunk + end)) {
			break;
		}

		/* update the iterator to eat the next white space */
		end--;
	}

	/* the number of items trimmed */
	total -= end;
	total += iterator;
	
	/* copy the exact amount of non white spaces items */
	iterator2 = 0;
	while (iterator2 < (end - iterator + 1)) {
		/* copy the content */
		chunk [iterator2] = chunk [iterator + iterator2];

		/* update the iterator */
		iterator2++;
	}
	chunk [ end - iterator + 1] = 0;

	if (trimmed != NULL)
		*trimmed = total;

	/* return the result reference */
	return;	
}


bool ws_conn_get_mime_header (const char * buffer, int buffer_size, char ** header, char ** value)
{
	int iterator = 0;
	int iterator2 = 0;

	/* ws_log (ctx, ws_LEVEL_DEBUG, "Processing content: %s", buffer); */
	
	/* ok, find the first : */
	while (iterator < buffer_size && buffer[iterator] && buffer[iterator] != ':')
		iterator++;
	if (buffer[iterator] != ':')
	{
		Logger.Log(ERROR, "not find ':' from[%s] size[%d] iterator[%d]", buffer, buffer_size, iterator);
		return false;
	} 

	/* copy the header value */
	(*header) = (char*)calloc (1, iterator + 1);
	memcpy (*header, buffer, iterator);
	
	/* now get the mime header value */
	iterator2 = iterator + 1;
	while (iterator2 < buffer_size && buffer[iterator2] && buffer[iterator2] != '\n')
		iterator2++;
	if (buffer[iterator2] != '\n') 
	{
		Logger.Log(ERROR, "not find '\\n' from[%s] size[%d] iterator2[%d]", buffer, buffer_size, iterator2);
		return false;
	} 

	/* copy the value */
	(*value) = (char*)calloc (1, (iterator2 - iterator) + 1);
	memcpy (*value, buffer + iterator + 1, iterator2 - iterator);

	/* trim content */
	ws_trim (*value, NULL);
	ws_trim (*header, NULL);
	
	return true;
}
bool ws_conn_check_mime_header_repeated ( 
						    char         * header, 
						    char         * value, 
						    const char   * ref_header, 
						    void*      check)
{
	if (strcasecmp (ref_header, header) == 0)
	{
		if (check) 
		{
			Logger.Log(ERROR, "Provided header %s twice, closing connection", header);
			free (header);
			free (value);
			return true;
		} /* end if */
	} /* end if */
	return false;
}
int ws_complete_handshake(struct Handshake *hs, char * buffer, int buffer_size)
{
	char * header;
	char * value;
	//Logger.Log(INFO, "Parse Http line[%s]", buffer);
		
	/* handle content */
	if (ws_ncmp (buffer, "GET ", 4))
	{
		/* get url method */
		ws_conn_get_http_url (hs, buffer, buffer_size,  &hs->get_url);
		return 1;
	} /* end if */

	/* get mime header */
	if (! ws_conn_get_mime_header (buffer, buffer_size, &header, &value)) 
	{
		Logger.Log(ERROR, "Failed to acquire mime header from remote peer during handshake, closing connection");
		return 0;
	}
		
	/* ok, process here predefined headers */
	if (ws_conn_check_mime_header_repeated (header, value, "Host", hs->host_name))
		return 0;
	if (ws_conn_check_mime_header_repeated (header, value, "Upgrade", INT_TO_PTR (hs->upgrade_websocket))) 
		return 0;
	if (ws_conn_check_mime_header_repeated (header, value, "Connection", INT_TO_PTR (hs->connection_upgrade))) 
		return 0;
	if (ws_conn_check_mime_header_repeated (header, value, "Sec-WebSocket-Key", hs->websocket_key)) 
		return 0;
	if (ws_conn_check_mime_header_repeated (header, value, "Origin", hs->origin)) 
		return 0;
	if (ws_conn_check_mime_header_repeated (header, value, "Sec-WebSocket-Protocol", hs->protocols)) 
		return 0;
	if (ws_conn_check_mime_header_repeated (header, value, "Sec-WebSocket-Version", hs->websocket_version)) 
		return 0;
	
	/* set the value if required */
	if (strcasecmp (header, "Host") == 0)
		hs->host_name = value;
	else if (strcasecmp (header, "Sec-Websocket-Key") == 0)
		hs->websocket_key = value;
	else if (strcasecmp (header, "Origin") == 0)
		hs->origin = value;
	else if (strcasecmp (header, "Sec-Websocket-Protocol") == 0)
		hs->protocols = value;
	else if (strcasecmp (header, "Sec-Websocket-Version") == 0)
		hs->websocket_version = value;
	else if (strcasecmp (header, "Upgrade") == 0) {
		hs->upgrade_websocket = 1;
		free (value);
	} else if (strcasecmp (header, "Connection") == 0) {
		hs->connection_upgrade = 1;
		free (value);
	} else {
		/* release value, no body claimed it */
		free (value);
	} /* end if */
	
	/* release the header */
	free(header);

	return 1; /* continue reading lines */
}

#if 0
bool ws_base64_encode (const char  * content, 
				  int           length, 
				  char        * output, 
				  int         * output_size)
{
	BIO     * b64;
	BIO     * bmem;
	BUF_MEM * bptr;

	if (content == NULL || output == NULL || length <= 0 || output_size == NULL)
		return false;
	
	/* create bio */
	b64  = BIO_new (BIO_f_base64());
	bmem = BIO_new (BIO_s_mem());
	
	/* push */
	b64  = BIO_push(b64, bmem);
	
	if (BIO_write (b64, content, length) != length) {
		BIO_free_all (b64);
		return false;
	}

	if (BIO_flush (b64) != 1) {
		BIO_free_all (b64);
		return false;
	}

	/* now get content */
	BIO_get_mem_ptr (b64, &bptr);
	
	/* check output size */
	if ((*output_size) < bptr->length) {
		BIO_free_all (b64);

		*output_size = bptr->length;
		return false;
	}

	memcpy(output, bptr->data, bptr->length - 1);
	output[bptr->length-1] = 0;

	BIO_free_all (b64);

	return true;
}
#endif

char * ws_conn_produce_accept_key (const char * websocket_key)
{
	const char * static_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";	
	char       * accept_key;	
	int          accept_key_size;
	int          key_length;

	if (websocket_key == NULL)
		return NULL;
	
	key_length  = strlen (websocket_key);
	accept_key_size = key_length + 36 + 1;
	accept_key      = (char*)calloc (1, accept_key_size);

	memcpy (accept_key, websocket_key, key_length);
	memcpy (accept_key + key_length, static_guid, 36);

	//Logger.Log(ERROR, "base key value: %s", accept_key);
#if 0
	/* now sha-1 */
	unsigned char   buffer[EVP_MAX_MD_SIZE];
	unsigned int    md_len = EVP_MAX_MD_SIZE;
	EVP_MD_CTX      mdctx;
	const EVP_MD  * md = NULL;
	md = EVP_sha1 ();
	EVP_DigestInit (&mdctx, md);
	EVP_DigestUpdate (&mdctx, accept_key, strlen (accept_key));
	EVP_DigestFinal (&mdctx, buffer, &md_len);
#else
	unsigned char   buffer[SHA1_DIGEST_SIZE];
	unsigned int    md_len = SHA1_DIGEST_SIZE;
    SHA1_CTX context;
    memset(&context, 0x00, sizeof(SHA1_CTX));
    sat_SHA1_Init(&context);
    sat_SHA1_Update(&context, (const uint8*)accept_key, strlen((char*)accept_key));
    sat_SHA1_Final(&context, buffer);
#endif

	//Logger.Log(ERROR, "Sha-1 length is: %u", md_len);
	/* now convert into base64 */
#if 0
	if (! ws_base64_encode ((const char *) buffer, md_len, (char *) accept_key, &accept_key_size)) {
		Logger.Log(ERROR, "Failed to base64 sec-websocket-key value..");
		return NULL;
	}
#else
	if ( 0 != base64_encode ((unsigned char *) accept_key, &accept_key_size, (unsigned char *) buffer, md_len)) {
		Logger.Log(ERROR, "Failed to base64 sec-websocket-key value..");
		return NULL;
	}
	
#endif
	Logger.Log(NOTIC, "Returning Sec-Websocket-Accept: %s", accept_key);
	
	return accept_key;
	
}



bool ws_handshake_check (struct Handshake *hs, char * reply)
{

	char      * accept_key;

	/* call to check listener handshake */
	//Logger.Log(ERROR, "Checking client handshake data..");

	/* ensure we have all minumum data */
	if (! hs->upgrade_websocket ||
	    ! hs->connection_upgrade ||
	    ! hs->websocket_key ||
	    ! hs->origin ||
	    ! hs->websocket_version) {
		Logger.Log(NOTIC, "Client from didn't provide all websocket handshake values required, closing session (Upgraded: websocket %d, Connection: upgrade%d, Sec-WebSocket-Key: %p, Origin: %p, Sec-WebSocket-Version: %p)",
			    hs->upgrade_websocket,
			    hs->connection_upgrade,
			    hs->websocket_key,
			    hs->origin,
			    hs->websocket_version);
		return false;
	} /* end if */

	/* check protocol support */
	if (strtod (hs->websocket_version, NULL) != DEFAULT_PROT_VER)
	{
		Logger.Log(ERROR,"Received request for an unsupported protocol version: %s, expected: %d",
			    hs->websocket_version, DEFAULT_PROT_VER);
		return false;
	} /* end if */


	/* produce accept key */
	accept_key = ws_conn_produce_accept_key (hs->websocket_key);
	
	/* ok, send handshake reply */
	if (hs->protocols) 
	{
		/* send accept header accepting protocol requested by the user */
		snprintf(reply, 4095, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\nSec-WebSocket-Protocol: %s\r\n\r\n", 
					      accept_key, hs->protocols);
	} 
	else 
	{
		/* send accept header without telling anything about protocols */
		snprintf (reply, 4095, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", 
					      accept_key);
	}
		
	free(accept_key);
	
	return true; /* signal handshake was completed */
}


int ws_parse_handshake(const char* input_frame, int32& dwsize, struct Handshake *hs, char *reply)
{
	Logger.Log(INFO,"input ws_parse_handshake");
	char HttpHeader[4096] = {0};
	if( dwsize > (int)sizeof(HttpHeader) )
	{
		Logger.Log(ERROR,"input_len < sizeof(HttpHeader", dwsize, sizeof(HttpHeader));
		return ERR_NO_MORE_DATA;
	}
	strncpy(HttpHeader, input_frame, dwsize);
	char line[1024] = {0};
	int nRet = 0;
	while( ( nRet = ws_getline_from_buf(HttpHeader, line, sizeof(line), &hs->headlen)) > 0 )
	{
		if (nRet == 2 && ws_ncmp (line, "\r\n", 2))break;
		if( !ws_complete_handshake(hs, line, nRet ))
		{
			Logger.Log(ERROR,"ws_complete_handshake failed");
			return ERR_FAILED;
		}
		memset(line, 0x00, sizeof(line));
	}
	if( !ws_handshake_check(hs, reply))
	{
		Logger.Log(ERROR,"ws_handshake_check failed");
		return ERR_FAILED;
	}
	hs->handshake_ok = true;
	dwsize -= hs->headlen;
	return ERR_SUCCESS;
}


int ws_get_bit (char byte, int position) {
	return ( ( byte & (1 << position) ) >> position);
}

void ws_mask_content ( char * payload, int payload_size, char * mask)
{
	int iter       = 0;
	int mask_index = 0;

	while (iter < payload_size) {
		/* rotate mask and apply it */
		mask_index = (iter) % 4;
		payload[iter] ^= mask[mask_index];
		iter++;
	} /* end while */

	return;
} 
int ws_get_msg(char* input_frame, int32& dwsize, struct Handshake *hs, struct Frame* fm)
{
	if( !hs->handshake_ok )
	{
		Logger.Log(ERROR, "handshake_ok(%d)", hs->handshake_ok);
		return ERR_FAILED;
	}
	/*
	  0                   1                   2                   3
	  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	  +-+-+-+-+-------+-+-------------+-------------------------------+
	  |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
	  |I|S|S|S|  (4)  |A|     (7)     |             (16/63)           |
	  |N|V|V|V|       |S|             |   (if payload len==126/127)   |
	  | |1|2|3|       |K|             |                               |
	  +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
	  |     Extended payload length continued, if payload len == 127  |
	  + - - - - - - - - - - - - - - - +-------------------------------+
	  |                               |Masking-key, if MASK set to 1  |
	  +-------------------------------+-------------------------------+
	  | Masking-key (continued)       |          Payload Data         |
	  +-------------------------------- - - - - - - - - - - - - - - - +
	  :                     Payload Data continued ...                :
	  + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
	  |                     Payload Data continued ...                |
	  +---------------------------------------------------------------+
	*/
	
	if( dwsize < 2  )//FIN,OP , isMARK,len + MASK
	{
		//no much data
		Logger.Log(ERROR, "no much data dwsize[%d]", dwsize);
		return ERR_NO_MORE_DATA;
	}
	char* buffer = input_frame;
	fm->has_fin      = ws_get_bit (buffer[0], 7);
	fm->op_code      = buffer[0] & 0x0F;
	fm->is_masked    = ws_get_bit (buffer[1], 7);
	fm->payload_size = buffer[1] & 0x7F;
	fm->header_size = 0;
	if( !fm->is_masked )
	{
		Logger.Log(ERROR, "Received websocket frame with mask bit set to zero, closing buf[%x %x]",buffer[0], buffer[1]);
		return ERR_FAILED;
	}
	
	if (fm->payload_size < 0) 
	{
		Logger.Log(ERROR, "Received wrong payload size at first 7 bits, closing ");
		return ERR_FAILED;
	} /* end if */

	fm->header_size = 2;
	buffer += fm->header_size;
	if (fm->payload_size < 126) 
	{
		
		/* nothing to declare here */
	}
	else if (fm->payload_size == 126)
	{
		//FIN,OP , isMARK,7bitlen + 2bitlen + MASK
		if( dwsize < 2 + 2 + 2 + 4 )//FIN,OP + MARK,len
		{
			//no much data
			Logger.Log(ERROR, "no much data dwsize[%d]", dwsize);
			return ERR_NO_MORE_DATA;
		}
		fm->payload_size = ntohs(*((unsigned short*)buffer));
		fm->header_size += 2; //64bit len
		buffer += 2;
		
	} 
	else if (fm->payload_size == 127)
	{	
		//FIN,OP , isMARK,7bitlen + 64bitlen + MASK
		if( dwsize < 2 + 2  +  8 + 4 )
		{
			//no much data
			Logger.Log(ERROR, "no much data dwsize[%d]", dwsize);
			return ERR_NO_MORE_DATA;
		}
	
		fm->payload_size = ntohll(*((long int*)buffer));
		
		fm->header_size += 8; //64bit len
		buffer += 8;
		
	} /* end if */

	if (fm->op_code == NOPOLL_PONG_FRAME)
	{
		Logger.Log(ERROR, "PONG received over connection");
		return ERR_FAILED;
	} /* end if */

	if (fm->op_code == NOPOLL_CLOSE_FRAME)
	{
		Logger.Log(ERROR, "Proper connection close frame ");
		return ERR_FAILED;
	} /* end if */

	if (fm->op_code == NOPOLL_PING_FRAME)
	{
		Logger.Log(NOTIC, "PING received over connection");

		/* call to send pong */
		dwsize =  fm->header_size;
		return ERR_SUCCESS;
	} /* end if */

	memcpy(fm->mask, buffer, 4);
	
	
	if (fm->payload_size == 0)
	{
		Logger.Log(ERROR,"Found incoming frame with payload size 0, shutting down");
		return ERR_FAILED;
	}
	fm->header_size += 4; //64bit len
	buffer += 4;

	if( dwsize - fm->header_size < fm->payload_size )
	{
		Logger.Log(ERROR,"input_len(%u)-header_size(%u)[%u] < fm->payload_size[%llu]", dwsize , fm->header_size, dwsize - fm->header_size, fm->payload_size);
		return ERR_NO_MORE_DATA;
	}

	/*
	Logger.Log(NOTIC,"Frame header fin[%d] op[%d] ismark[%d] header_size[%d] length[%llu]  mark[%p]", 
				fm->has_fin, fm->op_code, fm->is_masked, fm->header_size, fm->payload_size, *((uint32*)fm->mask));
	*/	
	ws_mask_content(buffer, fm->payload_size, fm->mask);
	//dumpmsg(buffer, fm->payload_size);
	dwsize =  fm->header_size;
	return ERR_SUCCESS;
} 
