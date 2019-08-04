#pragma once
#ifndef _WINDOWS
	
#else
#include <process.h>
#include <string>
#include <WS2tcpip.h>
using std::string;
#endif

#include "CTcpSessionMgr.h"
#include "CNetEventModleMgr.h"

#ifndef _WINDOWS
#else
typedef struct stPer_Io_Context_Accept
{
	OVERLAPPED     Overlapped;               // 每一个重叠I/O网络操作都要有一个                
	SOCKET         sockAccept;               // 这个I/O操作所使用的Socket，每个连接的都是一样的  
	WSABUF         wsaBuf;                   // 存储数据的缓冲区，用来给重叠操作传递参数的，关于WSABUF后面还会讲  
	char           szBuffer[MAX_BUFFER_LEN]; // 对应WSABUF里的缓冲区  
	DWORD          OpType;                   // 标志这个重叠I/O操作是做什么的，例如Accept/Recv等
	DWORD          dwRecvLen;                //接收数据长度
}PER_IO_CONTEXT_ACCEPT_T, *LP_PER_IO_CONTEXT_ACCEPT_T;
#endif

class CAcceptSession
{
public:
	CAcceptSession();
	~CAcceptSession();

	int AcceptSession_Init(int nListenPort, int nEventModleCount, int nNetEventModleThreadCount, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, bool bIsHaveHB);
	int AcceptSession_Fini();

	int AcceptSession_Accept();

	int AcceptSession_SendData(int nSessionId, const unsigned int nSendDataLen, const char *szSendData, const void *pUser);

	bool AcceptSession_HandleAcceptError(const DWORD& dwErr);
#ifndef _WINDOWS
	bool AcceptSession_IsSocketAlive(int nSocket);
#else
	bool AcceptSession_IsSocketAlive(SOCKET Socket);
	static LPFN_ACCEPTEX                 m_AcceptEx;
	static LPFN_CONNECTEX                m_ConnectEx;
	static LPFN_DISCONNECTEX             m_DisconnectEx;
	static LPFN_GETACCEPTEXSOCKADDRS     m_GetAcceptExSocketAddrs;
#endif
	string GetLocalIP();	
private:
#ifndef _WINDOWS
	int                        m_nSocket;
	int                        m_AcceptEpoll;
	pthread_t                  m_tAcceptPthread;
	struct epoll_event         m_pEpollEvent;
#else
	SOCKET                     m_Socket;
	HANDLE                     m_hAcceptIOCP;
	HANDLE                     m_hAcceptThread;
	LP_PER_IO_CONTEXT_ACCEPT_T m_pPerIoContextAccept;
#endif
	bool                       m_bAcceptThreadFlag;
	int                        m_nListenPort;
	int                        m_nEventModleCount;
	int                        m_nAcceptIndex;
	CTcpSessionMgr            *m_pTcpSessionMgr;
	CNetEventModleMgr         *m_pNetEventModleMgr;
	ITransPortEngineRecv      *m_pITransPortEngineRecv;
	bool                       m_bIsHaveHead;
};

