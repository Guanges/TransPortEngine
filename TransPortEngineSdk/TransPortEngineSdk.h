#pragma once
#ifndef _WINDOWS
#define TRANSPORT_ENGINE_SDK_API extern "C"
#else
#define TRANSPORT_ENGINE_SDK_API extern "C" __declspec(dllexport)
#endif

enum MsgType
{
	CONNECT = 0,
	DISCONNECT,

};

class ITransPortEngineRecv
{
public:
	virtual void OnTransPortEngineRecv(int nSessionId, const unsigned int nRecvDataLen, const char *szRecvData, void *pUser) = 0;
	virtual void OnTransPortEngineNotify(int nSessionId, int nMsgType, char *szMsgDescribe, void *pUser) = 0;
};

TRANSPORT_ENGINE_SDK_API int TransPortEngine_Init();
TRANSPORT_ENGINE_SDK_API int TransPortEngine_Fini();


TRANSPORT_ENGINE_SDK_API void* TransPortEngine_CreateServer(int nListenPort, int nNetEventModleCount, int nNetEventModleThreadCount, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, bool bIsHaveHB);
TRANSPORT_ENGINE_SDK_API int TransPortEngine_DestroyServer(void* pTransPortEngine);
TRANSPORT_ENGINE_SDK_API int TransPortEngine_ServerSendData(void* pTransPortEngine, int nSessionId, const unsigned int nSendDataLen, const char *szSendData, const void *pUser);
TRANSPORT_ENGINE_SDK_API int TransPortEngine_ServerSendTo(void* pTransPortEngine, const unsigned int nSendDataLen, const char *szSendData, const void *pUser);

TRANSPORT_ENGINE_SDK_API void* TransPortEngine_CreateClient(char *szServerIp, int nServerPort, ITransPortEngineRecv *pITransPortEngineRecv, bool bIsHaveHead, bool bIsHaveHB, int nHeartBeatInterval);
TRANSPORT_ENGINE_SDK_API int TransPortEngine_DestroyClient(void* pTransPortEngine);
TRANSPORT_ENGINE_SDK_API int TransPortEngine_ClientSendData(void* pTransPortEngine, const unsigned int nSendDataLen, const char *szSendData, const void *pUser);
