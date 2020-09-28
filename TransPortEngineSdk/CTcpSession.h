#pragma once

#ifndef _WINDOWS
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
using std::string;

#define DWORD unsigned long
#define MAX_EVENT_NUM 1000
#else
#include "vld.h"
#include <winsock2.h>
#include <MSWSock.h>
#include <string>
#include <process.h>
using std::string;
#endif
#include "SSAutoLock.h"
#include <vector>
#include <iostream>
#include <deque>
#include <time.h>
using std::vector;
using std::deque;
using namespace std;
#include "TransPortEngineSdk.h"
#define HEARTBEAT_TIMEOUT 20

#define IP_LEN     32
#define DIGEST_LEN 40
#define HEAD_TYPE_LEN 2

#include "LogCommSdk.h"

enum enPotocolType
{
	TCP_PROTOCOL,
	UDP_PROTOCOL
};

#define HEART_BEAT "HB"
#define RECV_DATA  "DT"

typedef struct HeadMsg
{
	char     szHeadType[HEAD_TYPE_LEN];
	uint16_t uSaltValue;
	int      nDataLen;
}HEAD_MSG_T, *LP_HEAD_MSG_T;



typedef struct NetFiveElement
{
	int    nPotocolType;
	string strRemoteIp;
	int    nRemotePort;
	string strLocalIp;
	int    nLocalPort;
}NET_FIVE_ELEMENT_T, *LP_NET_FIVE_ELEMENT_T;

#define MAX_BUFFER_LEN 4096

typedef enum OperationType
{
	IO_UNKNOWN,        //δ֪IO״̬
	IO_ACCEPT,         //����IO״̬
	IO_RECV,           //����IO״̬
	IO_SEND,           //����IO״̬
	IO_CONNECT,        //����IO״̬
	IO_DISCONNECT      //�Ͽ�����IO״̬
}OPERATION_TYPE;

#ifndef _WINDOWS

typedef struct stRecvCache
{
	int nRecvLen;
	char szRecvBuf[MAX_BUFFER_LEN];
}RECV_CACHE_T, *LP_RECV_CACHE_T;
#else
typedef struct stPer_Io_Context_Recv
{
	OVERLAPPED     Overlapped;               // ÿһ���ص�I/O���������Ҫ��һ��                
	SOCKET         sockRecv;                 // ���I/O������ʹ�õ�Socket��ÿ�����ӵĶ���һ����  
	WSABUF         wsaBuf;                   // �洢���ݵĻ��������������ص��������ݲ����ģ�����WSABUF���滹�ὲ  
	OPERATION_TYPE OpType;                   // ��־����ص�I/O��������ʲô�ģ�����Accept/Recv��
	char           szBuffer[MAX_BUFFER_LEN]; // ��ӦWSABUF��Ļ�����  
	DWORD          dwRecvLen;                //�������ݳ���
}PER_IO_CONTEXT_RECV_T, *LP_PER_IO_CONTEXT_RECV_T;

typedef struct stPer_Io_Context_Send
{
	OVERLAPPED     Overlapped;               // ÿһ���ص�I/O���������Ҫ��һ��                
	SOCKET         sockSend;                 // ���I/O������ʹ�õ�Socket��ÿ�����ӵĶ���һ����  
	WSABUF         wsaBuf;                   // �洢���ݵĻ��������������ص��������ݲ����ģ�����WSABUF���滹�ὲ  
	OPERATION_TYPE OpType;                   // ��־����ص�I/O��������ʲô�ģ�����Accept/Recv��
	char*          szBuffer;                 // ��ӦWSABUF��Ļ�����  
	DWORD          dwSendLen;                //�������ݳ���
	int            nSquence;
}PER_IO_CONTEXT_SEND_T, *LP_PER_IO_CONTEXT_SEND_T;
#endif
typedef struct stBufInfo
{
	char *szBuf;
	int   nBufLen;
	stBufInfo()
	{
		szBuf = NULL;
		nBufLen = 0;
	};
}BUFINFO_T, *LP_BUFINFO_T;

class CTcpSession
{
public:
	CTcpSession();
	~CTcpSession();

#ifndef _WINDOWS
	class INetEventModleEpoll
	{
	public:
		virtual int NetEventModleEpoll_AddEvent(CTcpSession* pTcpSession) = 0;
		virtual int NetEventModleEpoll_ModEvent(CTcpSession* pTcpSession) = 0;
		virtual int NetEventModleEpoll_DelEvent(CTcpSession* pTcpSession) = 0;
		virtual int NetEventModleEpoll_GetEpoll() = 0;
	};
	int TcpSession_Init(int nSocket, INetEventModleEpoll* pINetEventModleEpoll, LP_NET_FIVE_ELEMENT_T pNetFiveElement, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, void *pUser);
	int TcpSession_GetSocket();
	bool TcpSession_IsSendData();
	CCritSec* TcpSession_GetEpollEventLock();
#else
	int TcpSession_Init(SOCKET Socket, int nEventModleIndex, LP_NET_FIVE_ELEMENT_T pNetFiveElement, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, void *pUser);
	SOCKET TcpSession_GetSocket();
	LP_PER_IO_CONTEXT_RECV_T TcpSession_GetPerIoContextRecv();
	int TcpSession_CancelIoSend();
	int TcpSession_IoSendCount();
	bool TcpSession_IsToBeClose();
	CCritSec* TcpSession_GetIOCPEventLock();
#endif
	int TcpSession_Fini();
	int  TcpSession_GetNetEventModleIndex();
	int TcpSession_IOSend(int nSquence);
	int TcpSession_IORecv();
	int TcpSession_SendData(const char *szData, const int nDataLen);
	int TcpSession_IOSend(const char *szData, const int nDataLen);
	int TcpSession_IORecv(int nPostRecvEventCount);
	unsigned int TcpSession_GetRecvCacheLen();
	int TcpSession_GetRecvCacheBuf(char *szBuf);
	bool TcpSession_IsHeartbeatTimeOut();
	
private:
	int TcpSession_HeadleHB(char *szData, const int nDataLen);
private:
	LP_NET_FIVE_ELEMENT_T            m_pNetFiveElement;
#ifndef _WINDOWS
	int                              m_nSocket;
	//RECV_CACHE_T                     m_RecvCache;
	vector<LP_BUFINFO_T>             m_SendBufs;
	CCritSec                         m_SendBufLock;
	INetEventModleEpoll*             m_pINetEventModleEpoll;
	CCritSec                         m_EpollEventLock;
#else
	SOCKET                           m_Socket;
	vector<LP_PER_IO_CONTEXT_SEND_T> m_PerIoContextSends;
	CCritSec                         m_PerIoContextSendLock;
	LP_PER_IO_CONTEXT_RECV_T         m_pPerIoContextRecv;
	CCritSec                         m_IOCPEventLock;
#endif
	deque<LP_BUFINFO_T>              m_RecvBufs;
	CCritSec                         m_RecvBufLock;
	int                              m_nEventModleIndex = 0;
	int                              m_nSquence;
	time_t                           m_tHeartbeatTimeStamp = time(NULL);
	ITransPortEngineRecv             *m_pITransPortEngineRecv;
	bool                             m_bIsHaveHead;
	bool                             m_bIsRecvHead;
	unsigned int                     m_nNeedRecvDataLen = 0;
	uint16_t                         m_uSaltValue;
	int                              m_nRecvDataType;
	void*                            m_pUser;
	bool                             m_bIsToBeClose = false;
};


#ifndef _WINDOWS

#else
#endif


