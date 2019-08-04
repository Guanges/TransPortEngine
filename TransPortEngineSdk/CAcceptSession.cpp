#include "CAcceptSession.h"



#ifndef _WINDOWS
void* AcceptThread(void *vParam)
#else
LPFN_ACCEPTEX                 CAcceptSession::m_AcceptEx = NULL;
LPFN_CONNECTEX                CAcceptSession::m_ConnectEx = NULL;
LPFN_DISCONNECTEX             CAcceptSession::m_DisconnectEx = NULL;
LPFN_GETACCEPTEXSOCKADDRS     CAcceptSession::m_GetAcceptExSocketAddrs = NULL;
void AcceptThread(void *vParam)
#endif
{
	CAcceptSession* pAcceptSession = reinterpret_cast<CAcceptSession*>(vParam);
	pAcceptSession->AcceptSession_Accept();
#ifndef _WINDOWS
	return NULL;
#else
	_endthread();
#endif
}

CAcceptSession::CAcceptSession()
{
#ifndef _WINDOWS
	m_nSocket = 0;
	m_AcceptEpoll = 0;
	m_tAcceptPthread = 0;
	m_pEpollEvent = {0};
#else
	m_Socket = INVALID_SOCKET;
	m_hAcceptIOCP = NULL;
	m_hAcceptThread = NULL;
	m_pPerIoContextAccept = NULL;
#endif
	m_nListenPort = 0;
	m_nEventModleCount = 0;
	m_nAcceptIndex = 0;
	m_bAcceptThreadFlag = true;
}


CAcceptSession::~CAcceptSession()
{
}
#ifndef _WINDOWS
#else
void* NetFunctionEx(SOCKET Socket, GUID Guid)
{
	void* pFunction = NULL;
	DWORD dwBytes = 0;
	if (SOCKET_ERROR == WSAIoctl(
		Socket,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&Guid,
		sizeof(Guid),
		&pFunction,
		sizeof(pFunction),
		&dwBytes,
		NULL,
		NULL))
	{
		return NULL;
	}
	return pFunction;
}

////////////////////////////////////////////////////////////////////
// 获得本机的IP地址
string CAcceptSession::GetLocalIP()
{
	// 获得本机主机名
	char hostname[MAX_PATH] = { 0 };
	gethostname(hostname, MAX_PATH);
	struct hostent FAR* lpHostEnt = gethostbyname(hostname);
	if (lpHostEnt == NULL)
	{
		return ADDR_ANY;
	}

	// 取得IP地址列表中的第一个为返回的IP(因为一台主机可能会绑定多个IP)
	LPSTR lpAddr = lpHostEnt->h_addr_list[0];

	// 将IP地址转化成字符串形式
	struct in_addr inAddr;
	memmove(&inAddr, lpAddr, 4);
	string strIP = inet_ntoa(inAddr);

	return strIP;
}
#endif






int CAcceptSession::AcceptSession_Init(int nListenPort, int nEventModleCount, int nNetEventModleThreadCount, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, bool bIsHaveHB)
{
	if (0 == nListenPort || 0 == nEventModleCount)
	{
		return -1;
	}
	m_nListenPort = nListenPort;
	m_nEventModleCount = nEventModleCount;
	m_pITransPortEngineRecv = pITransPortEngineRecv;
	m_bIsHaveHead = bIsHaveHead;
#ifndef _WINDOWS
	m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
	int flag;
	flag = fcntl(m_nSocket, F_GETFL);
	fcntl(m_nSocket, F_SETFL, flag | O_NONBLOCK);
	int reuse = 1;
	if (-1 == setsockopt(m_nSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)))
	{
		return -1;
	}
	struct sockaddr_in SockAddr;

	SockAddr.sin_family = AF_INET;
    	SockAddr.sin_port = htons(nListenPort);
    	SockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (-1 == bind(m_nSocket, (struct sockaddr *)&SockAddr, sizeof(sockaddr)))
	{
		return -3;
	}

	if (-1 == listen(m_nSocket, 4096))
	{
		return -4;
	}
	int nRet = 0;
#else
	m_Socket = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	int reuse = 1;
	if (SOCKET_ERROR == setsockopt(m_Socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)))
	{
		return -1;
	}
	SOCKADDR_IN SockAddr;

	SockAddr.sin_family = AF_INET;

	INT nRet = inet_pton(AF_INET, "0.0.0.0", (void *)&SockAddr.sin_addr);
	if (nRet < 0)
	{
		return -2;
	}

	SockAddr.sin_port = htons(nListenPort);

	if (SOCKET_ERROR == bind(m_Socket, (struct sockaddr *)&SockAddr, sizeof(sockaddr)))
	{
		return -3;
	}

	if (SOCKET_ERROR == listen(m_Socket, 4096))
	{
		return -4;
	}
#endif
	m_pTcpSessionMgr = new CTcpSessionMgr;
	if (NULL == m_pTcpSessionMgr)
	{
		return -5;
	}
	m_pNetEventModleMgr = new CNetEventModleMgr;
	if (NULL == m_pNetEventModleMgr)
	{
		return -7;
	}
	nRet = m_pTcpSessionMgr->TcpSessionMgr_Init(bIsHaveHB, m_pNetEventModleMgr);
	if (nRet < 0)
	{
		return -6;
	}
	nRet = m_pNetEventModleMgr->NetEventModleMgr_Init(nEventModleCount, nNetEventModleThreadCount, reinterpret_cast<CTcpSessionMgr *>(m_pTcpSessionMgr));
	if (nRet < 0)
	{
		return -8;
	}

#ifndef _WINDOWS
	m_AcceptEpoll = epoll_create(1);
	if (m_AcceptEpoll < 0)
	{
		return -9;
	}
	m_pEpollEvent.events = EPOLLIN;
	m_pEpollEvent.data.fd = m_nSocket;

	epoll_ctl(m_AcceptEpoll, EPOLL_CTL_ADD, m_nSocket, &m_pEpollEvent);

	nRet = pthread_create(&m_tAcceptPthread, NULL, AcceptThread, this);
	if (nRet != 0)
	{
		return -10;
	}	
#else
	m_AcceptEx = (LPFN_ACCEPTEX)NetFunctionEx(m_Socket, WSAID_ACCEPTEX);
	m_ConnectEx = (LPFN_CONNECTEX)NetFunctionEx(m_Socket, WSAID_CONNECTEX);
	m_DisconnectEx = (LPFN_DISCONNECTEX)NetFunctionEx(m_Socket, WSAID_DISCONNECTEX);
	m_GetAcceptExSocketAddrs = (LPFN_GETACCEPTEXSOCKADDRS)NetFunctionEx(m_Socket, WSAID_GETACCEPTEXSOCKADDRS);
	if (NULL == m_AcceptEx || NULL == m_ConnectEx || NULL == m_DisconnectEx || NULL == m_GetAcceptExSocketAddrs)
	{
		return -9;
	}
	m_hAcceptIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if (NULL == m_hAcceptIOCP)
	{
		return -10;
	}
	if (NULL == CreateIoCompletionPort((HANDLE)m_Socket, m_hAcceptIOCP, (ULONG_PTR)this, 0))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			return -11;
		}
	}
	m_pPerIoContextAccept = new PER_IO_CONTEXT_ACCEPT_T;
	memset(m_pPerIoContextAccept, 0, sizeof(PER_IO_CONTEXT_ACCEPT_T));
	m_pPerIoContextAccept->OpType = IO_ACCEPT;
	m_pPerIoContextAccept->Overlapped = { 0 };
	m_pPerIoContextAccept->sockAccept = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_pPerIoContextAccept->wsaBuf.len = MAX_BUFFER_LEN;
	m_pPerIoContextAccept->wsaBuf.buf = m_pPerIoContextAccept->szBuffer;
	DWORD dwRecvLen = 0;
	if (!m_AcceptEx(m_Socket, m_pPerIoContextAccept->sockAccept, &m_pPerIoContextAccept->wsaBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwRecvLen, &m_pPerIoContextAccept->Overlapped))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			return -12;
		}
	}
	m_hAcceptThread = (HANDLE)_beginthread(AcceptThread, 0, this);
	if (NULL == m_hAcceptThread)
	{
		return -13;
	}
#endif
	return 0;
}

int CAcceptSession::AcceptSession_Fini()
{
#ifndef _WINDOWS
	if (m_bAcceptThreadFlag)
	{
		m_bAcceptThreadFlag = false;
		
		pthread_join(m_tAcceptPthread, NULL);
	}
#else
	if (m_bAcceptThreadFlag)
	{
		m_bAcceptThreadFlag = false;
		PostQueuedCompletionStatus(m_hAcceptIOCP, 0, 0, 0);
		WaitForSingleObject(m_hAcceptThread, INFINITE);
	}
	if (m_pPerIoContextAccept)
	{
		delete m_pPerIoContextAccept;
	}

	CloseHandle(m_hAcceptIOCP);
#endif
	if (m_pNetEventModleMgr)
	{
		m_pNetEventModleMgr->NetEventModleMgr_Fini();
		delete m_pNetEventModleMgr;
	}
	if (m_pTcpSessionMgr)
	{
		m_pTcpSessionMgr->TcpSessionMgr_Fini();
		delete m_pTcpSessionMgr;
	}
	return 0;
}
#ifndef _WINDOWS
int CAcceptSession::AcceptSession_Accept()
{
	m_nAcceptIndex = 0;
	while (m_bAcceptThreadFlag)
	{
		int nNumEvent = epoll_wait(m_AcceptEpoll, &m_pEpollEvent, 1, 1000);
		if (nNumEvent > 0) 
		{
			int nAcceptSock = accept(m_nSocket, NULL, NULL);
			if (nAcceptSock > 0) 
			{
				m_nAcceptIndex++;
				int nNetEventModleIndex = m_nAcceptIndex % m_nEventModleCount;

				CTcpSession* pTcpSession = new CTcpSession;
				if (NULL == pTcpSession)
				{
					printf("Create CTcpSession Error:%p\n", pTcpSession);
					close(nAcceptSock);
					continue;
				}

				LP_NET_FIVE_ELEMENT_T pNetFiveElement = new NET_FIVE_ELEMENT_T;
				if (NULL == pNetFiveElement)
				{
					printf("Create NET_FIVE_ELEMENT_T Error:%p\n", pNetFiveElement);
					close(nAcceptSock);
					delete pTcpSession;
					continue;
				}

				CNetEventModleEpoll* pNetEventModleEpoll = m_pNetEventModleMgr->NetEventModleMgr_FindModle(nNetEventModleIndex);
				if (NULL == pNetEventModleEpoll)
				{
					printf("NetEventModleMgr_FindModle nNetEventModleIndex:%d\n", nNetEventModleIndex);
					delete pTcpSession;
					delete pNetFiveElement;
					continue;
				}

				int nRet = pTcpSession->TcpSession_Init(nAcceptSock, pNetEventModleEpoll, pNetFiveElement, m_pITransPortEngineRecv, m_bIsHaveHead, this);
				if (nRet < 0)
				{
					printf("TcpSession_Init Error:%d\n", nRet);
					delete pTcpSession;
					delete pNetFiveElement;
					continue;
				}

				nRet = m_pTcpSessionMgr->TcpSessionMgr_AddSession(pTcpSession);
				if (nRet < 0)
				{
					nRet = pTcpSession->TcpSession_Fini();
					if (nRet < 0)
					{
						printf("TcpSession_Fini Error:%d\n", nRet);
					}
					delete pTcpSession;
					continue;
				}

				nRet = pNetEventModleEpoll->NetEventModleEpoll_AddEvent(pTcpSession);
				if (nRet < 0)
				{
					nRet = m_pTcpSessionMgr->TcpSessionMgr_DelSession(nAcceptSock);
					if (nRet < 0)
					{
						printf("TcpSessionMgr_DelSession Error:%d\n", nRet);
					}
					nRet = pTcpSession->TcpSession_Fini();
					if (nRet < 0)
					{
						printf("TcpSession_Fini Error:%d\n", nRet);
					}
					delete pTcpSession;
					continue;
				}

				nRet = pTcpSession->TcpSession_IORecv(1);
				if (nRet < 0)
				{
					nRet = pNetEventModleEpoll->NetEventModleEpoll_DelEvent(pTcpSession);
					if (nRet < 0)
					{
						printf("NetEventModleIOCP_DelEvent Error:%d\n", nRet);
					}
					nRet = m_pTcpSessionMgr->TcpSessionMgr_DelSession(nAcceptSock);
					if (nRet < 0)
					{
						printf("TcpSessionMgr_DelSession Error:%d\n", nRet);
					}
					nRet = pTcpSession->TcpSession_Fini();
					if (nRet < 0)
					{
						printf("TcpSession_Fini Error:%d\n", nRet);
					}
					delete pTcpSession;
					continue;
				}

			}
		}
	}
	return 0;
}
#else

bool CAcceptSession::AcceptSession_IsSocketAlive(SOCKET Socket)
{
	int nByteSent = send(Socket, "", 0, 0);
	if (-1 == nByteSent)
	{
		return false;
	}
	return true;
}

bool CAcceptSession::AcceptSession_HandleAcceptError(const DWORD& dwErrno)
{
	if (WAIT_TIMEOUT == dwErrno)
	{
		if (!AcceptSession_IsSocketAlive(m_Socket))
		{
			return true;
		}
		else
		{
			return true;
		}
	}
	else if (ERROR_NETNAME_DELETED == dwErrno)
	{
		return true;
	}
	else
	{
		return false;
	}
}

int CAcceptSession::AcceptSession_Accept()
{
	while (m_bAcceptThreadFlag)
	{
		OVERLAPPED*          pOverlapped = NULL;
		CAcceptSession*      pAcceptSession = NULL;
		DWORD                dwBytesTransfered = 0;
		BOOL bRet = GetQueuedCompletionStatus(
			m_hAcceptIOCP,
			&dwBytesTransfered,
			(PULONG_PTR)&pAcceptSession,
			&pOverlapped,
			INFINITE);
		if (!bRet)
		{
			DWORD dwErr = GetLastError();
			if (!AcceptSession_HandleAcceptError(dwErr))
			{
				break;
			}
			continue;
		}
		else
		{
			m_nAcceptIndex++;
			int nNetEventModleIndex = m_nAcceptIndex % m_nEventModleCount;
			LP_PER_IO_CONTEXT_ACCEPT_T pIoContext = CONTAINING_RECORD(pOverlapped, PER_IO_CONTEXT_ACCEPT_T, Overlapped);
			if ((0 == dwBytesTransfered) && (IO_RECV == pIoContext->OpType || IO_SEND == pIoContext->OpType))
			{
				// 释放掉对应的资源
				//@TODO

				continue;
			}
			else
			{
				switch (pIoContext->OpType)
				{
				case IO_ACCEPT:
				{
					CTcpSession* pTcpSession = new CTcpSession;
					if (NULL == pTcpSession)
					{
						printf("Create CTcpSession Error:%p\n", pTcpSession);
						closesocket(pIoContext->sockAccept);
						continue;
					}

					LP_NET_FIVE_ELEMENT_T pNetFiveElement = new NET_FIVE_ELEMENT_T;
					if (NULL == pNetFiveElement)
					{
						printf("Create NET_FIVE_ELEMENT_T Error:%p\n", pNetFiveElement);
						closesocket(pIoContext->sockAccept);
						delete pTcpSession;
						continue;
					}
					SOCKADDR_IN* LocalAddr = NULL;
					int nLocalLen = sizeof(SOCKADDR_IN);
					SOCKADDR_IN* RemoteAddr = NULL;
					int nRemoteLen = sizeof(SOCKADDR_IN);
					m_GetAcceptExSocketAddrs(pIoContext->szBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&LocalAddr, &nLocalLen, (LPSOCKADDR*)&RemoteAddr, &nRemoteLen);

					pNetFiveElement->strLocalIp = inet_ntoa(LocalAddr->sin_addr);
					pNetFiveElement->nLocalPort = ntohs(LocalAddr->sin_port);

					pNetFiveElement->nRemotePort = ntohs(RemoteAddr->sin_port);
					pNetFiveElement->strRemoteIp = inet_ntoa(RemoteAddr->sin_addr);

					int nRet = pTcpSession->TcpSession_Init(pIoContext->sockAccept, nNetEventModleIndex, pNetFiveElement, m_pITransPortEngineRecv, m_bIsHaveHead, this);
					if (nRet < 0)
					{
						printf("TcpSession_Init Error:%d\n", nRet);
						delete pTcpSession;
						delete pNetFiveElement;
						continue;
					}

					nRet = m_pTcpSessionMgr->TcpSessionMgr_AddSession(pTcpSession);
					if (nRet < 0)
					{
						nRet = pTcpSession->TcpSession_Fini();
						if (nRet < 0)
						{
							printf("TcpSession_Fini Error:%d\n", nRet);
						}
						delete pTcpSession;
						continue;
					}

					 CNetEventModleIOCP* pNetEventModleIOCP = m_pNetEventModleMgr->NetEventModleMgr_FindModle(nNetEventModleIndex);
					 if (NULL == pNetEventModleIOCP)
					 {
						 nRet = m_pTcpSessionMgr->TcpSessionMgr_DelSession(pIoContext->sockAccept);
						 if (nRet < 0)
						 {
							 printf("TcpSessionMgr_DelSession Error:%d\n", nRet);
						 }
						 nRet = pTcpSession->TcpSession_Fini();
						 if (nRet < 0)
						 {
							 printf("TcpSession_Fini Error:%d\n", nRet);
						 }
						 delete pTcpSession;
						 continue;
					 }

					 nRet = pNetEventModleIOCP->NetEventModleIOCP_AddEvent(pTcpSession);
					 if (nRet < 0)
					 {
						 nRet = m_pTcpSessionMgr->TcpSessionMgr_DelSession(pIoContext->sockAccept);
						 if (nRet < 0)
						 {
							 printf("TcpSessionMgr_DelSession Error:%d\n", nRet);
						 }
						 nRet = pTcpSession->TcpSession_Fini();
						 if (nRet < 0)
						 {
							 printf("TcpSession_Fini Error:%d\n", nRet);
						 }
						 delete pTcpSession;
						 continue;
					 }

					 nRet = pTcpSession->TcpSession_IORecv(1);
					 if (nRet < 0)
					 {
						 nRet = pNetEventModleIOCP->NetEventModleIOCP_DelEvent(pTcpSession);
						 if (nRet < 0)
						 {
							 printf("NetEventModleIOCP_DelEvent Error:%d\n", nRet);
						 }
						 nRet = m_pTcpSessionMgr->TcpSessionMgr_DelSession(pIoContext->sockAccept);
						 if (nRet < 0)
						 {
							 printf("TcpSessionMgr_DelSession Error:%d\n", nRet);
						 }
						 nRet = pTcpSession->TcpSession_Fini();
						 if (nRet < 0)
						 {
							 printf("TcpSession_Fini Error:%d\n", nRet);
						 }
						 delete pTcpSession;
						 continue;
					 }
					 m_pPerIoContextAccept->sockAccept = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
					 m_pPerIoContextAccept->wsaBuf.buf = m_pPerIoContextAccept->szBuffer;
					 m_pPerIoContextAccept->wsaBuf.len = MAX_BUFFER_LEN;
					 memset(m_pPerIoContextAccept->szBuffer, 0, MAX_BUFFER_LEN);
					 DWORD dwRecvLen = 0;
					 if (!m_AcceptEx(m_Socket, m_pPerIoContextAccept->sockAccept, &m_pPerIoContextAccept->wsaBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &dwRecvLen, &m_pPerIoContextAccept->Overlapped))
					 {
						 if (WSA_IO_PENDING != WSAGetLastError())
						 {
							 break;
						 }
					 }
				}
				break;
				case IO_RECV:
				{
					
				}
				break;
				case IO_SEND:
				{

				}
				break;
				case IO_CONNECT:
				{

				}
				break;
				case IO_DISCONNECT:
				{

				}
				break;
				default:
					break;
				}
			}
		}
	}
	return 0;
}

#endif

int CAcceptSession::AcceptSession_SendData(int nSessionId, const unsigned int nSendDataLen, const char *szSendData, const void *pUser)
{
	CTcpSession	*pTcpSession = m_pTcpSessionMgr->TcpSessionMgr_FindSession(nSessionId);
	if (NULL == pTcpSession)
	{
		return -1;
	}
	int nRet = pTcpSession->TcpSession_IOSend(szSendData, nSendDataLen);
	if (nRet < 0)
	{
		return -2;
	}
	return 0;
}
