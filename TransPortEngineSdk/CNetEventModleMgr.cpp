#include "CNetEventModleMgr.h"

//CNetEventModleMgr* CNetEventModleMgr::m_pNetEventModleMgr = NULL;

CNetEventModleMgr::CNetEventModleMgr()
{
	m_nNetEventModleIndex = 0;
}


CNetEventModleMgr::~CNetEventModleMgr()
{
	for (unsigned int i = 0; i < m_NetEventModles.size(); i++)
	{
		delete m_NetEventModles[i];
	}
	m_NetEventModles.clear();
}
/*
CNetEventModleMgr* CNetEventModleMgr::CreateInstance()
{
	if (NULL == m_pNetEventModleMgr)
	{
		m_pNetEventModleMgr = new CNetEventModleMgr;
		if (NULL == m_pNetEventModleMgr)
		{
			return NULL;
		}
	}
	return m_pNetEventModleMgr;
}

int CNetEventModleMgr::DestroyInstance()
{
	if (m_pNetEventModleMgr)
	{
		delete m_pNetEventModleMgr;
	}
	return 0;
}
*/
int CNetEventModleMgr::NetEventModleMgr_Init(int nNetEventModleCount, int nNetEventModleThreadCount, void *pTcpSessionMgr)
{
	printf("nNetEventModleCount:%d nNetEventModleThreadCount:%d\n", nNetEventModleCount, nNetEventModleThreadCount);
	if (0 == nNetEventModleCount || 0 == nNetEventModleThreadCount || NULL == pTcpSessionMgr)
	{
		return -1;
	}
	m_nNetEventModleCount = nNetEventModleCount;
	m_nNetEventModleThreadCount = nNetEventModleThreadCount;
	for (int i = 0; i < nNetEventModleCount; i++)
	{
		m_nNetEventModleIndex++;
#ifndef _WINDOWS
		CNetEventModleEpoll *pNetEventModleEpoll = new CNetEventModleEpoll;
		if (NULL == pNetEventModleEpoll)
		{
			return -2;
		}
		int nRet = pNetEventModleEpoll->NetEventModleEpoll_Init(i, m_nNetEventModleThreadCount, pTcpSessionMgr);
		if (nRet < 0)
		{
			return -3;
		}
		CSSAutoLock AutoLock(&m_NetEventModleLock);
		printf("Index:%d pNetEventModleEpoll:%p\n", i, pNetEventModleEpoll);
		m_NetEventModles.insert(map<int, CNetEventModleEpoll*>::value_type(i, pNetEventModleEpoll));
#else
		CNetEventModleIOCP *pNetEventModleIOCP = new CNetEventModleIOCP;
		if (NULL == pNetEventModleIOCP)
		{
			return -2;
		}
		int nRet = pNetEventModleIOCP->NetEventModleIOCP_Init(i, m_nNetEventModleThreadCount, pTcpSessionMgr);
		if (nRet < 0)
		{
			return -3;
		}
		CSSAutoLock AutoLock(&m_NetEventModleLock);
		m_NetEventModles.insert(map<int, CNetEventModleIOCP*>::value_type(i, pNetEventModleIOCP));
#endif
	}
	return 0;
}

int CNetEventModleMgr::NetEventModleMgr_Fini()
{
#ifndef _WINDOWS
	map<int, CNetEventModleEpoll*>::iterator NetEventModleIt = m_NetEventModles.begin();
#else
	map<int, CNetEventModleIOCP*>::iterator NetEventModleIt = m_NetEventModles.begin();
#endif
	for (; NetEventModleIt != m_NetEventModles.end(); NetEventModleIt++)
	{
#ifndef _WINDOWS
		int nRet = NetEventModleIt->second->NetEventModleEpoll_Fini();
		if (nRet < 0)
		{
			return -1;
		}
#else
		int nRet = NetEventModleIt->second->NetEventModleIOCP_Fini();
		if (nRet < 0)
		{
			return -1;
		}
#endif
		delete NetEventModleIt->second;
	}

	return 0;
}

#ifndef _WINDOWS
int CNetEventModleMgr::NetEventModleMgr_AddModle(int nNetEventModleIndex, CNetEventModleEpoll* pNetEventModle)
#else
int CNetEventModleMgr::NetEventModleMgr_AddModle(int nNetEventModleIndex, CNetEventModleIOCP* pNetEventModle)
#endif
{
	CSSAutoLock AutoLock(&m_NetEventModleLock);
#ifndef _WINDOWS
	m_NetEventModles.insert(map<int, CNetEventModleEpoll*>::value_type(nNetEventModleIndex, pNetEventModle));
#else
	m_NetEventModles.insert(map<int, CNetEventModleIOCP*>::value_type(nNetEventModleIndex, pNetEventModle));
#endif
	return 0;
}

int CNetEventModleMgr::NetEventModleMgr_DelModle(int nNetEventModleIndex)
{
	CSSAutoLock AutoLock(&m_NetEventModleLock);
#ifndef _WINDOWS
	map<int, CNetEventModleEpoll*>::iterator NetEventModleIt = m_NetEventModles.find(nNetEventModleIndex);
#else
	map<int, CNetEventModleIOCP*>::iterator NetEventModleIt = m_NetEventModles.find(nNetEventModleIndex);
#endif
	if (NetEventModleIt == m_NetEventModles.end())
	{
		return -1;
	}
	m_NetEventModles.erase(NetEventModleIt);
	return 0;
}

#ifndef _WINDOWS
CNetEventModleEpoll* CNetEventModleMgr::NetEventModleMgr_FindModle(int nNetEventModleIndex)
#else
CNetEventModleIOCP* CNetEventModleMgr::NetEventModleMgr_FindModle(int nNetEventModleIndex)
#endif
{
	CSSAutoLock AutoLock(&m_NetEventModleLock);
#ifndef _WINDOWS
	map<int, CNetEventModleEpoll*>::iterator NetEventModleIt = m_NetEventModles.find(nNetEventModleIndex);
#else
	map<int, CNetEventModleIOCP*>::iterator NetEventModleIt = m_NetEventModles.find(nNetEventModleIndex);
#endif
	if (NetEventModleIt == m_NetEventModles.end())
	{
		return NULL;
	}

	return NetEventModleIt->second;
}

