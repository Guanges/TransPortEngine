#include "CTcpSession.h"
CTcpSession::CTcpSession()
{
	m_pNetFiveElement = NULL;
#ifndef _WINDOWS
	m_nSocket = -1;
	m_pINetEventModleEpoll = NULL;
	m_SendBufs.clear();
#else
	m_Socket = 0;
	CSSAutoLock AutoLock(&m_PerIoContextSendLock);
	m_PerIoContextSends.clear();
	m_pPerIoContextRecv = NULL;
#endif
	m_RecvBufs.clear();
	m_nEventModleIndex = 0;
	m_nSquence = 0;
	m_bIsRecvHead = false;
}


CTcpSession::~CTcpSession()
{
#ifndef _WINDOWS
	if (-1 != m_nSocket)
	{
		close(m_nSocket);
		m_nSocket = -1;
	}
#else
	if (m_pPerIoContextRecv)
	{
		delete m_pPerIoContextRecv;
		m_pPerIoContextRecv = NULL;
	}
	CSSAutoLock AutoLock(&m_PerIoContextSendLock);
	for (unsigned int i = 0; i < m_PerIoContextSends.size(); i++)
	{
		if (m_PerIoContextSends[i]->szBuffer)
		{
			delete[] m_PerIoContextSends[i]->szBuffer;
		}
		delete m_PerIoContextSends[i];
	}
	m_PerIoContextSends.clear();

	if (0 != m_Socket)
	{
		closesocket(m_Socket);
		m_Socket = 0;
	}

#endif
	if (m_pNetFiveElement)
	{
		delete m_pNetFiveElement;
		m_pNetFiveElement = NULL;
	}

	
	CSSAutoLock AutoLock1(&m_RecvBufLock);
	for (unsigned int i = 0; i < m_RecvBufs.size(); i++)
	{
		if (m_RecvBufs[i]->szBuf)
		{
			delete[] m_RecvBufs[i]->szBuf;
		}
		delete m_RecvBufs[i];
	}
	m_RecvBufs.clear();
}

#ifndef _WINDOWS
int CTcpSession::TcpSession_Init(int nSocket, INetEventModleEpoll* pINetEventModleEpoll, LP_NET_FIVE_ELEMENT_T pNetFiveElement, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, void *pUser)
#else
int CTcpSession::TcpSession_Init(SOCKET Socket, int nEventModleIndex, LP_NET_FIVE_ELEMENT_T pNetFiveElement, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, void *pUser)
#endif
{
#ifndef _WINDOWS
	if (-1 == nSocket || NULL == pINetEventModleEpoll || NULL == pNetFiveElement || NULL == pUser)
#else
	if (INVALID_SOCKET == Socket || 0 > nEventModleIndex || NULL == pNetFiveElement || NULL == pUser)
#endif
	{
		return -1;
	}
	m_pNetFiveElement = pNetFiveElement;
	m_pITransPortEngineRecv = pITransPortEngineRecv;
	m_bIsHaveHead = bIsHaveHead;
	m_pUser = pUser;
#ifndef _WINDOWS
	m_pINetEventModleEpoll = pINetEventModleEpoll;
	m_nSocket = nSocket;
	int nFlag = fcntl(m_nSocket, F_GETFL);
	fcntl(m_nSocket, F_SETFL, nFlag | O_NONBLOCK);

	m_pRecvCache = new RECV_CACHE_T;
	memset(m_pRecvCache, 0, sizeof(RECV_CACHE_T));
	if (m_pITransPortEngineRecv)
	{
		m_pITransPortEngineRecv->OnTransPortEngineNotify(static_cast<int>(m_nSocket), CONNECT, NULL, this);
	}
#else
	m_nEventModleIndex = nEventModleIndex;
	m_Socket = Socket;
	m_pPerIoContextRecv = new PER_IO_CONTEXT_RECV_T;
	if (NULL == m_pPerIoContextRecv)
	{
		return -2;
	}
	memset(m_pPerIoContextRecv, 0, sizeof(PER_IO_CONTEXT_RECV_T));
	m_pPerIoContextRecv->dwRecvLen = 0;
	m_pPerIoContextRecv->OpType = IO_RECV;
	m_pPerIoContextRecv->Overlapped = { 0 };
	m_pPerIoContextRecv->sockRecv = m_Socket;
	memset(m_pPerIoContextRecv->szBuffer, 0, MAX_BUFFER_LEN);
	m_pPerIoContextRecv->wsaBuf.len = MAX_BUFFER_LEN;
	m_pPerIoContextRecv->wsaBuf.buf = m_pPerIoContextRecv->szBuffer;
	if (m_pITransPortEngineRecv)
	{
		m_pITransPortEngineRecv->OnTransPortEngineNotify(static_cast<int>(m_Socket), CONNECT, NULL, this);
	}
#endif
	return 0;
}

int CTcpSession::TcpSession_Fini()
{
	if (m_pNetFiveElement)
	{
		delete m_pNetFiveElement;
		m_pNetFiveElement = NULL;
	}
#ifndef _WINDOWS
	if (m_pITransPortEngineRecv)
	{
		m_pITransPortEngineRecv->OnTransPortEngineNotify(static_cast<int>(m_nSocket), DISCONNECT, NULL, this);
	}
	if (-1 != m_nSocket)
	{
		close(m_nSocket);
		m_nSocket = -1;
	}
#else
	if (m_pPerIoContextRecv)
	{
		delete m_pPerIoContextRecv;
		m_pPerIoContextRecv = NULL;
	}
	
	CSSAutoLock AutoLock(&m_PerIoContextSendLock);
	for (unsigned int i = 0; i < m_PerIoContextSends.size(); i++)
	{
		if (m_PerIoContextSends[i])
		{
			if (m_PerIoContextSends[i]->szBuffer)
			{
				delete m_PerIoContextSends[i]->szBuffer;
			}
			delete m_PerIoContextSends[i];
		}
	}
	m_PerIoContextSends.clear();

	if (m_pITransPortEngineRecv)
	{
		m_pITransPortEngineRecv->OnTransPortEngineNotify(static_cast<int>(m_Socket), DISCONNECT, NULL, this);
	}

	if (0 != m_Socket)
	{
		closesocket(m_Socket);
		m_Socket = 0;
	}

#endif
	CSSAutoLock AutoLock1(&m_RecvBufLock);
	for (unsigned int i = 0; i < m_RecvBufs.size(); i++)
	{
		if (m_RecvBufs[i])
		{
			if (m_RecvBufs[i]->szBuf)
			{
				delete[] m_RecvBufs[i]->szBuf;
			}
			delete m_RecvBufs[i];
		}
	}
	m_RecvBufs.clear();
	return 0;
}

int CTcpSession::TcpSession_IOSend(int nSquence)
{
	
#ifndef _WINDOWS
	
	if (m_SendBufs.empty())
	{
		return 0;
	}
	LP_BUFINFO_T pBufInfo = NULL;
	{
		CSSAutoLock AutoLock(&m_SendBufLock);
		pBufInfo = m_SendBufs.front();
		m_SendBufs.erase(m_SendBufs.begin());
	}
	if (NULL  == pBufInfo)
	{
		return 0;
	}
	int nRet = write(m_nSocket, pBufInfo->szBuf, pBufInfo->nBufLen);
	if (nRet == -1) 
	{
        	if (errno != EINTR && errno != EAGAIN) 
		{
            		return -1;
		}
	}
	else if (nRet != pBufInfo->nBufLen)
	{
		return -2;
	}
	delete [] pBufInfo->szBuf;
	delete pBufInfo;
	nRet = m_pINetEventModleEpoll->NetEventModleEpoll_ModEvent(this);
	if (nRet < 0)
	{
		return -1;
	}
#else
	CSSAutoLock AutoLock(&m_PerIoContextSendLock);
	LP_PER_IO_CONTEXT_SEND_T pPerIoContextSend = NULL;
	unsigned int i = 0;
	for (i = 0; i < m_PerIoContextSends.size(); i++)
	{
		if (m_PerIoContextSends[i]->nSquence == nSquence)
		{
			pPerIoContextSend = m_PerIoContextSends[i];
			break;
		}
	}
	if (NULL == pPerIoContextSend)
	{
		return 0;
	}

	if (pPerIoContextSend->dwSendLen == pPerIoContextSend->wsaBuf.len)
	{
		delete [] pPerIoContextSend->szBuffer;
		delete pPerIoContextSend;
		m_PerIoContextSends.erase(m_PerIoContextSends.begin() + i);
		return 0;
	}
	else if (pPerIoContextSend->dwSendLen > pPerIoContextSend->wsaBuf.len)
	{
		delete[] pPerIoContextSend->szBuffer;
		delete pPerIoContextSend;
		m_PerIoContextSends.erase(m_PerIoContextSends.begin() + i);
		return -2;
	}
	else
	{
		pPerIoContextSend->wsaBuf.buf = pPerIoContextSend->wsaBuf.buf + pPerIoContextSend->dwSendLen;
		pPerIoContextSend->wsaBuf.len = pPerIoContextSend->wsaBuf.len - pPerIoContextSend->dwSendLen;
		pPerIoContextSend->dwSendLen = 0;
	}

	DWORD dwFlag = 0;
	if (false == m_bIsToBeClose)
	{
		int nRet = WSASend(m_Socket, &pPerIoContextSend->wsaBuf, 1, &pPerIoContextSend->dwSendLen, dwFlag, &pPerIoContextSend->Overlapped, NULL);
		if (nRet == INVALID_SOCKET && (WSA_IO_PENDING != WSAGetLastError()))
		{
			return -3;
		}
	}
#endif
	return 0;
}

int CTcpSession::TcpSession_IORecv()
{
#ifndef _WINDOWS
	memset(m_pRecvCache, 0, sizeof(RECV_CACHE_T));
	m_pRecvCache->nRecvLen = MAX_BUFFER_LEN;
	int nRet = read(m_nSocket, m_pRecvCache->szRecvBuf, m_pRecvCache->nRecvLen);
	if (nRet > 0)
	{
		if (m_bIsHaveHead)  //处理有头数据
		{
			if (m_bIsRecvHead)  //已经接收到头
			{
				if (static_cast<int>(m_nNeedRecvDataLen - TcpSession_GetRecvCacheLen()) == nRet)
				{
					char* szBuf = new char[m_nNeedRecvDataLen + 1];
					memset(szBuf, 0, m_nNeedRecvDataLen + 1);
					int nLen = TcpSession_GetRecvCacheBuf(szBuf);
					memcpy(szBuf + nLen, m_pRecvCache->szRecvBuf, nRet);
					if (0 == m_nRecvDataType)
					{
						TcpSession_HeadleHB(szBuf, m_nNeedRecvDataLen);
					}
					else if (1 == m_nRecvDataType)
					{
						if (m_pITransPortEngineRecv)
						{
							m_pITransPortEngineRecv->OnTransPortEngineRecv(static_cast<int>(m_nSocket), m_nNeedRecvDataLen, szBuf, this);
						}
					}
					m_bIsRecvHead = false;
					m_nNeedRecvDataLen = 0;
}
				else if (static_cast<int>(m_nNeedRecvDataLen - TcpSession_GetRecvCacheLen()) < nRet)
				{
					LP_BUFINFO_T pBufInfo = new BUFINFO_T;
					memset(pBufInfo, 0, sizeof(BUFINFO_T));
					pBufInfo->nBufLen = nRet;
					pBufInfo->szBuf = new char[nRet + 1];
					memset(pBufInfo->szBuf, 0, nRet + 1);
					memcpy(pBufInfo->szBuf, m_pRecvCache->szRecvBuf, nRet);
					CSSAutoLock AutoLock(&m_RecvBufLock);
					m_RecvBufs.push_back(pBufInfo);
				}
				else if (static_cast<int>(m_nNeedRecvDataLen - TcpSession_GetRecvCacheLen()) > nRet)
				{
					return -1;
				}
			}
			else //未接收到头
			{
				if (nRet > static_cast<int>(sizeof(HEAD_MSG_T) - TcpSession_GetRecvCacheLen()))
				{
					return -1;
				}
				else if (nRet < static_cast<int>(sizeof(HEAD_MSG_T) - TcpSession_GetRecvCacheLen()))
				{
					LP_BUFINFO_T pBufInfo = new BUFINFO_T;
					memset(pBufInfo, 0, sizeof(BUFINFO_T));
					pBufInfo->nBufLen = nRet;
					pBufInfo->szBuf = new char[nRet + 1];
					memset(pBufInfo->szBuf, 0, nRet + 1);
					memcpy(pBufInfo->szBuf, m_pRecvCache->szRecvBuf, nRet);
					CSSAutoLock AutoLock(&m_RecvBufLock);
					m_RecvBufs.push_back(pBufInfo);
				}
				else if (nRet == static_cast<int>(sizeof(HEAD_MSG_T) - TcpSession_GetRecvCacheLen()))
				{
					char* szBuf = new char[sizeof(HEAD_MSG_T)];
					memset(szBuf, 0, sizeof(HEAD_MSG_T));
					int nLen = TcpSession_GetRecvCacheBuf(szBuf);
					memcpy(szBuf + nLen, m_pRecvCache->szRecvBuf, nRet);
					LP_HEAD_MSG_T pHeadMsg = reinterpret_cast<LP_HEAD_MSG_T>(szBuf);
					if (!memcmp(pHeadMsg->szHeadType, HEART_BEAT, sizeof(HEART_BEAT)))
					{
						m_nRecvDataType = 0;
						m_nNeedRecvDataLen = ntohl(pHeadMsg->nDataLen);
						m_uSaltValue = ntohs(pHeadMsg->uSaltValue);
						if (m_nNeedRecvDataLen > 0)
						{
							m_bIsRecvHead = true;
						}
						else
						{
							TcpSession_HeadleHB(NULL, 0);
						}
						nRet = TcpSession_IOSend(szBuf, sizeof(HEAD_MSG_T));
						if (nRet < 0)
						{
							cout << "TcpSession_IOSend HB Error\n" << endl;
						}
					}
					else if (!memcmp(pHeadMsg->szHeadType, RECV_DATA, sizeof(RECV_DATA)))
					{
						m_nRecvDataType = 1;
						m_nNeedRecvDataLen = ntohl(pHeadMsg->nDataLen);
						m_uSaltValue = ntohs(pHeadMsg->uSaltValue);
						if (m_nNeedRecvDataLen > 0)
						{
							m_bIsRecvHead = true;
						}
					}
					delete[] szBuf;
				}
			}
		}
		else //处理无头数据
		{
			if (m_pITransPortEngineRecv)
			{
				m_pITransPortEngineRecv->OnTransPortEngineRecv(static_cast<int>(m_nSocket), nRet, m_pRecvCache->szRecvBuf, this);
			}
		}


	}
	else if (0 == nRet)
	{
		return -1;
	}
	else if (-1 == nRet)
	{
		if (errno != EINTR && errno != EAGAIN)
		{
			return -2;
		}
	}
	nRet = m_pINetEventModleEpoll->NetEventModleEpoll_ModEvent(this);
	if (nRet < 0)
	{
		return -1;
	}
#else
	if (0 != m_pPerIoContextRecv->dwRecvLen)
	{
		if (m_bIsHaveHead)  //处理有头数据
		{
			if (m_bIsRecvHead)  //已经接收到头
			{
				if ((m_nNeedRecvDataLen - TcpSession_GetRecvCacheLen()) == m_pPerIoContextRecv->dwRecvLen)
				{
					char *szBuf = new __nothrow char[m_nNeedRecvDataLen + 1];
					memset(szBuf, 0, m_nNeedRecvDataLen + 1);
					int nRet = TcpSession_GetRecvCacheBuf(szBuf);
					memcpy(szBuf + nRet, m_pPerIoContextRecv->szBuffer, m_pPerIoContextRecv->dwRecvLen);
					if (0 == m_nRecvDataType)
					{
						TcpSession_HeadleHB(szBuf, m_nNeedRecvDataLen);
					}
					else if (1 == m_nRecvDataType)
					{
						if (m_pITransPortEngineRecv)
						{
							m_pITransPortEngineRecv->OnTransPortEngineRecv(static_cast<int>(m_Socket), m_nNeedRecvDataLen, szBuf, this);
						}
					}
					m_bIsRecvHead = false;
					m_nNeedRecvDataLen = 0;
				}
				else if ((m_nNeedRecvDataLen - TcpSession_GetRecvCacheLen()) < m_pPerIoContextRecv->dwRecvLen)
				{
					LP_BUFINFO_T pBufInfo = new __nothrow BUFINFO_T;
					memset(pBufInfo, 0, sizeof(BUFINFO_T));
					pBufInfo->nBufLen = m_pPerIoContextRecv->dwRecvLen;
					pBufInfo->szBuf = new __nothrow char[m_pPerIoContextRecv->dwRecvLen + 1];
					memset(pBufInfo->szBuf, 0, m_pPerIoContextRecv->dwRecvLen + 1);
					memcpy(pBufInfo->szBuf, m_pPerIoContextRecv->szBuffer, m_pPerIoContextRecv->dwRecvLen);
					CSSAutoLock AutoLock(&m_RecvBufLock);
					m_RecvBufs.push_back(pBufInfo);
				}
				else if ((m_nNeedRecvDataLen - TcpSession_GetRecvCacheLen()) > m_pPerIoContextRecv->dwRecvLen)
				{
					return -1;
				}
			}
			else //未接收到头
			{
				if (m_pPerIoContextRecv->dwRecvLen > (sizeof(HEAD_MSG_T) - TcpSession_GetRecvCacheLen()))
				{
					return -1;
				}
				else if (m_pPerIoContextRecv->dwRecvLen < (sizeof(HEAD_MSG_T) - TcpSession_GetRecvCacheLen()))
				{
					LP_BUFINFO_T pBufInfo = new __nothrow BUFINFO_T;
					memset(pBufInfo, 0, sizeof(BUFINFO_T));
					pBufInfo->nBufLen = m_pPerIoContextRecv->dwRecvLen;
					pBufInfo->szBuf = new __nothrow char[m_pPerIoContextRecv->dwRecvLen + 1];
					memset(pBufInfo->szBuf, 0, m_pPerIoContextRecv->dwRecvLen + 1);
					memcpy(pBufInfo->szBuf, m_pPerIoContextRecv->szBuffer, m_pPerIoContextRecv->dwRecvLen);
					CSSAutoLock AutoLock(&m_RecvBufLock);
					m_RecvBufs.push_back(pBufInfo);
				}
				else if (m_pPerIoContextRecv->dwRecvLen == (sizeof(HEAD_MSG_T) - TcpSession_GetRecvCacheLen()))
				{
					char *szBuf = new __nothrow char[sizeof(HEAD_MSG_T)];
					memset(szBuf, 0, sizeof(HEAD_MSG_T));
					int nRet = TcpSession_GetRecvCacheBuf(szBuf);
					memcpy(szBuf + nRet, m_pPerIoContextRecv->szBuffer, m_pPerIoContextRecv->dwRecvLen);
					LP_HEAD_MSG_T pHeadMsg = reinterpret_cast<LP_HEAD_MSG_T>(szBuf);
					if (!memcmp(pHeadMsg->szHeadType, HEART_BEAT, sizeof(HEART_BEAT)))
					{
						m_nRecvDataType = 0;
						m_nNeedRecvDataLen = ntohl(pHeadMsg->nDataLen);
						m_uSaltValue = ntohs(pHeadMsg->uSaltValue);
						if (m_nNeedRecvDataLen > 0)
						{
							m_bIsRecvHead = true;
						}
						else
						{
							TcpSession_HeadleHB(NULL, 0);
						}
						nRet = TcpSession_IOSend(szBuf, sizeof(HEAD_MSG_T));
						if (nRet < 0)
						{
							cout << "TcpSession_IOSend HB Error\n" << endl;
						}
					}
					else if (!memcmp(pHeadMsg->szHeadType, RECV_DATA, sizeof(RECV_DATA)))
					{
						m_nRecvDataType = 1;
						m_nNeedRecvDataLen = ntohl(pHeadMsg->nDataLen);
						m_uSaltValue = ntohs(pHeadMsg->uSaltValue);
						if (m_nNeedRecvDataLen > 0)
						{
							m_bIsRecvHead = true;
						}
					}
					delete[] szBuf;
				}
			}
		}
		else //处理无头数据
		{
			if (m_pITransPortEngineRecv)
			{
				m_pITransPortEngineRecv->OnTransPortEngineRecv(static_cast<int>(m_Socket), m_pPerIoContextRecv->dwRecvLen, m_pPerIoContextRecv->szBuffer, this);
			}
		}
	}
	memset(m_pPerIoContextRecv->szBuffer, 0, MAX_BUFFER_LEN);
	m_pPerIoContextRecv->dwRecvLen = 0;
	DWORD dwFlag = 0;
	if (m_bIsHaveHead)
	{
		if (m_bIsRecvHead)
		{
			m_pPerIoContextRecv->wsaBuf.len = m_nNeedRecvDataLen - TcpSession_GetRecvCacheLen();
		}
		else
		{
			m_pPerIoContextRecv->wsaBuf.len = sizeof(HEAD_MSG_T) - TcpSession_GetRecvCacheLen();
		}
	}
	else
	{
		m_pPerIoContextRecv->wsaBuf.len = MAX_BUFFER_LEN;
	}
	if (false == m_bIsToBeClose)
	{
		int nRet = WSARecv(m_Socket, &m_pPerIoContextRecv->wsaBuf, 1, &m_pPerIoContextRecv->dwRecvLen, &dwFlag, &m_pPerIoContextRecv->Overlapped, NULL);
		if (nRet == INVALID_SOCKET && (WSA_IO_PENDING != WSAGetLastError()))
		{
			return -1;
		}
	}
#endif
	return 0;
}

unsigned int CTcpSession::TcpSession_GetRecvCacheLen()
{
	int nTotalLen = 0;
	CSSAutoLock AutoLock(&m_RecvBufLock);
	for (unsigned int i = 0; i < m_RecvBufs.size(); i++)
	{
		nTotalLen += m_RecvBufs[i]->nBufLen;
	}
	return nTotalLen;
}

int CTcpSession::TcpSession_GetRecvCacheBuf(char *szBuf)
{
	int nTotalLen = 0;
	CSSAutoLock AutoLock(&m_RecvBufLock);
	for (unsigned int i = 0; i < m_RecvBufs.size(); i++)
	{
		memcpy(szBuf + nTotalLen, m_RecvBufs[i]->szBuf, m_RecvBufs[i]->nBufLen);
		nTotalLen += m_RecvBufs[i]->nBufLen;
		delete[] m_RecvBufs[i]->szBuf;
		delete m_RecvBufs[i];
	}
	m_RecvBufs.clear();
	return nTotalLen;
}

int CTcpSession::TcpSession_SendData(const char *szData, const int nDataLen)
{
	if (m_bIsHaveHead)
	{
		HEAD_MSG_T HeadMsg = { 0 };
		HeadMsg.nDataLen = htonl(nDataLen);
		HeadMsg.uSaltValue = htons(0);
		memcpy(HeadMsg.szHeadType, RECV_DATA, sizeof(RECV_DATA));

		char *szBuf = new char[nDataLen + sizeof(HEAD_MSG_T)];
		memset(szBuf, 0, nDataLen + sizeof(HEAD_MSG_T));
		memcpy(szBuf, &HeadMsg, sizeof(HEAD_MSG_T));
		memcpy(szBuf + sizeof(HEAD_MSG_T), szData, nDataLen);
		return TcpSession_IOSend(szBuf, nDataLen + sizeof(HEAD_MSG_T));
	}
	else
	{
		return TcpSession_IOSend(szData, nDataLen);
	}
}

int CTcpSession::TcpSession_IOSend(const char *szData, const int nDataLen)
{
	if (NULL == szData || 0 >= nDataLen)
	{
		return -1;
	}
#ifndef _WINDOWS
	LP_BUFINFO_T pBufInfo = new BUFINFO_T;
	memset(pBufInfo, 0, sizeof(BUFINFO_T));

	pBufInfo->szBuf = new char[nDataLen + 1];
	memset(pBufInfo->szBuf, 0, nDataLen + 1);
	memcpy(pBufInfo->szBuf, szData, nDataLen);
	pBufInfo->nBufLen = nDataLen;
	CSSAutoLock AutoLock(&m_SendBufLock);
	m_SendBufs.push_back(pBufInfo);

	int nRet = m_pINetEventModleEpoll->NetEventModleEpoll_ModEvent(this);
	if (nRet < 0)
	{
		return -2;
	}
#else
	if (false == m_bIsToBeClose)
	{
		LP_PER_IO_CONTEXT_SEND_T pPerIoContextSend = new PER_IO_CONTEXT_SEND_T;
		if (NULL == pPerIoContextSend)
		{
			return -1;
		}
		memset(pPerIoContextSend, 0, sizeof(PER_IO_CONTEXT_SEND_T));
		pPerIoContextSend->dwSendLen = 0;
		pPerIoContextSend->nSquence = ++m_nSquence;
		pPerIoContextSend->OpType = IO_SEND;
		pPerIoContextSend->Overlapped = { 0 };
		pPerIoContextSend->sockSend = m_Socket;
		pPerIoContextSend->szBuffer = new char[nDataLen + 1];
		memset(pPerIoContextSend->szBuffer, 0, nDataLen + 1);
		memcpy(pPerIoContextSend->szBuffer, szData, nDataLen);
		pPerIoContextSend->wsaBuf.len = nDataLen;
		pPerIoContextSend->wsaBuf.buf = pPerIoContextSend->szBuffer;

		DWORD dwFlag = 0;
		int nRet = WSASend(m_Socket, &pPerIoContextSend->wsaBuf, 1, &pPerIoContextSend->dwSendLen, dwFlag, &pPerIoContextSend->Overlapped, NULL);
		if (nRet == INVALID_SOCKET && (WSA_IO_PENDING != WSAGetLastError()))
		{
			delete[] pPerIoContextSend->szBuffer;
			delete pPerIoContextSend;
			return -2;
		}
		CSSAutoLock AutoLock(&m_PerIoContextSendLock);
		m_PerIoContextSends.push_back(pPerIoContextSend);
	}
#endif
	return 0;
}

int CTcpSession::TcpSession_IORecv(int nPostRecvEventCount)
{
#ifndef _WINDOWS
	int nRet = m_pINetEventModleEpoll->NetEventModleEpoll_ModEvent(this);
	if (nRet < 0)
	{
		return -1;
	}
#else
	DWORD dwFlag = 0;
	if (m_bIsHaveHead)
	{
		if (m_bIsRecvHead)
		{
			m_pPerIoContextRecv->wsaBuf.len = m_nNeedRecvDataLen - TcpSession_GetRecvCacheLen();
		}
		else
		{
			m_pPerIoContextRecv->wsaBuf.len = sizeof(HEAD_MSG_T) - TcpSession_GetRecvCacheLen();
		}
	}
	else
	{
		m_pPerIoContextRecv->wsaBuf.len = MAX_BUFFER_LEN;
	}
	if (false == m_bIsToBeClose)
	{
		int nRet = WSARecv(m_Socket, &m_pPerIoContextRecv->wsaBuf, 1, &m_pPerIoContextRecv->dwRecvLen, &dwFlag, &m_pPerIoContextRecv->Overlapped, NULL);
		if (nRet == INVALID_SOCKET && (WSA_IO_PENDING != WSAGetLastError()))
		{
			return -1;
		}
	}
#endif
	return 0;
}

#ifndef _WINDOWS
int CTcpSession::TcpSession_GetSocket()
#else
SOCKET CTcpSession::TcpSession_GetSocket()
#endif
{
#ifndef _WINDOWS
	return m_nSocket;
#else
	return m_Socket;
#endif
}

#ifndef _WINDOWS
bool CTcpSession::TcpSession_IsSendData()
{
	if (m_SendBufs.empty())
	{
		return  false;
	}
	return true;
}

CCritSec* CTcpSession::TcpSession_GetEpollEventLock()
{
	return &m_EpollEventLock;
}


#else
LP_PER_IO_CONTEXT_RECV_T CTcpSession::TcpSession_GetPerIoContextRecv()
{
	return m_pPerIoContextRecv;
}

int CTcpSession::TcpSession_CancelIoSend()
{
	CSSAutoLock AutoLock(&m_PerIoContextSendLock);

	for (int i = 0; i < m_PerIoContextSends.size(); i++)
	{
		if (!CancelIoEx((HANDLE)m_Socket, &m_PerIoContextSends[i]->Overlapped))
		{
			if (WSA_IO_PENDING == WSAGetLastError())
			{
				continue;
			}
		}

	}
	return 0;
}

int CTcpSession::TcpSession_IoSendCount()
{
	CSSAutoLock AutoLock(&m_PerIoContextSendLock);
	m_bIsToBeClose = true;
	return static_cast<int>(m_PerIoContextSends.size());
}

bool CTcpSession::TcpSession_IsToBeClose()
{
	return m_bIsToBeClose;
}

CCritSec* CTcpSession::TcpSession_GetIOCPEventLock()
{
	return &m_IOCPEventLock;
}

#endif

int  CTcpSession::TcpSession_GetNetEventModleIndex()
{
	return m_nEventModleIndex;
}

bool CTcpSession::TcpSession_IsHeartbeatTimeOut()
{
	if (time(NULL) - m_tHeartbeatTimeStamp > HEARTBEAT_TIMEOUT)
	{
		return true;
	}
	return false;
}

int CTcpSession::TcpSession_HeadleHB(char *szData, const int nDataLen)
{

	m_tHeartbeatTimeStamp = time(NULL);
	return 0;
}


