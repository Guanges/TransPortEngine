// TcpServerTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include "TransPortEngineSdk.h"

#pragma comment(lib, "TransPortEngineSdk.lib")

class TransPortRecv :public ITransPortEngineRecv
{
public:
	virtual void OnTransPortEngineRecv(int nSessionId, const unsigned int nRecvDataLen, const char *szRecvData, void *pUser);
	virtual void OnTransPortEngineNotify(int nSessionId, int nMsgType, char *szMsgDescribe, void *pUser);

	int TransPortRecv_Init();

	int TransPortRecv_Unit();
private:
	void* m_pTransPortEngine;
};

void TransPortRecv::OnTransPortEngineRecv(int nSessionId, const unsigned int nRecvDataLen, const char *szRecvData, void *pUser)
{
	printf("%s %d\n", szRecvData, nRecvDataLen);

	TransPortEngine_ServerSendTo(pUser, nRecvDataLen, szRecvData, NULL);

}

int nConnectCount = 0;
int nDisconnectCount = 0;

void TransPortRecv::OnTransPortEngineNotify(int nSessionId, int nMsgType, char *szMsgDescribe, void *pUser)
{
	if (nMsgType == CONNECT)
	{
		nConnectCount++;
	}
	else if (nMsgType == DISCONNECT)
	{
		nDisconnectCount++;
	}
	printf("%d %d %d %d\n", nSessionId, nMsgType, nConnectCount, nDisconnectCount);
}

int TransPortRecv::TransPortRecv_Init()
{
	int nRet = TransPortEngine_Init();
	if (nRet < 0)
	{
		return -1;
	}
	m_pTransPortEngine = TransPortEngine_CreateServer(13250, 1, 3, this, false, true);
	if (NULL == m_pTransPortEngine)
	{
		return -2;
	}
	return 0;
}

int TransPortRecv::TransPortRecv_Unit()
{
	int nRet = TransPortEngine_DestroyServer(m_pTransPortEngine);
	if (nRet < 0)
	{
		return -1;
	}
	return 0;
}

int main()
{
	TransPortRecv *pTransPortRecv = new TransPortRecv;
	int nRet = pTransPortRecv->TransPortRecv_Init();
	if (nRet < 0)
	{
		return -1;
	}
	while (1)
	{
		getchar();
	}

	nRet = pTransPortRecv->TransPortRecv_Unit();
	if (nRet < 0)
	{
		return -2;
	}
	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
