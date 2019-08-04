#pragma once
#include "CTcpSession.h"

typedef struct ThreadInfo
{
#ifndef _WINDOWS
	pthread_t           tWorkThread;
#else
	HANDLE              hWorkThread;
#endif
	bool                bWorkThreadFlag;
	void*               pNetEventModleIOCP;
	struct ThreadInfo()
	{
#ifndef _WINDOWS
		tWorkThread = 0;
#else
		hWorkThread = NULL;
#endif
		bWorkThreadFlag = true;
		pNetEventModleIOCP = NULL;
	}
}THREADINFO_T, *LP_THREADINFO_T;

class CNetEventModleIOCP
{
public:
	CNetEventModleIOCP();
	~CNetEventModleIOCP();

	int NetEventModleIOCP_Init(int nNetEventModleIndex, int nNetEventModleThreadCount, void *pTcpSessionMgr);
	int NetEventModleIOCP_Fini();

	int NetEventModleIOCP_AddEvent(CTcpSession* pTcpSession);
	int NetEventModleIOCP_DelEvent(CTcpSession* pTcpSession);

	int NetEventModleIOCP_HandleData(LP_THREADINFO_T pThreadInfo);

	bool AcceptSession_IsSocketAlive(SOCKET Socket);
	bool AcceptSession_HandleAcceptError(CTcpSession* pTcpSession, SOCKET Socket, const DWORD& dwErrno);
private:
	vector<LP_THREADINFO_T> m_WorkThreadInfos;
	int                     m_nNetEventModleThreadCount;
	HANDLE                  m_hHandleDataIOCP;
	int                     m_nNetEventModleIndex;
	CCritSec                m_WorkThreadLock;
	void                   *m_pTcpSessionMgr;
};

