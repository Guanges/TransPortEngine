#pragma once

#ifndef _WINDOWS
#else
#endif
#include <map>
using std::map;
#include "CTcpSession.h"
#include "CNetEventModleMgr.h"


class CTcpSessionMgr
{
public:
	CTcpSessionMgr();
	~CTcpSessionMgr();
	//static CTcpSessionMgr* CreateInstance();
	//static int DestroyInstance();

	int TcpSessionMgr_Init(bool bIsHeartbeat, CNetEventModleMgr *pNetEventModelMgr);
	int TcpSessionMgr_Fini();

	int TcpSessionMgr_AddSession(CTcpSession* pTcpSession);
	
#ifndef _WINDOWS
	CTcpSession* TcpSessionMgr_FindSession(int nSocket);
	int TcpSessionMgr_DelSession(int nSocket);
#else
	CTcpSession* TcpSessionMgr_FindSession(SOCKET Socket);
	int TcpSessionMgr_DelSession(SOCKET Socket);
#endif
	void TcpSessionMgr_HeartbeatDetection();
private:
	
private:
	//static CTcpSessionMgr*     m_pTcpSessionMgr;
#ifndef _WINDOWS
	map<int, CTcpSession*>       m_TcpSessions;
	pthread_t                    m_tHeartbeatDetection;
#else
	map<SOCKET, CTcpSession*>    m_TcpSessions;
	HANDLE                       m_hHeartbeatDetection;
#endif
	CCritSec                     m_TcpSessionMgrLock;
	bool                         m_bIsHeartbeat;
	CNetEventModleMgr         *  m_pBindNetEventModels;
};

