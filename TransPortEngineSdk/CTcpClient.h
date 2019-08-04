#pragma once
#include "CTcpSession.h"


class CTcpClient
{
public:
	CTcpClient(void);
	CTcpClient(char *szIp, int nPort);
	~CTcpClient(void);
	int TcpClient_SocketInit(ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, bool bIsHaveHB, int nHeartBeatInterval);
	int TcpClient_SocketUnit();
	long TcpClient_SocketConnect();
	void TcpClient_SocketDisconnect();
	int  TcpClient_SocketSendMsg(const char *SendBuf, int nBufLen);
	int  TcpClient_SendData(const char *szSendBuf, int nBufLen);
	bool TcpClient_SessionTimeout();
	void TcpClient_Recvfrom();
	int  TcpClient_KeepSessionAlive();
	bool TcpClient_IsExist(char* szIp, int nPort);

	int TcpClient_GetRecvCacheLen();
	int TcpClient_GetRecvCacheBuf(char *szBuf);
	int TcpClient_HandleHB(char *szData, const int nDataLen);
private:
#ifndef _WINDOWS
	int          m_nSocket;
	pthread_t    m_tRecvThread;
	pthread_t    m_tHeartbeatThread;
	struct sockaddr_in  m_AddrServer;
#else
	SOCKET       m_Socket;
	HANDLE       m_hRecvThread;
	HANDLE       m_hHeartbeatThread;
	SOCKADDR_IN  m_AddrServer;
#endif
	
	bool         m_nLinkStatus;
	char         m_szIp[256];
	int          m_nPort;
	bool         m_bRecvFlag;
	time_t       m_tHeartbeatTimeStamp;
	int          m_nMsgType;
	bool         m_bHeartbeatFlag;
	ITransPortEngineRecv            *m_pITransPortEngineRecv;
	bool                             m_bIsHaveHB;
	bool                             m_bIsHaveHead;
	bool                             m_bIsRecvHead;
	int                              m_nHeartBeatInterval;
	int                              m_nNeedRecvDataLen;
	deque<LP_BUFINFO_T>              m_RecvBufs;
	CCritSec                         m_RecvBufLock;
};




