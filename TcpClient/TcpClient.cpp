// TcpClient.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
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

	int TransPortRecv_SendData(int nSendDataLen, char* szSendData);
private:
	void* m_pTransPortEngine;
};

void TransPortRecv::OnTransPortEngineRecv(int nSessionId, const unsigned int nRecvDataLen, const char *szRecvData, void *pUser)
{
	printf("%s %d\n", szRecvData, nRecvDataLen);
}

void TransPortRecv::OnTransPortEngineNotify(int nSessionId, int nMsgType, char *szMsgDescribe, void *pUser)
{
	printf("%d %d\n", nSessionId, nMsgType);
}

int TransPortRecv::TransPortRecv_Init()
{
	int nRet = TransPortEngine_Init();
	if (nRet < 0)
	{
		return -1;
	}
	m_pTransPortEngine = TransPortEngine_CreateClient((char *)"47.110.127.155", 13250, this, false, false, 5000);
	if (NULL == m_pTransPortEngine)
	{
		return -2;
	}
	return 0;
}

int TransPortRecv::TransPortRecv_Unit()
{
	int nRet = TransPortEngine_DestroyClient(m_pTransPortEngine);
	if (nRet < 0)
	{
		return -1;
	}
	return 0;
}

int TransPortRecv::TransPortRecv_SendData(int nSendDataLen, char* szSendData)
{
	int nRet = TransPortEngine_ClientSendData(m_pTransPortEngine, nSendDataLen, szSendData, NULL);
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
		pTransPortRecv->TransPortRecv_SendData(11, (char *)"hello world");
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
