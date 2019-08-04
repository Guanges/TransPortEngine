#pragma once
#ifndef _WINDOWS
#include "CNetEventModleEpoll.h"
#else
#include "CNetEventModleIOCP.h"
#endif
#include <vector>
#include <map>
#include "SSAutoLock.h"
using std::vector;
using std::map;

class CNetEventModleMgr
{
public:
	CNetEventModleMgr();
	~CNetEventModleMgr();
	//static CNetEventModleMgr* CreateInstance();
	//static  int DestroyInstance();

	int NetEventModleMgr_Init(int nNetEventModleCount, int nNetEventModleThreadCount, void *pTcpSessionMgr);
	int NetEventModleMgr_Fini();

#ifndef _WINDOWS
	int NetEventModleMgr_AddModle(int nNetEventModleIndex, CNetEventModleEpoll* pNetEventModle);
	CNetEventModleEpoll* NetEventModleMgr_FindModle(int nNetEventModleIndex);
#else
	int NetEventModleMgr_AddModle(int nNetEventModleIndex, CNetEventModleIOCP* pNetEventModle);
	CNetEventModleIOCP* NetEventModleMgr_FindModle(int nNetEventModleIndex);
#endif
	int NetEventModleMgr_DelModle(int nNetEventModleIndex);
	

private:
	
private:
	//static CNetEventModleMgr*      m_pNetEventModleMgr;
#ifndef _WINDOWS
	map<int, CNetEventModleEpoll *>  m_NetEventModles;
#else
	map<int, CNetEventModleIOCP *>   m_NetEventModles;
#endif
	CCritSec                         m_NetEventModleLock;
	int                              m_nNetEventModleCount;
	int                              m_nNetEventModleThreadCount;
	int                              m_nNetEventModleIndex;
};

