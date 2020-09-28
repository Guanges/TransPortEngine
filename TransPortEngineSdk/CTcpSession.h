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
	IO_UNKNOWN,        //未知IO状态
	IO_ACCEPT,         //接受IO状态
	IO_RECV,           //接收IO状态
	IO_SEND,           //发送IO状态
	IO_CONNECT,        //连接IO状态
	IO_DISCONNECT      //断开连接IO状态
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
	OVERLAPPED     Overlapped;               // 每一个重叠I/O网络操作都要有一个                
	SOCKET         sockRecv;                 // 这个I/O操作所使用的Socket，每个连接的都是一样的  
	WSABUF         wsaBuf;                   // 存储数据的缓冲区，用来给重叠操作传递参数的，关于WSABUF后面还会讲  
	OPERATION_TYPE OpType;                   // 标志这个重叠I/O操作是做什么的，例如Accept/Recv等
	char           szBuffer[MAX_BUFFER_LEN]; // 对应WSABUF里的缓冲区  
	DWORD          dwRecvLen;                //接收数据长度
}PER_IO_CONTEXT_RECV_T, *LP_PER_IO_CONTEXT_RECV_T;

typedef struct stPer_Io_Context_Send
{
	OVERLAPPED     Overlapped;               // 每一个重叠I/O网络操作都要有一个                
	SOCKET         sockSend;                 // 这个I/O操作所使用的Socket，每个连接的都是一样的  
	WSABUF         wsaBuf;                   // 存储数据的缓冲区，用来给重叠操作传递参数的，关于WSABUF后面还会讲  
	OPERATION_TYPE OpType;                   // 标志这个重叠I/O操作是做什么的，例如Accept/Recv等
	char*          szBuffer;                 // 对应WSABUF里的缓冲区  
	DWORD          dwSendLen;                //接收数据长度
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


