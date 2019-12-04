#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <string>
#include <list>
#include <vector>
#include <map>
#include <string.h>

using namespace std;

typedef char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

typedef void (*FunCallback)(void* pParam, uint16 wFlag);

#define ntohll(x)	__bswap_64 (x)
#define htonll(x)	__bswap_64 (x)

//error code
#define ERR_SUCCESS			 0
#define ERR_FAILED			-1
#define ERR_NO_MORE_DATA	-2
#define ERR_NO_MORE_SPACE	-3
#define ERR_PROTOCOL		-4
#define ERR_FILTERMSG		-5 //��Ϣ������,����

//return code
#define RET_SUCCESS 			0x0000
#define RET_ERROR			        0x0001
#define RET_REDIRECT 			0x0002
#define RET_NOTVIP				0x0003
#define RET_LESSLV				0x0004
#define RET_ISBANNED			0x0005
#define RET_ISKICKED			0x0006
#define RET_ROOMNOTEXIST		0x0007
#define RET_USERNOTEXIST		0x0008

#define RET_NOTIN_FANSLIST		0x0021

#define RET_NOT_IN_FAMILY		0x0031

#define RET_FAILED				0x1001
#define RET_VERSION_ERROR 		0x1002
#define RET_VALIDATION_FAILED 	0x1003
#define RET_USR_LICENSE_ERROR	0x1004
#define RET_USR_EXIST			0x1005
#define RET_USR_NO_EXIST		0x1006
#define RET_USR_AV_EXIST		0x1007
#define RET_USR_AV_NO_EXIST		0x1008
#define RET_USR_FORCE_DOWN		0x1030
#define RET_UNKNOWN_ERROR 		0x1FFF
#define RET_USER_OFFLINE		0x5001	//����
#define RET_USER_BUSY			0x5002	//æµ

#if  defined(__CONN__) || defined(__MSGCENT__)
#define MAX_BUFFER_SIZE			32768	//32K
#else
#define MAX_BUFFER_SIZE			(1024*256)	//256K
#endif

#ifdef __CONN__
#define MAX_SOCKET_NUM			10000
#define MAX_CHECK_TIMEOUT		300		//5min
#define MAX_MODULE_TIMEOUT		120
#else
#define MAX_SOCKET_NUM			200
#define MAX_CHECK_TIMEOUT		300		//5min
#endif

#define MAX_TIME_OUT			5
#define MAX_HEARTBEAT			10		//10s
#define MAX_MSGPUSHINTER		10		//10s

#define IOSMSGPUSHINTER			30

#define MAX_ONLINE_NUMS			10000
#define MAX_DES_KEY				24
#define MAX_DEVICE_ID_LEN		33
#define MAX_LICENSE_LEN			33
#define MAX_PASSWD_LEN			33
#define MAX_PLTTYPE_LEN			33
#define MAX_MODEL_LEN			33
#define MAX_ADD_DATA_LEN		(5*1024)
#define MAX_AD_LEN				17
#define MAX_REASON_LEN			33
#define MAX_NICKNAME_LEN		33
#define MAX_TEL_LEN				30
#define MAX_FILTE_WORLD_SIZE	200 //###max list, map size
//#define MAX_SHUT_USERINFO_SIZE	1000 
#define MAX_ROOMID_LIST_SIZE	200
#define MAX_ROOMINFO_LIST_SIZE	200
#define MAX_CONNMODULE_LIST_SIZE	50 //connectģ������
#define MAX_ROOM_AuditMsg_LIST_SIZE	100 //������ϱ���Ϣ����
#define MAX_UID_LIST_SIZE	200 //�û��б���֧���û���
#define MAX_AuditMsg_SEND_INTERVAL 157000 //΢��
#define MAX_IOS_USERLIST_SIZE	100000

#define MAX_TITLE_COUNT			10
#define MAX_SHUT_COUNT			5
#define MAX_GET_ULIST_SIZE		20 //һ��ȡ�б�����û���
#define MAX_MSG_LEN				1024
#define MAX_SAVEMSG_COUNT		3 //ÿ�����䱣��������Ϣ����Ŀ
#define SYNC_HEARTBEAT_COUNT		6	//chatsvr��ʱͬ��������Ϣ��DC��ʱ����(��������,10��һ��)
#define VISTORSYNC_BASE_TIME	5
//protocol version
#define PROTOCOL_VERSION_0x1001 0x1001			//�޼���
#define PROTOCOL_VERSION_0x2001 0x2001			//�޼���
#define PROTOCOL_VERSION_0x2002 0x2002			//base64
#define PROTOCOL_VERSION_0x2003 0x2003			//aes
#define PROTOCOL_VERSION_0x2004 0x2004			//gzip
#define PROTOCOL_VERSION_0x2005 0x2005			//aes + gzip

#define PROTOCOL_HEAD_LEN 		32

//compiled config
#define CFG_OBJECT_POOL
//#define CFG_SYNC_VC

//flag
#define FLAG_CMD_CTRL			0x4000

//id
#define ID_SYS_CTRL				0x0000
#define ID_SYS_USER				0x0001

#define ID_SYS_CN				1000
#define	ID_SYS_UM				2000
#define ID_SYS_CS				3000

#define  MOD_YD_POINT			    40001 //�ƶ������
#define  MOD_LT_POINT			    43001 //��ͨ�����
#define  MOD_DX_POINT			    45001 //���Ž����

#define MAX_SEND_MSG_INTERVAL	2
#define MAX_ROOMINFO_LIST_SIZE	200

#define XIAOI_USER 		1900051

const uint16 CMD_USER_LOGIN = 0x0000;  //�û���¼
const uint16 CMD_USER_LOGIN_ACK = 0x0001;  //�û���¼��Ӧ
const uint16 CMD_USER_LOGOUT = 0x0002;  //�û�ע��
const uint16 CMD_USER_LOGOUT_ACK = 0x0003;  //�û�ע����Ӧ
const uint16 CMD_USER_FORCE_DOWN = 0x0004;  //�û���������֪ͨ
const uint16 CMD_ROOM_NTF_USER_REFRESH = 0x0005;  //�������û���Ϣ����֪ͨ
const uint16 CMD_USER_VOD_REQ = 0x0006	;  //�û����
const uint16 CMD_USER_VOD_REQ_ACK = 0x0007;  //�û������Ӧ

const uint16 CMD_USER_VOD_ACCEPT = 0x0008;  //�������ȷ��
const uint16 CMD_USER_VOD_ACCEPT_ACK = 0x0009;  //�������ȷ����Ӧ
const uint16 CMD_USER_HEARTBEAT = 0x000A;  //�ͻ�������
const uint16 CMD_USER_HEARTBEAT_ACK = 0x000B;  //������Ӧ
const uint16 CMD_USER_MSG_SEND = 0x000C;  //�û���������Ϣ
const uint16 CMD_USER_MSG_SEND_ACK = 0x000D;  //�û���������Ϣ��Ӧ
const uint16 CMD_USER_MSG_RECV = 0x000E;  //�û�������Ϣ
const uint16 CMD_USER_REFRESH = 0x0010;  //ˢ���û���Ϣ
const uint16 CMD_USER_REFRESH_ACK = 0x0011;  //ˢ���û���Ϣ
const uint16 CMD_USER_VOD_RECV = 0x0012;  //�����յ��û��������
const uint16 CMD_USER_VOD_ACCEPT_NTF = 0x0013;  //����ȷ��֪ͨ
const uint16 CMD_USER_WHISPER_SEND = 0x0014;  //�û��������Ļ�
const uint16 CMD_USER_WHISPER_SEND_ACK = 0x0015;  //�û��������Ļ���Ӧ
const uint16 CMD_ROOM_LOGIN = 0x0106;  //���뷿��
const uint16 CMD_ROOM_LOGIN_ACK = 0x0107;  //���뷿����Ӧ
const uint16 CMD_ROOM_LOGOUT = 0x0108;  //�˳�����
const uint16 CMD_ROOM_LOGOUT_ACK = 0x0109;  //�˳�������Ӧ
const uint16 CMD_ROOM_GET_USER_LISTINFO = 0x010A;  //��ȡ�����û���Ϣ�б�
const uint16 CMD_ROOM_GET_USER_LISTINFO_ACK = 0x010B;  //��ȡ�����û���Ϣ�б���Ӧ
const uint16 CMD_ROOM_STAT_REFRESH = 0x010C;  //����ֱ��״̬����
const uint16 CMD_ROOM_STAT_REFRESH_ACK = 0x010D;  //����ֱ��״̬������Ӧ
const uint16 CMD_ROOM_NTF_USERLIST_CHG = 0x010E;  //�û���¼����֪ͨ
const uint16 CMD_ROOM_LOCK_NTF= 0x010F;  //��������֪ͨ
const uint16 CMD_ROOM_NTF_STAT_REFRESH = 0x0110;  //�������Ը���֪ͨ
const uint16 CMD_ROOM_VCUSER_STATIC = 0x0111;  //VC�û��ڷ�������ͳ��
const uint16 CMD_ROOM_BAN_USER_MSG = 0x0112;  //��ֹ�û�����
const uint16 CMD_ROOM_BAN_USER_MSG_ACK = 0x0113;  //��ֹ�û�������Ӧ
const uint16 CMD_ROOM_LOCK = 0x0114;  //��������
const uint16 CMD_ROOM_LOCK_ACK = 0x0115;  //����������Ӧ

const uint16 CMD_ROOM_BAN_USER_NTF = 0x0116;  //��ֹ�û�����֪ͨ
const uint16 CMD_ROOM_KICKOFF_USER = 0x0118;  //�߳�����
const uint16 CMD_ROOM_KICKOFF_USER_ACK = 0x0119;  //�߳�������Ӧ
const uint16 CMD_ROOM_KICKOFF_USER_NTF = 0x011A;  //�߳�����֪ͨ

#ifdef XIAOI
const uint16 CMD_ROOM_XIAOIMSG			=	0x01a0;  //xiaoi msg
#endif
const uint16 CMD_WEBUSER_HTTPRESP		= 0x0210;	//WEB_USER handshake resp;
const uint16 CMD_WEBUSER_PONG 			= 0x0211;
const uint16 CMD_WEBUSER_ECHO			= 0x0212;

const uint16 CMD_GET_ROOM_ATTR					= 0x0500;	
const uint16 CMD_GET_ROOM_ATTR_ACK				= 0x0501;	
const uint16 CMD_CS_GET_ROOM_ANNC					= 0x0502;	// 	��ȡ���乫��
const uint16 CMD_CS_GET_ROOM_ANNC_ACK				= 0x0503;	// 	
const uint16 CMD_CS_GET_FILTERWORDS					= 0x0504;	//��ȡ�������б�
const uint16 CMD_CS_GET_FILTERWORDS_ACK				= 0x0505;	//
const uint16 CMD_CS_GET_ROOM_LIMITLIST					= 0x0506;	// 	��ȡ�������/�����б�
const uint16 CMD_CS_GET_ROOM_LIMITLIST_ACK				= 0x0507;	//
const uint16 CMD_CS_ROOMSTAT_SYNC					= 0x050A;	//ͬ������ֱ��״̬
const uint16 CMD_CS_ROOMSTAT_SYNC_ACK				= 0x050B;	//
const uint16 CMD_CS_LIMITSTAT_SYNC					= 0x050C;	//����Ȩ�޲���ͬ��
const uint16 CMD_CS_LIMITSTAT_SYNC_ACK				= 0x050D;	//
const uint16 CMD_CS_GETUSERINFO						= 0x050E;
const uint16 CMD_CS_GETUSERINFO_ACK					= 0x050F;
const uint16 CMD_SEND_AUDITMSG						= 0x0510;// 	�����ϱ���δ����Ϣ
const uint16 CMD_SEND_AUDITMSG_ACK					= 0x0511;
const uint16 CMD_CS_GETUSEREACHFANS					= 0x0512;
const uint16 CMD_CS_GETUSEREACHFANS_ACK				= 0x0513;

const uint16 CMD_CS_GET_IOS_USERINFO				= 0x0532;
const uint16 CMD_CS_GET_IOS_USERINFO_ACK			= 0x0533;


const uint16 CMD_CS_GET_ROOM_USERCOUNT					= 0x0600;	// 	��ȡ�����û�����
const uint16 CMD_CS_GET_ROOM_USERCOUNT_ACK				= 0x0601;	//
const uint16 CMD_CS_ROOM_LOCK					= 0x0602;	// 	��������Ա��̨���շ���
const uint16 CMD_CS_ROOM_LOCK_ACK				= 0x0603;	// 	
const uint16 CMD_CS_ROOM_TITLE_SYNC					= 0x0604;	//����֮��(ͷ��ͬ��)
const uint16 CMD_CS_ROOM_TITLE_SYNC_ACK				= 0x0605;	//
const uint16 CMD_CS_SEND_SYSMSG					= 0x0606;	// 	CS����ϵͳ��Ϣ
const uint16 CMD_CS_SEND_SYSMSG_ACK				= 0x0607;	//
const uint16 CMD_CS_FILTERWORDS_SYNC					= 0x0608;	// 	ͬ���������б�
const uint16 CMD_CS_FILTERWORDS_SYNC_ACK				= 0x0609;	//
const uint16 CMD_CS_ROOM_VISTERCOUNT_SYNC					= 0x060A;	// 	ͬ���������б�
const uint16 CMD_CS_ROOM_VISTERCOUNT_SYNC_ACK				= 0x060B;	//
const uint16 CMD_CS_APPOINT_ROOMADMIN					= 0x060C;	// 	��������Ա
const uint16 CMD_CS_APPOINT_ROOMADMIN_ACK				= 0x060D;	//
const uint16 CMD_CS_USER_REFRESH_BATCH					= 0x060E;	// 	�û���Ϣˢ�£�������
const uint16 CMD_CS_USER_REFRESH_BATCH_ACK				= 0x060F;	//
const uint16 CMD_CS_USER_REFRESH               = 0x0610;
const uint16 CMD_CS_USER_REFRESH_ACK          = 0x0611;
const uint16 CMD_CS_ISSUED_AUDITMSG               = 0x0612;// 	CS�����·����������Ϣ
const uint16 CMD_CS_ISSUED_AUDITMSG_ACK          = 0x0613;
const uint16 CMD_CS_USER_LIMIT_ALL					= 0x0614;	//  	ȫ�������/�߳�
const uint16 CMD_CS_USER_LIMIT_ALL_ACK				= 0x0615;	//
const uint16 CMD_CS_ROOM_NPC_LOGIN					= 0x0618;	//	NPC���뷿��
const uint16 CMD_CS_ROOM_NPC_LOGIN_ACK				= 0x0619;	//	NPC�˳�����
const uint16 CMD_CS_IM_P2P_SYSMSG					= 0x0630;	//IMȺ��P2Pϵͳ��Ϣ
const uint16 CMD_CS_IM_P2P_SYSMSG_ACK				= 0x0631;	//IMȺ��P2Pϵͳ��Ϣack
const uint16 CMD_CS_IM_FAMILY_SYSMSG				= 0x0632;	// 	IMȺ������ϵͳ��Ϣ
const uint16 CMD_CS_IM_FAMILY_SYSMSG_ACK			= 0x0633;	// 	IMȺ������ϵͳ��ϢACK
const uint16 CMD_CS_MUTUALFANS_REFRESH 				= 0x0634;
const uint16 CMD_CS_MUTUALFANS_REFRESH_ACK			= 0x0635;
const uint16 CMD_CS_SYNC_IOSUSERINFO				= 0x0636;
const uint16 CMD_CS_SYNC_IOSUSERINFO_ACK			= 0x0637;


const uint16 CMD_MODULE_LOGIN					= 0x5000;
const uint16 CMD_MODULE_LOGIN_ACK				= 0x5001;
const uint16 CMD_GET_ROOM_POS					= 0x5002;
const uint16 CMD_GET_ROOM_POS_ACK				= 0x5003;
const uint16 CMD_SYS_HEARTBEAT					= 0x5004;
const uint16 CMD_SYS_HEARTBEAT_ACK				= 0x5005;
const uint16 CMD_SYNC_ROOM_INFO					= 0x5006;//������Ϣͬ��
const uint16 CMD_SYNC_ROOM_INFO_ACK				= 0x5007;//������Ϣͬ������



const uint16 CMD_SYS_CLEAN_ROOM					= 0x5008;
const uint16 CMD_ROOM_USER_DUMP					= 0x5009;
const uint16 CMD_SYS_VISITOR_CALC				= 0x500a;
const uint16 CMD_SYS_RESET_IDC					= 0x500b;
const uint16 CMD_SYS_CLEAR_ROOM_POS 			= 0x500c;
const uint16 CMD_SYS_CLEAN_ROOM_USER			= 0x500d;
const uint16 CMD_SYS_SYNC_ANCHOR_NOTIFY			= 0x500e;

const uint16 CMD_LOGIN_FAMILY					= 0x500f;
const uint16 CMD_LOGIN_FAMILY_ACK				= 0x5010;
const uint16 CMD_LOGOUT_FAMILY					= 0x5011;
const uint16 CMD_LOGOUT_FAMILY_ACK				= 0x5012;
const uint16 CMD_FAMILY_CREATE					= 0x5013;
const uint16 CMD_FAMILY_CREATE_ACK				= 0x5014;

const uint16 CMD_MODULE_CLOSE					= 0x5015;
const uint16 CMD_GET_P2PMSG						= 0x5016;
const uint16 CMD_NEW_HEARTBEAT					= 0x5017;
const uint16 CMD_SYNC_FAMILY_INFO				= 0x5018;

const uint16 CMD_GET_EACHFANS					= 0x5019;
const uint16 CMD_SYNC_EACHFANS					= 0x501a;
const uint16 CMS_TOSYNC_ROOMINFO				= 0x501b;
const uint16 CMD_RESET_LOGINUSER				= 0x501c;

const uint16 CMD_SYNC_IOS_LOGIN					= 0x5100;
const uint16 CMD_SYNC_IOS_LOGOUT				= 0x5101;
const uint16 CMD_SYNC_IOS_USERINFO				= 0x5102;
const uint16 CMD_RECONN_IOSMSGCENTER			= 0x5103;

const uint16 CMD_UNKNOW = 0x0FFF;

const uint16 CMD_FORWARD_REQUEST			= 0x0800;//ת������
const uint16 CMD_FORWARD_REQUEST_ACK		= 0x0801;

const uint16 CMD_SYS_CLOSE_USER 				= 0x1000;
const uint16 CMD_SYS_CLOSE_MOD 					= 0x1001;


const uint16 CMD_SYS_MOD_CONN	 				= 0x2000;
const uint16 CMD_SYS_MOD_CLOSE_USER 			= 0x2001;
const uint16 CMD_SYS_SVR_CLOSE_USER 			= 0x2002;

const uint64 ID_ROOM_FLAG = (0x01ull << 63);
const uint32 ID_USER_FLAG = (0x01u << 31);

#define  CMD_INNER_FLAGE        (0x1 << 15)


//////////////// FOR VC 

const uint16 CMD_VC_USER_ROOMLOGIN			= 0x0900;
const uint16 CMD_VC_USER_ROOMLOGIN_ACK		= 0x0901;
const uint16 CMD_VC_USER_ROOMLOGOUT			= 0x0902;
const uint16 CMD_VC_USER_ROOMLOGOUT_ACK		= 0x0903;

const uint16 CMD_VC_USER_CLEAN				= 0x09ff; //agent��������VC�û�

#define  AGENT_USER_FLAGE		(0x1 << 30) //VC�û�ID��־λ

//////////////// FOR AGENT
#define  VC_USER_FLAGE		(0x1 << 30) //agentģ��ʹ�� 


#define CMD_NWE_HEARTBEAT 					0x1b96615d
////////////////////////////for P2P MSG
const uint16 CMD_IM_P2P_MSGSEND				 = 0x0300;
const uint16 CMD_IM_P2P_MSGSEND_ACK 		 = 0x0301;
const uint16 CMD_IM_P2P_MSGRECV				 = 0x0302;
const uint16 CMD_IM_P2P_MSGRECV_ACK			 = 0x0303;

//////////////////////////for Family Msg
const uint16 CMD_IM_FAMILY_MSGSEND			 = 0x0320;
const uint16 CMD_IM_FAMILY_MSGSEND_ACK		 = 0x0321;
const uint16 CMD_IM_FAMILY_MSGRECV			 = 0x0322;
const uint16 CMD_IM_FAMILY_GETRECORD		 = 0x0325;
///////////////////////////

enum ENCRYPT_TYPE
{
	NO_ENCRYPT			= 0x0000,	
	ENCRYPT_BASE64		= 0x0001,	
	ENCRYPT_AES			= 0x0002,
	ENCRYPT_GZIP		= 0x0003,
	ENCRYPT_AESGZIP		= 0x0004
};


enum IDC_TYPE
{
	IT_YD		= 0x01,	
	IT_DX		= 0x02,	
	IT_LT		= 0x03,	
	IT_UNKNOW	= 0x04	
};

enum REFRESH_TYPE
{
	IT_USERINFO		= 0x01,	//�û���Ϣ
	IT_PRIVILEGE		= 0x02,	//��Ȩ
	IT_LIVESTAT	    = 0x03,	//ֱ��
};


enum MSG_TYPE
{
	MT_NORMAL		= 0x01,	
	MT_BROADCAST 	= 0x02
};

enum IO_TYPE
{
	IO_UDP_AUDIO_RTP = 0, 
	IO_UDP_AUDIO_RTCP, 
	IO_UDP_VIDEO_RTP, 
	IO_UDP_VIDEO_RTCP, 
	IO_TCP,
	IO_HTTP
};

enum IO_STATE
{
	IO_IDLE		= 0x00,
	IO_BUSY		= 0x01,
	IO_READ		= 0x02,
	IO_READING	= 0x04,
	IO_WRITE	= 0x08,
	IO_WRITING	= 0x10,
#ifdef XIAOI
	IO_HAND		= 0x20,
	IO_HANDING	= 0x40,
#endif
	IO_FINIT		= 0x80
};

enum IO_RESULT
{
	IO_SUCCESS = 0,
	IO_FAILED,
	IO_EAGAIN,
	IO_CLOSE,
	IO_TIMEOUT,
	IO_ERROR
};

enum TIMER_TYPE
{
	TIMER_CHECK_TIMEOUT = 1,
	TIMER_SVR_RECONNECT,
	TIMER_HEARTBEAT,
	TIMER_VISITOR_CALC,
	TIMER_MSGPUSH
};

enum SERVER_TYPE
{
	ST_DATA_CENTER  = 0x01,
	ST_CHAT_SVR		= 0x02,
	ST_MSG_CENTER	= 0x03,
	ST_CONNECTOR	= 0x04,
	ST_AUTH_SVR		= 0x05,
	ST_WEB_SVR		= 0x06,
	ST_WEB_CLT		= 0x07,
	ST_BUSI_SVR		= 0x08,
	ST_V_ROOM       = 0x09,
#ifdef XIAOI
	ST_XIAOI		= 0x0b,
#endif	
	ST_V_MSG_CACHE	= 0x0a,
	ST_IOS_MSGCENTER	= 0x0c
	
};

enum CHATSERVER_MODID
{
	MODID_20001  = 20001,
	MODID_21001  = 21001//���󷿼�ID
};


enum MODULE_TYPE
{
	MT_CLIENT = 0x01,
	MT_SERVER = 0x02
};

enum WEBSVR_TYPE
{
	WEB_TYPE = 0x01,
	DB_TYPE = 0x02
};
//CS limit type
enum e_CS_LIMIT_TYPE
{
    LT_NORMAL = 0x0000,
	LT_REBAN = 0x0001,
    LT_REKICK = 0x0002,
    LT_BAN = 0x0003,
    LT_KICK = 0x0004,
    LT_EXT = 0xFFFF
};

//Msg Type
enum e_MSG_TYPE
{
    MSG_ROOMTXT = 0x0001,
    MSG_WHISPER = 0x0002,
    MSG_SYSMSG = 0x0003,
	MSG_VIPCAR = 0x0004,
    MSG_GIFT = 0x0005,
    MSG_LOTTERY = 0x0006,
	MSG_PARKING = 0x0007,
	MSG_SOFA = 0x0008,
	MSG_SCREEN = 0x0009,
	MSG_SPEAKER = 0x000a,
	MSG_VOD = 0x000b,
	MSG_VODACCEPT = 0x000c,
	MSG_SYSMSG_W  = 0x000d,
	MSG_ANCHORMSG  = 0x000e,
	MSG_ANCHORMSG_W  = 0x000f,
    MSG_SENDCLAWSMSG  = 0x0010,//��צ��	
    MSG_LEVELUPMSG  = 0x0011,//����/�����ȼ�����

	MSG_SPONSOR_PULL  = 0x0012,//������ȡ�齱����(->������)
	MSG_SPONSOR_FEEDBACK  = 0x0013,//�����߷������û�->��̨->CS->������
    MSG_DRAW_STARTS  = 0x0014,//�齱��ʼ��ʾ(������)
    MSG_WINNING_RESULTS  = 0x0015,//�н����֪ͨ
    MSG_GIFT_UPDATE  = 0x0016,//�����������
    MSG_RED_START  = 0x0017,//�������ʼ��ʾ
    MSG_RED_END  = 0x0018,//�������֪ͨ
		 
    MSG_EXT = 0xFFFF
};


//Msg Obj Type
enum e_MSGOBJ_TYPE
{
    MSO_TEXT = 0x0001,
    MSO_USER = 0x0002,
    MSO_PROPS = 0x0003,
    MSO_SONG = 0x0004,
    MSO_SOFA = 0x0005,
    MSO_PARKING = 0x0006,
    MSO_GRADE = 0x0007,//�ȼ� 
    MSO_EXPRESSION = 0x0008,//����  
    MSO_ACTIVE_OBJ = 0x0009,//����� 
    MSO_LINK = 0x0011,
    MSO_LINK_EXT = 0x0012,
    
    MSO_EXT = 0xFFFF
};

//user type
enum e_USER_TYPE
{
    UT_USER = 0x0000,
    UT_ADMIN = 0x0001,
	UT_VISITOR = 0x0002,
	UT_TMPUSER = 0x0003,
    
    UT_EXT = 0xFFFF
};

//client  type
enum e_CLIENT_TYPE
{
    CT_WEB = 0x0001,
    CT_ANDROID = 0x0002,
	CT_IPHONE = 0x0003,
	CT_IPAD = 0x0004,
	CT_VC = 0x0010,
    
    CT_EXT = 0xFFFF
};

//room lock
enum e_ROOM_LOCKFLAG
{
    RL_NORMAL = 0x0000,
    RL_LOCKED = 0x0001,
    
    RL_EXT = 0xFFFF
};

//room audit
enum e_ROOM_AUDITFLAG
{
    RL_CLOSE = 0x0000,
    RL_OPEN = 0x0001,
};


//login type
enum e_USERLIST_CHGTYPE
{
    ULC_LOGIN = 0x0001,
    ULC_LOGOUT = 0x0002,
    
    ULC_EXT = 0xFFFF
};


//user type
enum e_USERLIST_TYPE
{
    UL_ADMIN = 0x0001,
    UL_USER = 0x0002,
    
    UL_EXT = 0xFFFF
};




//limit type
enum e_CLIENT_LIMIT_TYPE
{
	CL_RESHUT = 0x0000,
	CL_SHUT = 0x0001,
    
	CL_EXT = 0xFFFF
};

enum e_ROOM_LIST_TYPE
{
    RT_BANLIST = 0x0001,
	RT_KICKLIST = 0x0002,
    RT_ROOMADMINLIST = 0x0003,
    
    RT_EXT = 0xFFFF
};

enum e_LIVE_STAT
{
    LS_LIVEOFF = 0x0000,
	LS_LIVEON = 0x0001,
	LS_CHNCHG = 0x0002,
    
    LS_EXT = 0xFFFF

};



enum e_SHUT_PURVIEW_FLAG
{
	SP_SUCCESS = 0x0000, //ADC�ɹ�
	SP_BANADC = 0x0001, //����Ա�б��е���(��������/����Ա/����) ���ܱ���
    	SP_NOTVIP = 0x0002, //����VIP,������
	SP_LESSLV = 0x0003, //�ȼ���
	SP_OVERCOUNT = 0x0004, //���쳬������
	SP_ADDCOUNT = 0x0005, //�ɹ�,����+ 1, �����û���Ϣ��CONN, �Է�����һ������
	SP_RECOUNT = 0x0006, //���ô���(=1), �����û���Ϣ��CONN
    SP_EXT = 0xFFFF
};


enum e_USER_LIMIT
{
	LIMT_ALL_ROOM_BAN = 0x0001, //ȫ�������
	LIMT_ALL_ROOM_KICK = 0x0002, //ȫ�����߳�
    LIMT_ACCOUNTS_FROZEN = 0x0004, //�ʺŶ���
	LIMT_ALL_ROOM_SHIELD = 0x0005, //ȫ�������η���.
	LIMT_RELIEVE_ALL_ROOM_BAN = 0x000b, //��ȫ�������
	LIMT_RELIEVE_ALL_ROOM_KICK = 0x000c, //��ȫ�����߳�
	LIMT_RELIEVE_ACCOUNTS_FROZEN = 0x000e, //���ʺŶ���
    LIMT_RELIEVE_ALL_ROOM_SHIELD = 0x000f					//��ȫ�������η���.
};
typedef enum {
	/** 
	 * @brief Support to model unknown op code.
	 */
	NOPOLL_UNKNOWN_OP_CODE = -1,
	/** 
	 * @brief Denotes a continuation frame.
	 */
	NOPOLL_CONTINUATION_FRAME = 0,
	/** 
	 * @brief Denotes a text frame (utf-8 content) and the first
	 * frame of the message.
	 */
	NOPOLL_TEXT_FRAME         = 1,
	/** 
	 * @brief Denotes a binary frame and the first frame of the
	 * message.
	 */
	NOPOLL_BINARY_FRAME       = 2,
	/** 
	 * @brief Denotes a close frame request.
	 */
	NOPOLL_CLOSE_FRAME        = 8,
	/** 
	 * @brief Denotes a ping frame (used to ring test the circuit
	 * and to keep alive the connection).
	 */
	NOPOLL_PING_FRAME         = 9,
	/** 
	 * @brief Denotes a pong frame (reply to ping request).
	 */
	NOPOLL_PONG_FRAME         = 10
} noPollOpCode;



enum e_CS_NPC_LOGINOUT
{
	CN_LOGIN = 0x0001,
	CN_LOGOUT = 0x0002,
    
	CN_EXT = 0xFFFF
};


#endif
