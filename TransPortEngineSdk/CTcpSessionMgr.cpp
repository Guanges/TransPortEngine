#include "CTcpSessionMgr.h"

//CTcpSessionMgr* CTcpSessionMgr::m_pTcpSessionMgr = NULL;

#ifndef _WINDOWS
void* HeartbeatDetectionThread(void *vParam)
#else
void HeartbeatDetectionThread(void *vParam)
#endif
{
	CTcpSessionMgr* pAcceptSession =  reinterpret_cast<CTcpSessionMgr*>(vParam);
	pAcceptSession->TcpSessionMgr_HeartbeatDetection();
#ifndef _WINDOWS
	return NULL;
#else
	_endthread();
#endif
}

CTcpSessionMgr::CTcpSessionMgr()
{

}


CTcpSessionMgr::~CTcpSessionMgr()
{

}
/*
CTcpSessionMgr* CTcpSessionMgr::CreateInstance()
{
	if (NULL == m_pTcpSessionMgr)
	{
		m_pTcpSessionMgr = new CTcpSessionMgr;
		if (NULL == m_pTcpSessionMgr)
		{
			return NULL;
		}
	}
	return m_pTcpSessionMgr;
}

int CTcpSessionMgr::DestroyInstance()
{
	if (m_pTcpSessionMgr)
	{
		delete m_pTcpSessionMgr;
		m_pTcpSessionMgr = NULL;
	}
	return 0;
}
*/
int CTcpSessionMgr::TcpSessionMgr_Init(bool bIsHeartbeat, CNetEventModleMgr *pNetEventModelMgr)
{
	if (NULL == pNetEventModelMgr)
	{
		return -1;
	}
	m_bIsHeartbeat = bIsHeartbeat;
	m_pBindNetEventModels = pNetEventModelMgr;
	
	m_TcpSessions.clear();

#ifndef _WINDOWS
	int nRet = pthread_create(&m_tHeartbeatDetection, 0, HeartbeatDetectionThread, this);
	if (nRet < 0)
	{
		return -2;
	}
#else
	m_hHeartbeatDetection = (HANDLE)_beginthread(HeartbeatDetectionThread, 0, this);
	if (NULL == m_hHeartbeatDetection)
	{
		return -2;
	}
#endif

	return 0;
}

int CTcpSessionMgr::TcpSessionMgr_Fini()
{
#ifndef _WINDOWS
	if (m_bIsHeartbeat)
	{
		m_bIsHeartbeat = false;
		pthread_join(m_tHeartbeatDetection, nullptr);
	}
#else
	if (m_bIsHeartbeat)
	{
		m_bIsHeartbeat = false;
		WaitForSingleObject(m_hHeartbeatDetection, INFINITE);
	}
#endif


	CSSAutoLock AutoLock(&m_TcpSessionMgrLock);
#ifndef _WINDOWS
	map<int, CTcpSession*>::iterator TcpSessionIt = m_TcpSessions.begin();
#else
	map<SOCKET, CTcpSession*>::iterator TcpSessionIt = m_TcpSessions.begin();
#endif
	for (; TcpSessionIt != m_TcpSessions.end(); TcpSessionIt++)
	{
		TcpSessionIt->second->TcpSession_Fini();
		delete TcpSessionIt->second;
	}
	m_TcpSessions.clear();
	return 0;
}

int CTcpSessionMgr::TcpSessionMgr_AddSession(CTcpSession* pTcpSession)
{
	CSSAutoLock AutoLock(&m_TcpSessionMgrLock);
#ifndef _WINDOWS
	m_TcpSessions.insert(map<int, CTcpSession*>::value_type(pTcpSession->TcpSession_GetSocket(), pTcpSession));
#else
	m_TcpSessions.insert(map<SOCKET, CTcpSession*>::value_type(pTcpSession->TcpSession_GetSocket(), pTcpSession));
#endif
	return 0;
}

#ifndef _WINDOWS
int CTcpSessionMgr::TcpSessionMgr_DelSession(int nSocket)
#else
int CTcpSessionMgr::TcpSessionMgr_DelSession(SOCKET Socket)
#endif
{
	CSSAutoLock AutoLock(&m_TcpSessionMgrLock);
#ifndef _WINDOWS
	map<int, CTcpSession*>::iterator TcpSessionIt = m_TcpSessions.find(nSocket);
#else
	map<SOCKET, CTcpSession*>::iterator TcpSessionIt = m_TcpSessions.find(Socket);
#endif
	if (TcpSessionIt == m_TcpSessions.end())
	{
		return -1;
	}

	m_TcpSessions.erase(TcpSessionIt);

	return 0;

}

#ifndef _WINDOWS
CTcpSession* CTcpSessionMgr::TcpSessionMgr_FindSession(int nSocket)
#else
CTcpSession* CTcpSessionMgr::TcpSessionMgr_FindSession(SOCKET Socket)
#endif
{
	CSSAutoLock AutoLock(&m_TcpSessionMgrLock);
#ifndef _WINDOWS
	map<int, CTcpSession*>::iterator TcpSessionIt = m_TcpSessions.find(nSocket);
#else
	map<SOCKET, CTcpSession*>::iterator TcpSessionIt = m_TcpSessions.find(Socket);
#endif
	if (TcpSessionIt == m_TcpSessions.end())
	{
		return NULL;
	}
	return TcpSessionIt->second;
}

void CTcpSessionMgr::TcpSessionMgr_HeartbeatDetection()
{
	bool bFlag = false;
	while (m_bIsHeartbeat)
	{
#ifndef _WINDOWS
		sleep(10);
#else
		Sleep(10000);
#endif
		{
			CSSAutoLock AutoLock(&m_TcpSessionMgrLock);
#ifndef _WINDOWS
			map<int, CTcpSession*>::iterator TcpSessionIt = m_TcpSessions.begin();
#else
			map<SOCKET, CTcpSession*>::iterator TcpSessionIt = m_TcpSessions.begin();
#endif
			for (; TcpSessionIt != m_TcpSessions.end();)
			{
				bFlag = false;
				CTcpSession* pTcpSession = TcpSessionIt->second;
				{
#ifndef _WINDOWS
					//CSSAutoLock AutoLock(pTcpSession->TcpSession_GetEpollEventLock());
#else
					CSSAutoLock AutoLock(pTcpSession->TcpSession_GetIOCPEventLock());
#endif
					if (pTcpSession->TcpSession_IsHeartbeatTimeOut())
					{
						int nRet = 0;
						m_TcpSessions.erase(TcpSessionIt++);
#ifndef _WINDOWS
						nRet = m_pBindNetEventModels->NetEventModleMgr_FindModle(pTcpSession->TcpSession_GetNetEventModleIndex())->NetEventModleEpoll_DelEvent(pTcpSession);
						if (nRet < 0)
						{
							LOG_ERROR("NetEventModleEpoll_DelEvent Error!");
						}
#else
						nRet = m_pBindNetEventModels->NetEventModleMgr_FindModle(pTcpSession->TcpSession_GetNetEventModleIndex())->NetEventModleIOCP_DelEvent(pTcpSession);
						if (nRet < 0)
						{
							LOG_ERROR("NetEventModleIOCP_DelEvent Error!");
						}
#endif

						nRet = pTcpSession->TcpSession_Fini();
						if (nRet < 0)
						{
							LOG_ERROR("TcpSession_Fini Error!");
						}
						bFlag = true;
					}
					else
					{
						++TcpSessionIt;
					}
				}
				if (bFlag)
				{
					delete pTcpSession;
				}
			}
			LOG_INFO("TCPSession Size:{}", static_cast<int>(m_TcpSessions.size()));
		}
	}
}

