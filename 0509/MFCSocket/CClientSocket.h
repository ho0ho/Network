#pragma once

// CClientSocket 명령 대상

class CClientSocket : public CSocket
{
public:
	CClientSocket();
	virtual ~CClientSocket();
	CAsyncSocket* m_pListenSocket;
	void SetListenSocket(CAsyncSocket* pListen);
	virtual void OnClose(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
};


