#include "CAcceptSession.h"
#include "CTcpClient.h"

TRANSPORT_ENGINE_SDK_API int TransPortEngine_Init()
{
#ifndef _WINDOWS
#else
	WSADATA wsaData;
	int nResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (NO_ERROR != nResult)
	{
		return -1;
	}
#endif
	return 0;
}

TRANSPORT_ENGINE_SDK_API int TransPortEngine_Fini()
{
#ifndef _WINDOWS
#else
	WSACleanup();
#endif
	return 0;
}

TRANSPORT_ENGINE_SDK_API void* TransPortEngine_CreateServer(int nListenPort, int nNetEventModleCount, int nNetEventModleThreadCount, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, bool bIsHaveHB)
{
	CAcceptSession *pAcceptSession = new CAcceptSession;
	if (NULL == pAcceptSession)
	{
		return NULL;
	}
	int nRet = pAcceptSession->AcceptSession_Init(nListenPort, nNetEventModleCount, nNetEventModleThreadCount, pITransPortEngineRecv, bIsHaveHead, bIsHaveHB);
	if (nRet < 0)
	{
		delete pAcceptSession;
		return NULL;
	}
	return pAcceptSession;
}

TRANSPORT_ENGINE_SDK_API int TransPortEngine_DestroyServer(void* pTransPortEngine)
{
	CAcceptSession *pAcceptSession = (CAcceptSession*)pTransPortEngine;
	if (NULL == pAcceptSession)
	{
		return -1;
	}
	int nRet = pAcceptSession->AcceptSession_Fini();
	if (nRet < 0)
	{
		return -2;
	}
	delete pAcceptSession;
	return 0;
}

int TransPortEngine_ServerSendData(void* pTransPortEngine, int nSessionId, const unsigned int nSendDataLen, const char *szSendData, const void *pUser)
{
	CAcceptSession *pAcceptSession = (CAcceptSession*)pTransPortEngine;
	if (NULL == pAcceptSession)
	{
		return -1;
	}
	int nRet = pAcceptSession->AcceptSession_SendData(nSessionId, nSendDataLen, szSendData, pUser);
	if (nRet < 0)
	{
		return -2;
	}
	return 0;
}

int TransPortEngine_ServerSendTo(void* pTransPortEngine, const unsigned int nSendDataLen, const char *szSendData, const void *pUser)
{
	CTcpSession *pTcpSession = (CTcpSession*)pTransPortEngine;
	if (NULL == pTcpSession)
	{
		return -1;
	}
	int nRet = pTcpSession->TcpSession_SendData(szSendData, nSendDataLen);
	if (nRet < 0)
	{
		return -2;
	}
	return 0;
}

void* TransPortEngine_CreateClient(char *szServerIp, int nServerPort, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, bool bIsHaveHB, int nHeartBeatInterval)
{
	CTcpClient *pTcpClient = new CTcpClient(szServerIp, nServerPort);
	if (NULL == pTcpClient)
	{
		return NULL;
	}
	int nRet = pTcpClient->TcpClient_SocketInit(pITransPortEngineRecv, bIsHaveHead, bIsHaveHB, nHeartBeatInterval);
	if (nRet < 0)
	{
		delete pTcpClient;
		return NULL;
	}
	return pTcpClient;
}

int TransPortEngine_DestroyClient(void* pTransPortEngine)
{
	CTcpClient *pTcpClient = reinterpret_cast<CTcpClient*>(pTransPortEngine);
	if (NULL == pTcpClient)
	{
		return -1;
	}
	int nRet = pTcpClient->TcpClient_SocketUnit();
	if (nRet < 0)
	{
		return -2;
	}
	delete pTcpClient;
	return 0;
}

int TransPortEngine_ClientSendData(void* pTransPortEngine, const unsigned int nSendDataLen, const char *szSendData, const void *pUser)
{
	CTcpClient *pTcpClient = reinterpret_cast<CTcpClient*>(pTransPortEngine);
	if (NULL == pTcpClient)
	{
		return -1;
	}
	int nRet = pTcpClient->TcpClient_SendData(szSendData, nSendDataLen);
	if (nRet < 0)
	{
		return -2;
	}
	return 0;
}
