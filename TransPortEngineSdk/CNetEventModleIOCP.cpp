#include "CNetEventModleIOCP.h"
#include "CTcpSessionMgr.h"

#ifndef _WINDOWS
void* HandleThread(void *vParam)
#else
void HandleThread(void *vParam)
#endif
{
	LP_THREADINFO_T pThreadInfo = reinterpret_cast<LP_THREADINFO_T>(vParam);
	CNetEventModleIOCP* pNetEventModleIOCP = reinterpret_cast<CNetEventModleIOCP*>(pThreadInfo->pNetEventModleIOCP);
	pNetEventModleIOCP->NetEventModleIOCP_HandleData(pThreadInfo);
#ifndef _WINDOWS
	pthread_destroy();
#else
	_endthread();
#endif
}

CNetEventModleIOCP::CNetEventModleIOCP()
{
}


CNetEventModleIOCP::~CNetEventModleIOCP()
{
}

int CNetEventModleIOCP::NetEventModleIOCP_Init(int nNetEventModleIndex, int nNetEventModleThreadCount, void *pTcpSessionMgr)
{
	if (0 > nNetEventModleIndex || 0 == nNetEventModleThreadCount || NULL == pTcpSessionMgr)
	{
		return -1;
	}
	m_nNetEventModleIndex = nNetEventModleIndex;
	m_nNetEventModleThreadCount = nNetEventModleThreadCount;
	m_pTcpSessionMgr = pTcpSessionMgr;

	m_hHandleDataIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	if (NULL == m_hHandleDataIOCP)
	{
		return -2;
	}
	
	for (int i = 0; i < m_nNetEventModleThreadCount; i++)
	{
		LP_THREADINFO_T pThreadInfo = new THREADINFO_T;
		memset(pThreadInfo, 0, sizeof(THREADINFO_T));
		pThreadInfo->bWorkThreadFlag = true;
		pThreadInfo->pNetEventModleIOCP = this;
		pThreadInfo->hWorkThread = (HANDLE)_beginthread(HandleThread, 0, pThreadInfo);
		if (NULL == pThreadInfo->hWorkThread)
		{
			delete pThreadInfo;
			return -3;
		}
		CSSAutoLock AutoLock(&m_WorkThreadLock);
		m_WorkThreadInfos.push_back(pThreadInfo);
	}

	return 0;
}

int CNetEventModleIOCP::NetEventModleIOCP_Fini()
{
	for (int i = 0; i < m_nNetEventModleThreadCount; i++)
	{
		m_WorkThreadInfos[i]->bWorkThreadFlag = false;
		PostQueuedCompletionStatus(m_hHandleDataIOCP, 0, 0, 0);
	}
	for (int i = 0; i < m_nNetEventModleThreadCount; i++)
	{
		WaitForSingleObject(m_WorkThreadInfos[i]->hWorkThread, INFINITE);
	}

	for (int i = 0; i < m_nNetEventModleThreadCount; i++)
	{
		if (m_WorkThreadInfos[i])
		{
			delete m_WorkThreadInfos[i];
		}
	}
	CloseHandle(m_hHandleDataIOCP);
	return 0;
}

int CNetEventModleIOCP::NetEventModleIOCP_AddEvent(CTcpSession* pTcpSession)
{
	if (NULL == CreateIoCompletionPort((HANDLE)pTcpSession->TcpSession_GetSocket(), (HANDLE)m_hHandleDataIOCP, (ULONG_PTR)pTcpSession->TcpSession_GetSocket(), 0))
	{
		if (WSA_IO_PENDING != WSAGetLastError())
		{
			return -1;
		}
	}
	return 0;
}

int CNetEventModleIOCP::NetEventModleIOCP_DelEvent(CTcpSession* pTcpSession)
{
	if (!CancelIoEx((HANDLE)pTcpSession->TcpSession_GetSocket(), &pTcpSession->TcpSession_GetPerIoContextRecv()->Overlapped))
	{
		if (WSA_IO_PENDING == WSAGetLastError())
		{
			return -1;
		}
	}

	int nRet = pTcpSession->TcpSession_CancelIoSend();
	if (nRet < 0)
	{
		return -2;
	}
	return 0;
}

int CNetEventModleIOCP::NetEventModleIOCP_HandleData(LP_THREADINFO_T pThreadInfo)
{
	bool bFlag = false;
	while (pThreadInfo->bWorkThreadFlag)
	{
		OVERLAPPED*      pOverlapped = NULL;
		SOCKET           Socket = 0;
		DWORD            dwBytesTransfered = 0;
		BOOL bRet = GetQueuedCompletionStatus(
			m_hHandleDataIOCP,
			&dwBytesTransfered,
			(PULONG_PTR)&Socket,
			&pOverlapped,
			INFINITE);
		if (!bRet)
		{
			DWORD dwErrno = GetLastError();

			CTcpSession *pTcpSession = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_FindSession(Socket);
			if (nullptr == pTcpSession)
			{
				continue;
			}
			if (!AcceptSession_HandleAcceptError((CTcpSession*)pTcpSession, Socket, dwErrno))
			{
				continue;
			}
		}
		else
		{
			// 读取传入的参数
			bFlag = false;
			CTcpSession* pTcpSession = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_FindSession(Socket);
			if (nullptr == pTcpSession)
			{
				continue;
			}
			LP_PER_IO_CONTEXT_RECV_T pIoContext = CONTAINING_RECORD(pOverlapped, PER_IO_CONTEXT_RECV_T, Overlapped);

			// 判断是否有客户端断开了
			if ((0 == dwBytesTransfered) && (IO_RECV == pIoContext->OpType || IO_SEND == pIoContext->OpType))
			{
				// 释放掉对应的资源
				//@TODO
				int nRet = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_DelSession(Socket);
				if (nRet < 0)
				{
					LOG_ERROR("TcpSessionMgr_DelSession Error Ret:{}", nRet);
				}
				nRet = NetEventModleIOCP_DelEvent(pTcpSession);
				if (nRet < 0)
				{
					LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
				}
				nRet = pTcpSession->TcpSession_Fini();
				if (nRet < 0)
				{
					LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
				}
				bFlag = true;
			}
			else
			{
				switch (pIoContext->OpType)
				{
				case IO_ACCEPT:
				{

				}
				break;
				case IO_RECV:
				{
					pIoContext->dwRecvLen = dwBytesTransfered;
					int nRet = pTcpSession->TcpSession_IORecv();
					if (nRet < 0)
					{
						nRet = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_DelSession(Socket);
						if (nRet < 0)
						{
							LOG_ERROR("TcpSessionMgr_DelSession Error Ret:{}", nRet);
						}
						else
						{
							CSSAutoLock AutoLock(pTcpSession->TcpSession_GetIOCPEventLock());

							nRet = NetEventModleIOCP_DelEvent(pTcpSession);
							if (nRet < 0)
							{
								LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
							}
							nRet = pTcpSession->TcpSession_Fini();
							if (nRet < 0)
							{
								LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
							}
							bFlag = true;
						}
					}
				}
				break;
				case IO_SEND:
				{
					LP_PER_IO_CONTEXT_SEND_T pIoContext = CONTAINING_RECORD(pOverlapped, PER_IO_CONTEXT_SEND_T, Overlapped);
					pIoContext->dwSendLen = dwBytesTransfered;
					int nRet = pTcpSession->TcpSession_IOSend(pIoContext->nSquence);
					if (nRet < 0)
					{
						nRet = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_DelSession(Socket);
						if (nRet < 0)
						{
							LOG_ERROR("TcpSessionMgr_DelSession Error Ret:{}", nRet);
						}
						else
						{
							CSSAutoLock AutoLock(pTcpSession->TcpSession_GetIOCPEventLock());

							nRet = NetEventModleIOCP_DelEvent(pTcpSession);
							if (nRet < 0)
							{
								LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
							}
							nRet = pTcpSession->TcpSession_Fini();
							if (nRet < 0)
							{
								LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
							}
							bFlag = true;
						}
					}
				}
				break;
				case IO_CONNECT:
				{
					LOG_ERROR("connect msg!");
				}
				break;
				case IO_DISCONNECT:
				{
					LOG_ERROR("disconnect msg!");
				}
				break;
				default:
					break;
				}
			}
			if (bFlag)
			{
				delete pTcpSession;
			}
		}
	}
	return 0;
}

bool CNetEventModleIOCP::AcceptSession_IsSocketAlive(SOCKET Socket)
{
	int nByteSent = send(Socket, "", 0, 0);
	if (-1 == nByteSent)
	{
		return false;
	}
	return true;
}

bool CNetEventModleIOCP::AcceptSession_HandleAcceptError(CTcpSession* pTcpSession, SOCKET Socket, const DWORD& dwErrno)
{
	// 如果是超时了，就再继续等吧 
	bool bFlag = false;	
	if (WAIT_TIMEOUT == dwErrno)
	{
		// 确认客户端是否还活着...
		if (!AcceptSession_IsSocketAlive(Socket))
		{
			int nRet = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_DelSession(Socket);
			if (nRet < 0)
			{
				LOG_ERROR("TcpSessionMgr_DelSession Error Ret:{}", nRet);
			}
			else
			{
				CSSAutoLock AutoLock(pTcpSession->TcpSession_GetIOCPEventLock());
				nRet = NetEventModleIOCP_DelEvent(pTcpSession);
				if (nRet < 0)
				{
					LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
				}
				nRet = pTcpSession->TcpSession_Fini();
				if (nRet < 0)
				{
					LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
				}
				bFlag = true;
			}
		}
		else
		{
			return true;
		}
	}
	else if (ERROR_NETNAME_DELETED == dwErrno)
	{
		int nRet = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_DelSession(Socket);
		if (nRet < 0)
		{
			LOG_ERROR("TcpSessionMgr_DelSession Error Ret:{}", nRet);
		}
		else
		{
			CSSAutoLock AutoLock(pTcpSession->TcpSession_GetIOCPEventLock());
			nRet = NetEventModleIOCP_DelEvent(pTcpSession);
			if (nRet < 0)
			{
				LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
			}
			nRet = pTcpSession->TcpSession_Fini();
			if (nRet < 0)
			{
				LOG_ERROR("NetEventModleIOCP_DelEvent Error Ret:{}", nRet);
			}
			bFlag = true;
		}
	}
	else
	{

		return false;
	}
	if (bFlag)
	{
		delete pTcpSession;
	}
	return true;
}
