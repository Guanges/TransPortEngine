#include "CNetEventModleEpoll.h"
#include "CTcpSessionMgr.h"

#ifndef _WINDOWS
void* HandleThread(void *vParam)
#else
void HandleThread(void *vParam)
#endif
{
	LP_THREADINFO_T pThreadInfo = reinterpret_cast<LP_THREADINFO_T>(vParam);
	CNetEventModleEpoll* pNetEventModleEpoll = reinterpret_cast<CNetEventModleEpoll*>(pThreadInfo->pNetEventModleEpoll);
	pNetEventModleEpoll->NetEventModleEpoll_HandleData(pThreadInfo);
#ifndef _WINDOWS
	pthread_exit(NULL);
	return NULL;
#else
	_endthread();
#endif
}

CNetEventModleEpoll::CNetEventModleEpoll()
{
	m_nSocket = -1;
	m_nHandleDataEpoll = -1;
	m_WorkThreadInfos.clear();
	m_nNetEventModleIndex = 0;
	m_nNetEventModleThreadCount = 0;
}


CNetEventModleEpoll::~CNetEventModleEpoll()
{
	
}

int CNetEventModleEpoll::NetEventModleEpoll_Init(int nNetEventModleIndex, int nNetEventModleThreadCount, void *pTcpSessionMgr)
{
	if (0 > nNetEventModleIndex || 0 == nNetEventModleThreadCount || NULL == pTcpSessionMgr)
	{
		return -1;
	}
	m_nNetEventModleIndex = nNetEventModleIndex;
	m_nNetEventModleThreadCount = nNetEventModleThreadCount;
	m_pTcpSessionMgr = pTcpSessionMgr;
	m_nHandleDataEpoll = epoll_create(1000);
	if (0 == m_nHandleDataEpoll)
	{
		return -2;
	}

	for (int i = 0; i < m_nNetEventModleThreadCount; i++)
	{
		LP_THREADINFO_T pThreadInfo = new THREADINFO_T;
		memset(pThreadInfo, 0, sizeof(THREADINFO_T));
		pThreadInfo->bWorkThreadFlag = true;
		pThreadInfo->pNetEventModleEpoll = this;

		int nRet = pthread_create(&pThreadInfo->tWorkThread, 0, HandleThread, pThreadInfo);
		if (nRet < 0)
		{
			delete pThreadInfo;
			return -3;
		}
		CSSAutoLock AutoLock(&m_WorkThreadLock);
		m_WorkThreadInfos.push_back(pThreadInfo);
	}

	return 0;
}

int CNetEventModleEpoll::NetEventModleEpoll_Fini()
{
	for (unsigned int i = 0; i < m_WorkThreadInfos.size(); i++)
	{
		if (m_WorkThreadInfos[i]->bWorkThreadFlag)
		{
			m_WorkThreadInfos[i]->bWorkThreadFlag = false;

			pthread_join(m_WorkThreadInfos[i]->tWorkThread, NULL);

			delete m_WorkThreadInfos[i];
		}
	}
	m_WorkThreadInfos.clear();
	if (m_nHandleDataEpoll)
	{
		close(m_nHandleDataEpoll);
		m_nHandleDataEpoll = 0;
	}
	return 0;
}

int CNetEventModleEpoll::NetEventModleEpoll_AddEvent(CTcpSession* pTcpSession)
{
	struct epoll_event EventAdd;
	EventAdd.data.fd = pTcpSession->TcpSession_GetSocket();
	EventAdd.events = EPOLLIN | EPOLLONESHOT;
	if (pTcpSession->TcpSession_IsSendData())
	{
		EventAdd.events |= EPOLLOUT;
	}
	if (-1 == epoll_ctl(m_nHandleDataEpoll, EPOLL_CTL_ADD, pTcpSession->TcpSession_GetSocket(), &EventAdd))
	{
		return -1;
	}
	return 0;
}

int CNetEventModleEpoll::NetEventModleEpoll_ModEvent(CTcpSession* pTcpSession)
{
	struct epoll_event EventAdd;
	EventAdd.data.fd = pTcpSession->TcpSession_GetSocket();
	EventAdd.events = EPOLLIN | EPOLLONESHOT;
	if (pTcpSession->TcpSession_IsSendData())
	{
		EventAdd.events |= EPOLLOUT;
	}
	if (-1 == epoll_ctl(m_nHandleDataEpoll, EPOLL_CTL_MOD, pTcpSession->TcpSession_GetSocket(), &EventAdd))
	{
		return -1;
	}
	return 0;
}

int CNetEventModleEpoll::NetEventModleEpoll_DelEvent(CTcpSession* pTcpSession)
{
	struct epoll_event EventDel;
	if (-1 == epoll_ctl(m_nHandleDataEpoll, EPOLL_CTL_DEL, pTcpSession->TcpSession_GetSocket(), &EventDel))
	{
		return -1;
	}
	return 0;
}

int CNetEventModleEpoll::NetEventModleEpoll_HandleData(LP_THREADINFO_T pThreadInfo)
{
	bool bFlag = false;
	struct epoll_event Epoll_Event = {0};
	printf("Thread Start!\n");
	while (pThreadInfo->bWorkThreadFlag)
	{
		int nNumEvents = epoll_wait(m_nHandleDataEpoll, &Epoll_Event, 1, 1000);
		if (nNumEvents > 0)
		{               
			if (Epoll_Event.events & EPOLLRDHUP)
			{
				printf("Event not EpollOut And Not EpollIn! 1\n");
				continue;
			}

			if (Epoll_Event.events & EPOLLHUP)
			{
				printf("Event not EpollOut And Not EpollIn! 2\n");
				int nRet = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_DelSession(pTcpSession->GetSocket());
				if (nRet < 0)
				{
					printf("TcpSessionMgr_DelSession Error %d\n", pTcpSession->GetSocket());
				}
				else
				{
					CSSAutoLock AutoLock(pTcpSession->TcpSession_GetEpollEventLock());
					nRet = NetEventModleEpoll_DelEvent(pTcpSession);
					if (nRet < 0)
					{
						printf("NetEventModleEpoll_DelEvent Error!\n");
					}
					nRet = pTcpSession->TcpSession_Fini();
					if (nRet < 0)
					{
						printf("TcpSession_Fini Error!\n");
					}
					bFlag = true;
				}
				if (bFlag)
				{
					delete pTcpSession;
				}
				continue;
			}
			if (Epoll_Event.events & EPOLLERR)
			{
				printf("Event not EpollOut And Not EpollIn! 3\n");
				continue;
			}

			if (Epoll_Event.events & EPOLLPRI)
			{
				printf("Event not EpollOut And Not EpollIn! 4\n");
				continue;
			}

			if (Epoll_Event.events & EPOLLOUT)
			{
				bFlag = false;
				CTcpSession* pTcpSession = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_FindSession(nSock);
				if (nullptr == pTcpSession)
                {
					continue;
				}
				int nRet =  pTcpSession->TcpSession_IOSend(1);
				if (nRet < 0)
				{
					printf("TcpSession_IOSend Error! nRet:%d %0x\n", nRet, Epoll_Event.events);
					int nRet = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_DelSession(pTcpSession->GetSocket());
					if (nRet < 0)
					{
						printf("TcpSessionMgr_DelSession Error %d\n", pTcpSession->GetSocket());
					}
					else
					{
						CSSAutoLock AutoLock(pTcpSession->TcpSession_GetEpollEventLock());
						nRet = NetEventModleEpoll_DelEvent(pTcpSession);
						if (nRet < 0)
						{
							printf("NetEventModleEpoll_DelEvent Error!\n");
						}
						nRet = pTcpSession->TcpSession_Fini();
						if (nRet < 0)
						{
							printf("TcpSession_Fini Error!\n");
						}
						bFlag = true;
					}

					if (bFlag)
					{
						delete pTcpSession;
					}
					continue;
				}
			}
			if (Epoll_Event.events & EPOLLIN)
			{
				CTcpSession* pTcpSession = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_FindSession(nSock);
				if (nullptr == pTcpSession)
                {
					continue;
                }
				int nRet =  pTcpSession->TcpSession_IORecv();
				if (nRet < 0) 
				{
					printf("TcpSession_IORecv Error! nRet:%d %0x\n", nRet, Epoll_Event.events);
					int nRet = reinterpret_cast<CTcpSessionMgr*>(m_pTcpSessionMgr)->TcpSessionMgr_DelSession(pTcpSession->GetSocket());
					if (nRet < 0)
					{
						printf("TcpSessionMgr_DelSession Error %d\n", pTcpSession->GetSocket());
					}
					else
					{
						CSSAutoLock AutoLock(pTcpSession->TcpSession_GetEpollEventLock());
						nRet = NetEventModleEpoll_DelEvent(pTcpSession);
						if (nRet < 0)
						{
							printf("NetEventModleEpoll_DelEvent Error!\n");
						}
						nRet = pTcpSession->TcpSession_Fini();
						if (nRet < 0)
						{
							printf("TcpSession_Fini Error!\n");
						}
						bFlag = true;
					}
					if (bFlag)
					{
						delete pTcpSession;
					}
					continue;
				}
			}
		}
	}
	printf("Thread Stop!\n");
	return 0;
}

int CNetEventModleEpoll::NetEventModleEpoll_GetEpoll()
{
	return m_nHandleDataEpoll;
}


