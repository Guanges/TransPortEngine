#include "CTcpClient.h"

#ifndef _WINDOWS
void* RecvThread(void *vParam)
#else
void RecvThread(void *vParam)
#endif
{
	CTcpClient *pTcpClient = reinterpret_cast<CTcpClient *>(vParam);

	pTcpClient->TcpClient_Recvfrom();
#ifndef _WINDOWS
	pthread_exit(NULL);
	return nullptr;
#else
	_endthread();
#endif
}

#ifndef _WINDOWS
void* HeartbeatThread(void *vParam)
#else
void HeartbeatThread(void *vParam)
#endif
{
	CTcpClient *pTcpClient = reinterpret_cast<CTcpClient *>(vParam);

	pTcpClient->TcpClient_KeepSessionAlive();
#ifndef _WINDOWS
	pthread_exit(NULL);
	return nullptr;
#else
	_endthread();
#endif
}

CTcpClient::CTcpClient(void)
{
	
}

CTcpClient::CTcpClient(char *szIp, int nPort)
{
	m_nLinkStatus = DISCONNECT;
	memset(m_szIp, 0, 256);
	memcpy(m_szIp, szIp, strlen(szIp));
	m_nPort = nPort;
	m_bRecvFlag = true;
	m_bHeartbeatFlag = true;
#ifndef _WINDOWS
	m_AddrServer.sin_addr.s_addr = inet_addr(szIp);
#else
	m_AddrServer.sin_addr.S_un.S_addr= inet_addr(szIp);//INADDR_ANY表示任何IP
#endif
	m_AddrServer.sin_family=AF_INET;
	m_AddrServer.sin_port=htons(nPort);
	m_pITransPortEngineRecv = nullptr;
	m_bIsHaveHB = false;
	m_bIsHaveHead = false;
	m_bIsRecvHead = false;
	m_nHeartBeatInterval = 5000;
	m_nNeedRecvDataLen = 0;
}

CTcpClient::~CTcpClient(void)
{
}


int CTcpClient::TcpClient_SocketInit(ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, bool bIsHaveHB, int nHeartBeatInterval)
{
	if (NULL == pITransPortEngineRecv)
	{
		return -1;
	}
	m_pITransPortEngineRecv = pITransPortEngineRecv;
	m_bIsHaveHB = bIsHaveHB;
	m_bIsHaveHead = bIsHaveHead;
	m_nHeartBeatInterval = nHeartBeatInterval;

	if (m_bIsHaveHB)
	{
#ifndef _WINDOWS
		int nRet = pthread_create(&m_tHeartbeatThread, 0, HeartbeatThread, this);
		if (nRet < 0)
		{
			return -2;
		}
#else

		m_hHeartbeatThread = (HANDLE)_beginthread(HeartbeatThread, 0, this);
		if (0 == m_hHeartbeatThread)
		{
			return -2;
		}
#endif
	}
	else
	{
		if (m_nLinkStatus == DISCONNECT)
		{
#ifndef _WINDOWS
			m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (m_nSocket <= 0)
			{
				return -2;
			}
#else
			m_Socket = socket(AF_INET, SOCK_STREAM, 0);
			if (m_Socket == INVALID_SOCKET)
			{
				return -2;
			}
#endif
			int nRet = TcpClient_SocketConnect();
			if (nRet < 0)
			{
#ifndef _WINDOWS
				if (m_nSocket)
				{

					close(m_nSocket);
				}
#else
				if (m_Socket)
				{

					closesocket(m_Socket);
				}
#endif
				return -3;
			}

			m_nLinkStatus = CONNECT;

			if (m_pITransPortEngineRecv)
			{
				m_pITransPortEngineRecv->OnTransPortEngineNotify(0, CONNECT, NULL, this);
			}

			m_bRecvFlag = true;
#ifndef _WINDOWS
			nRet = pthread_create(&m_tRecvThread, 0, RecvThread, this);
			if (nRet < 0)
			{
				if (m_nSocket)
				{

					close(m_nSocket);
				}
				return -4;
			}
#else
			m_hRecvThread = (HANDLE)_beginthread(RecvThread, 0, this);
			if (0 == m_hRecvThread)
			{
				if (m_Socket)
				{

					closesocket(m_Socket);
				}
				return -4;
			}
#endif
		}
	}
	return 0;
}

int CTcpClient::TcpClient_SocketUnit()
{
	if (m_bHeartbeatFlag)
	{
		m_bHeartbeatFlag = false;
#ifndef _WINDOWS
		pthread_join(m_tHeartbeatThread, NULL);
#else
		WaitForSingleObject(m_hHeartbeatThread, INFINITE);
#endif
	}

	if (m_bRecvFlag)
	{
		m_bRecvFlag = false;
#ifndef _WINDOWS
		pthread_join(m_tRecvThread, NULL);
#else
		WaitForSingleObject(m_hRecvThread, INFINITE);
#endif
	}
#ifndef _WINDOWS
	if (m_nSocket)
	{

		close(m_nSocket);
	}
#else
	if (m_Socket)
	{

		closesocket(m_Socket);
	}
#endif


	return 0;
}

long CTcpClient::TcpClient_SocketConnect()
{
	int nRet;
#ifndef _WINDOWS
	nRet = connect(m_nSocket, (sockaddr *)&m_AddrServer, sizeof(m_AddrServer));
#else
	nRet = connect(m_Socket, (sockaddr *)&m_AddrServer, sizeof(SOCKADDR_IN));
#endif
	if (nRet)
	{
		return -1;
	}

	return 0;
}

int CTcpClient::TcpClient_SocketSendMsg(const char *SendBuf, int nBufLen)
{
#ifndef _WINDOWS
	return send(m_nSocket, SendBuf, nBufLen, 0);
#else
	return send(m_Socket, SendBuf, nBufLen, 0);
#endif
}

int CTcpClient::TcpClient_SendData(const char *szSendBuf, int nBufLen)
{
	if (m_bIsHaveHead)
	{
		HEAD_MSG_T HeadMsg = { 0 };
		HeadMsg.nDataLen = htonl(nBufLen);
		HeadMsg.uSaltValue = htons(0);
		memcpy(HeadMsg.szHeadType, RECV_DATA, sizeof(RECV_DATA));

		char *szBuf = new char[nBufLen + sizeof(HEAD_MSG_T)];
		memset(szBuf, 0, nBufLen + sizeof(HEAD_MSG_T));
		memcpy(szBuf, &HeadMsg, sizeof(HEAD_MSG_T));
		memcpy(szBuf + sizeof(HEAD_MSG_T), szSendBuf, nBufLen);
		return TcpClient_SocketSendMsg(szBuf, nBufLen + sizeof(HEAD_MSG_T));
	}
	else
	{
		return TcpClient_SocketSendMsg(szSendBuf, nBufLen);
	}
	
}

bool CTcpClient::TcpClient_SessionTimeout()
{
	time_t tNowTime = time(NULL);
	if (tNowTime - m_tHeartbeatTimeStamp >= HEARTBEAT_TIMEOUT)
	{
		return true;
	}
	else
	{
		return false;
	}

}

void CTcpClient::TcpClient_Recvfrom()
{
	int nRet;
	char szRecvBuf[MAX_BUFFER_LEN];
	while(m_bRecvFlag)
	{
		memset(szRecvBuf, 0, MAX_BUFFER_LEN);
		if (m_bIsHaveHead)
		{
			if (m_bIsRecvHead)
			{
#ifndef _WINDOWS
				nRet = recv(m_nSocket, szRecvBuf, m_nNeedRecvDataLen - TcpClient_GetRecvCacheLen(), 0);
#else
				nRet = recv(m_Socket, szRecvBuf, m_nNeedRecvDataLen - TcpClient_GetRecvCacheLen(), 0);
#endif
				if (nRet > 0)
				{
					if (nRet == (m_nNeedRecvDataLen - TcpClient_GetRecvCacheLen()))
					{
						char *szBuf = new char[m_nNeedRecvDataLen];
						memset(szBuf, 0, m_nNeedRecvDataLen);
						int nCopyLen = TcpClient_GetRecvCacheBuf(szBuf);
						memcpy(szBuf + nCopyLen, szRecvBuf, nRet);
						if (0 == m_nMsgType)
						{
							TcpClient_HandleHB(szBuf, m_nNeedRecvDataLen);
						}
						else if (1 == m_nMsgType)
						{
							if (m_pITransPortEngineRecv)
							{
								m_pITransPortEngineRecv->OnTransPortEngineRecv(0, m_nNeedRecvDataLen, szBuf, this);
							}
						}
						delete[] szBuf;
						m_bIsRecvHead = false;
					}
					else if (nRet > (m_nNeedRecvDataLen - TcpClient_GetRecvCacheLen()))
					{
						cout << "recv body error" << endl;
						m_bIsRecvHead = false;
					}
					else if (nRet < (m_nNeedRecvDataLen - TcpClient_GetRecvCacheLen()))
					{
						LP_BUFINFO_T pBufInfo = new BUFINFO_T;
						memset(pBufInfo, 0, sizeof(BUFINFO_T));
						pBufInfo->nBufLen = nRet;
						pBufInfo->szBuf = new char[nRet + 1];
						memset(pBufInfo->szBuf, 0, nRet + 1);
						memcpy(pBufInfo->szBuf, szRecvBuf, nRet);
						CSSAutoLock AutoLock(&m_RecvBufLock);
						m_RecvBufs.push_back(pBufInfo);
					}
					
				}
				else if (nRet == 0)
				{
					break;
				}
				else if (nRet < 0)
				{	
					break;
				}
			}
			else
			{
#ifndef _WINDOWS
				nRet = recv(m_nSocket, szRecvBuf, sizeof(HEAD_MSG_T) - TcpClient_GetRecvCacheLen(), 0);
#else
				nRet = recv(m_Socket, szRecvBuf, sizeof(HEAD_MSG_T) - TcpClient_GetRecvCacheLen(), 0);
#endif
				if (nRet > 0)
				{
					if (nRet == static_cast<int>(sizeof(HEAD_MSG_T) - TcpClient_GetRecvCacheLen()))
					{
						char *szBuf = new char[sizeof(HEAD_MSG_T) + 1];
						memset(szBuf, 0, sizeof(HEAD_MSG_T) + 1);
						int nCopyLen = TcpClient_GetRecvCacheBuf(szBuf);
						memcpy(szBuf + nCopyLen, szRecvBuf, nRet);
						LP_HEAD_MSG_T pMsgHead = (LP_HEAD_MSG_T)szBuf;
						m_nNeedRecvDataLen = ntohl(pMsgHead->nDataLen);
						if (!memcmp(pMsgHead->szHeadType, HEART_BEAT, sizeof(HEART_BEAT)))
						{
							m_nMsgType = 0;
							if (0 == m_nNeedRecvDataLen)
							{
								TcpClient_HandleHB(NULL, 0);
							}
							else
							{
								m_bIsRecvHead = true;
							}
						}
						else if (!memcmp(pMsgHead->szHeadType, RECV_DATA, sizeof(RECV_DATA)))
						{
							m_nMsgType = 1;
							if (0 < m_nNeedRecvDataLen)
							{
								m_bIsRecvHead = true;
							}
						}
						delete[] szBuf;
					}
					else if (nRet > static_cast<int>(sizeof(HEAD_MSG_T) - TcpClient_GetRecvCacheLen()))
					{
						cout << "recv head error" << endl;
					}
					else if (nRet < static_cast<int>(sizeof(HEAD_MSG_T) - TcpClient_GetRecvCacheLen()))
					{
						LP_BUFINFO_T pBufInfo = new BUFINFO_T;
						memset(pBufInfo, 0, sizeof(BUFINFO_T));
						pBufInfo->nBufLen = nRet;
						pBufInfo->szBuf = new char[nRet + 1];
						memset(pBufInfo->szBuf, 0, nRet + 1);
						memcpy(pBufInfo->szBuf, szRecvBuf, nRet);
						CSSAutoLock AutoLock(&m_RecvBufLock);
						m_RecvBufs.push_back(pBufInfo);
					}

				}
				else if (nRet == 0)
				{
					break;
				}
				else if (nRet < 0)
				{
					break;
				}
			}
		}
		else
		{
#ifndef _WINDOWS
			nRet = recv(m_nSocket, szRecvBuf, MAX_BUFFER_LEN, 0);
#else
			nRet = recv(m_Socket, szRecvBuf, MAX_BUFFER_LEN, 0);
#endif
			if (nRet > 0)
			{
				if (m_pITransPortEngineRecv)
				{
					m_pITransPortEngineRecv->OnTransPortEngineRecv(0, nRet, szRecvBuf, this);
				}
			}
			else if (nRet == 0)
			{
				break;
			}
			else if (nRet < 0)
			{
				break;
			}
		}
		
	}
}

int CTcpClient::TcpClient_KeepSessionAlive()
{
	int nRet;
	HEAD_MSG_T MsgHead;

	while(m_bHeartbeatFlag)
	{
#ifndef _WINDOWS
		sleep(m_nHeartBeatInterval);
#else
		Sleep(m_nHeartBeatInterval);
#endif

		if (m_nLinkStatus == DISCONNECT)
		{
#ifndef _WINDOWS
			m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
			if (m_nSocket <= 0)
			{
				continue;
			}
#else
			m_Socket = socket(AF_INET, SOCK_STREAM, 0);
			if (m_Socket == INVALID_SOCKET)
			{
				continue;
			}
#endif
			

			nRet = TcpClient_SocketConnect();
			if (nRet < 0)
			{
				continue;
			}

			m_nLinkStatus = CONNECT;

			if (m_pITransPortEngineRecv)
			{
				m_pITransPortEngineRecv->OnTransPortEngineNotify(0, CONNECT, NULL, this);
			}

			m_bRecvFlag = true;
#ifndef _WINDOWS
			int nRet = pthread_create(&m_tRecvThread, 0, RecvThread, this);
			if (nRet < 0)
			{
				continue;
			}
#else
			m_hRecvThread = (HANDLE)_beginthread(RecvThread, 0, this);
			if (0 == m_hRecvThread)
			{
				continue;
			}
#endif
			m_tHeartbeatTimeStamp = time(NULL);

			memset(&MsgHead, 0, sizeof(HEAD_MSG_T));
			memcpy(MsgHead.szHeadType, HEART_BEAT, sizeof(HEART_BEAT));
			MsgHead.nDataLen = 0;
			MsgHead.uSaltValue = 0;
			char *szSendHeartBeatBuf = new char[sizeof(HEAD_MSG_T) + MsgHead.nDataLen];
			memset(szSendHeartBeatBuf, 0, sizeof(HEAD_MSG_T) + MsgHead.nDataLen);
			memcpy(szSendHeartBeatBuf, &MsgHead, sizeof(HEAD_MSG_T));
			//if (MsgHead.nDataLen > 0)
			//{
			//	memcpy(szSendHeartBeatBuf + sizeof(HEAD_MSG_T), NULL, MsgHead.nDataLen);
			//}
			nRet = TcpClient_SocketSendMsg(szSendHeartBeatBuf, sizeof(HEAD_MSG_T) + MsgHead.nDataLen);
			if (nRet < 0)
			{
				cout << " TcpClient_SocketSendMsg Error" << endl;
			}
			delete[] szSendHeartBeatBuf;
		}
		else if (m_nLinkStatus == CONNECT)
		{
			if (TcpClient_SessionTimeout())
			{
				m_nLinkStatus = DISCONNECT;

				if (m_pITransPortEngineRecv)
				{
					m_pITransPortEngineRecv->OnTransPortEngineNotify(0, DISCONNECT, NULL, this);
				}

				m_bRecvFlag = false;
#ifndef _WINDOWS
				close(m_nSocket);
				pthread_join(m_tRecvThread, NULL);
#else
				closesocket(m_Socket);
				WaitForSingleObject(m_hRecvThread, INFINITE);
#endif
				

				continue;
			}
			memset(&MsgHead, 0, sizeof(HEAD_MSG_T));
			memcpy(MsgHead.szHeadType, HEART_BEAT, sizeof(HEART_BEAT));
			MsgHead.nDataLen = 0;
			MsgHead.uSaltValue = 0;
			
			char *szSendHeartBeatBuf = new char[sizeof(HEAD_MSG_T) + MsgHead.nDataLen];
			memset(szSendHeartBeatBuf, 0, sizeof(HEAD_MSG_T) + MsgHead.nDataLen);
			memcpy(szSendHeartBeatBuf, &MsgHead, sizeof(HEAD_MSG_T));
			//if (MsgHead.nDataLen > 0)
			//{
			//	memcpy(szSendHeartBeatBuf + sizeof(HEAD_MSG_T), NULL, MsgHead.nDataLen);
			//}
			TcpClient_SocketSendMsg(szSendHeartBeatBuf, sizeof(HEAD_MSG_T) + MsgHead.nDataLen);
			delete[] szSendHeartBeatBuf;
		}
	}
	return 0;
}

bool CTcpClient::TcpClient_IsExist(char* szIp, int nPort)
{
	if (!strcmp(szIp, m_szIp) && m_nPort == nPort)
	{
		return true;
	}
	return false;
}

int CTcpClient::TcpClient_GetRecvCacheLen()
{
	int nTotalLen = 0;
	CSSAutoLock AutoLock(&m_RecvBufLock);
	for (unsigned int i = 0; i < m_RecvBufs.size(); i++)
	{
		nTotalLen += m_RecvBufs[i]->nBufLen;
	}
	return nTotalLen;
}

int CTcpClient::TcpClient_GetRecvCacheBuf(char *szBuf)
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

int CTcpClient::TcpClient_HandleHB(char *szData, const int nDataLen)
{
	m_tHeartbeatTimeStamp = time(NULL);
	return 0;
}
