#pragma once
#ifndef _SSAUTOLOCK_H
#define _SSAUTOLOCK_H
#ifndef _WINDOWS
#include <pthread.h>
class CCritSec {

	pthread_mutex_t linux_mutex;
public:
	CCritSec() {
		pthread_mutex_init(&linux_mutex, NULL);
	};

	~CCritSec() {
		pthread_mutex_destroy(&linux_mutex);
	};

	void Lock() {
		pthread_mutex_lock(&linux_mutex);
	};

	void Unlock() {
		pthread_mutex_unlock(&linux_mutex);
	};

};
#else
class CCritSec {

	CRITICAL_SECTION m_CritSec;

public:
	CCritSec() {
		InitializeCriticalSection(&m_CritSec);
	};

	~CCritSec() {
		DeleteCriticalSection(&m_CritSec);
	};

	void Lock() {
		EnterCriticalSection(&m_CritSec);
	};

	void Unlock() {
		LeaveCriticalSection(&m_CritSec);
	};

};
#endif


class CSSAutoLock
{
public:
	CSSAutoLock(const CSSAutoLock &refAutoLock);
	CSSAutoLock &operator=(const CSSAutoLock &refAutoLock);

protected:
	CCritSec * m_pLock;

public:
	CSSAutoLock(CCritSec * plock)
	{
		m_pLock = plock;
		m_pLock->Lock();
	};

	~CSSAutoLock() {
		m_pLock->Unlock();
	};
};
#endif //_SSAUTOLOCK_H

