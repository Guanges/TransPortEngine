#pragma once
#include <WinSock.h>
#pragma comment(lib, "ws2_32.lib")
#define ntohll(x) (((_int64)(ntohl((int)((x << 32) >> 32))) << 32) | (unsigned int)ntohl(((int)(x >> 32))))
#define htonll(x) ntohll(x)



class CBtlByteStream
{
public: //Constructor & destructor
	//obsolete constructor, NOT recommended, use another instead
	CBtlByteStream(BYTE* pBuf, bool bNetOrder = true/*网络字节顺序*/) :
		m_pBuf(pBuf)
			,m_dwCurPos(0)
			,m_bNetOrder(bNetOrder)
			,m_bManaged(false)
			,m_dwMaxSize(0)	//no use, if not managed
		{
			assert(m_pBuf);
		}
		CBtlByteStream(DWORD dwBufSize = 16, bool bNetOrder = true) :
		m_dwCurPos(0)
			,m_bNetOrder(bNetOrder)
			,m_bManaged(true)
			,m_dwMaxSize(dwBufSize)
		{
			m_pBuf = new BYTE [m_dwMaxSize];
			assert(m_pBuf);
			if(m_pBuf)
				memset(m_pBuf, 0, m_dwMaxSize);
		}
		//another one, auto allocate buffer
		//CBtlByteStream(bool bNetOrder = true/*网络字节顺序*/) :
		//	m_dwCurPos(0)
		//	,m_bNetOrder(bNetOrder)
		//	,m_bManaged(true)
		//	,m_dwMaxSize(16)	//initial size, 太小的话没什么意义
		//   {
		//	m_pBuf = new BYTE [m_dwMaxSize];
		//	ATLASSERT(m_pBuf);
		//   }
		//destructor
		virtual ~CBtlByteStream()
		{
			if (m_bManaged && m_pBuf)
				delete [] m_pBuf;
		}

public:    // Operations
	inline DWORD SetCurPos(DWORD dwPos)
	{
		DWORD dwOld = m_dwCurPos;
		m_dwCurPos = dwPos;
		return dwOld;
	}
	inline DWORD GetCurPos()
	{
		return m_dwCurPos;
	}
	inline void Skip(long pos)
	{
		m_dwCurPos += pos;
	}
	inline BYTE* GetData()
	{
		return m_pBuf;
	}

	inline void ReadBytes(LPBYTE pData, DWORD dwSize)
	{
		if (dwSize <= 0 || pData == NULL)
			return;

		memcpy(pData, m_pBuf+m_dwCurPos, dwSize);
		m_dwCurPos += dwSize;
	}
	inline void WriteBytes(LPBYTE pData, DWORD dwSize)
	{
		if (dwSize <= 0 || pData == NULL)
			return;

		memcpy(m_pBuf+m_dwCurPos, pData, dwSize);
		m_dwCurPos += dwSize;
	}

	inline LPCSTR ReadString()
	{
		LPCSTR pStr = (LPCSTR)(m_pBuf+m_dwCurPos);
		m_dwCurPos += (DWORD)strlen(pStr)+1;
		return pStr;
	}

	inline void ReadString(LPSTR pStr)
	{
		strcpy_s(pStr, strlen(ReadString()), ReadString());
	}

	inline void WriteString(LPCSTR pStr)
	{
		if(pStr)
		{
			strcpy_s((char*)(m_pBuf+m_dwCurPos), strlen(pStr), pStr);
			m_dwCurPos += (DWORD)strlen(pStr)+1;
		}
		else
		{
			*(m_pBuf+m_dwCurPos) = '\0';
			m_dwCurPos++;
		}
	}

	inline CBtlByteStream& operator << (unsigned char ch)
	{ m_pBuf[m_dwCurPos++] = ch; return *this; }
	inline CBtlByteStream& operator << (char ch)
	{ m_pBuf[m_dwCurPos++] = (unsigned char)ch; return *this; }
	inline CBtlByteStream& operator << (short s)
	{
		if(m_bNetOrder)
			s = htons(s); 
		WriteBytes((BYTE*)&s, (DWORD)sizeof(short)); 
		return *this; 
	}
	inline CBtlByteStream& operator << (unsigned short s)
	{
		if(m_bNetOrder)
			s = htons(s);
		WriteBytes((BYTE*)&s, (DWORD)sizeof(unsigned short));
		return *this;
	}
	inline CBtlByteStream& operator << (int i)
	{
		if(m_bNetOrder)
			i = htonl(i);
		WriteBytes((BYTE*)&i, (DWORD)sizeof(int));
		return *this;
	}
	inline CBtlByteStream& operator << (unsigned int i)
	{
		if(m_bNetOrder)
			i = htonl(i);
		WriteBytes((BYTE*)&i, (DWORD)sizeof(unsigned int));
		return *this;
	}

	inline CBtlByteStream& operator << (long l)
	{
		if(m_bNetOrder)
			l = htonl(l);
		WriteBytes((BYTE*)&l, (DWORD)sizeof(long));
		return *this; 
	}
	inline CBtlByteStream& operator << (unsigned long l)
	{
		if(m_bNetOrder)
			l = htonl(l);
		WriteBytes((BYTE*)&l, (DWORD)sizeof(unsigned long));
		return *this;
	}
	inline CBtlByteStream& operator << (unsigned __int64 ull)
	{
		if(m_bNetOrder)
			ull=htonll(ull);
		WriteBytes((BYTE*)&ull, (DWORD)sizeof(unsigned __int64));
		return *this;
	}
	inline CBtlByteStream& operator << (float f)
	{
		if(m_bNetOrder)
			Swap(&f, sizeof(float));
		WriteBytes((BYTE*)&f, (DWORD)sizeof(float));
		return *this;
	}
	inline CBtlByteStream& operator << (double d)
	{
		if(m_bNetOrder)
			Swap(&d, sizeof(double));
		WriteBytes((BYTE*)&d, (DWORD)sizeof(double));
		return *this;
	}

	inline CBtlByteStream& operator >> (unsigned char& ch)
	{ ch = m_pBuf[m_dwCurPos++]; return *this; }
	inline CBtlByteStream& operator >> (char& ch)
	{ ch = (char)m_pBuf[m_dwCurPos++]; return *this; }
	inline CBtlByteStream& operator >> (short& s)
	{
		ReadBytes((BYTE*)&s, (DWORD)sizeof(short));
		if(m_bNetOrder)
			s = ntohs(s);
		return *this;
	}
	inline CBtlByteStream& operator >> (unsigned short& s)
	{
		ReadBytes((BYTE*)&s, (DWORD)sizeof(unsigned short));
		if(m_bNetOrder)
			s = ntohs(s);
		return *this;
	}
	inline CBtlByteStream& operator >> (int& i)
	{ 
		ReadBytes((BYTE*)&i, (DWORD)sizeof(int));
		if(m_bNetOrder)
			i = ntohl(i);
		return *this;
	}
	inline CBtlByteStream& operator >> (unsigned int& i)
	{
		ReadBytes((BYTE*)&i, (DWORD)sizeof(unsigned int));
		if(m_bNetOrder)
			i = ntohl(i);
		return *this;
	}

	inline CBtlByteStream& operator >> (long& l)
	{
		ReadBytes((BYTE*)&l, (DWORD)sizeof(long));
		if(m_bNetOrder)
			l = ntohl(l);
		return *this;
	}
	inline CBtlByteStream& operator >> (unsigned long& l)
	{
		ReadBytes((BYTE*)&l, (DWORD)sizeof(unsigned long));
		if(m_bNetOrder)
			l = ntohl(l);
		return *this;
	}
	inline CBtlByteStream& operator >> (unsigned __int64& ull)
	{
		ReadBytes((BYTE*)&ull, (DWORD)sizeof(unsigned __int64));
		if(m_bNetOrder)
			ull=ntohll(ull);
		return *this;
	}
	inline CBtlByteStream& operator >> (float& f)
	{
		ReadBytes((BYTE*)&f, (DWORD)sizeof(float));
		if(m_bNetOrder)
			Swap(&f, sizeof(f));
		return *this;
	}
	inline CBtlByteStream& operator >> (double& d)
	{
		ReadBytes((BYTE*)&d, (DWORD)sizeof(double));
		if(m_bNetOrder)
			Swap(&d, sizeof(double));
		return *this;
	}

	inline void Swap(void* pData, int nSize)
	{

#ifdef __SPARC__
		if (!pData || nSize <= 0)
			return; 
		BYTE* pHead = (BYTE*)pData;
		BYTE* pTail = (BYTE*)pData+nSize-1;
		BYTE byTmp;
		while (pTail - pHead >= 2)
		{
			byTmp = *pHead;
			*pHead = *pTail;
			*pTail = byTmp;
			pTail--;
			pHead++;
		}
#endif  // __SPARC__
#ifdef __POWER_PC__
		if (!pData || nSize <= 0)
			return; 
		BYTE* pHead = (BYTE*)pData;
		BYTE* pTail = (BYTE*)pData+nSize-1;
		BYTE byTmp;
		while (pTail - pHead >= 2)
		{
			byTmp = *pHead;
			*pHead = *pTail;
			*pTail = byTmp;
			pTail--;
			pHead++;
		}
#endif // __POWER_PC__
	}

protected:
	BYTE* m_pBuf;
	DWORD m_dwCurPos;
	DWORD m_dwMaxSize;
	bool m_bNetOrder;
	bool m_bManaged;
	bool m_bAutoReleaseBuf;
};


//} // End namespace BTL
