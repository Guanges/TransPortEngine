#pragma once
#include "CTcpSession.h"

typedef struct stThreadInfo
{
#ifndef _WINDOWS
	pthread_t           tWorkThread;
#else
	HANDLE              hWorkThread;
#endif
	bool                bWorkThreadFlag;
	void*               pNetEventModleEpoll;
	stThreadInfo()
	{
#ifndef _WINDOWS
		tWorkThread = 0;
#else
		hWorkThread = NULL;
#endif
		bWorkThreadFlag = true;
		pNetEventModleEpoll = NULL;
	}
}THREADINFO_T, *LP_THREADINFO_T;

#ifndef _WINDOWS
class CNetEventModleEpoll:public CTcpSession::INetEventModleEpoll
#else
class CNetEventModleEpoll
#endif
{
public:
	CNetEventModleEpoll();
	virtual ~CNetEventModleEpoll();

	int NetEventModleEpoll_Init(int nNetEventModleIndex, int nNetEventModleThreadCount, void *pTcpSessionMgr);
	int NetEventModleEpoll_Fini();

	virtual int NetEventModleEpoll_AddEvent(CTcpSession* pTcpSession);
	virtual int NetEventModleEpoll_ModEvent(CTcpSession* pTcpSession);
	virtual int NetEventModleEpoll_DelEvent(CTcpSession* pTcpSession);

	int NetEventModleEpoll_HandleData(LP_THREADINFO_T pThreadInfo);

	virtual int NetEventModleEpoll_GetEpoll();
private:
	int                      m_nSocket;
	int                      m_nHandleDataEpoll;
	vector<LP_THREADINFO_T>  m_WorkThreadInfos;
	int                      m_nNetEventModleIndex;
	int                      m_nNetEventModleThreadCount;
	CCritSec                 m_WorkThreadLock;
	void          *m_pTcpSessionMgr;
};

