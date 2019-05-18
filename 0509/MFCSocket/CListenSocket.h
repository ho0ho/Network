#pragma once

// CListenSocket 명령 대상

class CListenSocket : public CAsyncSocket
{
public:
	CListenSocket();
	virtual ~CListenSocket();

	CPtrList m_ptrClientSocketList;
	// CPtrList: 객체와 구조체를 가리킬 수 있는 포인터들을 담을 수 있는 컨테이너(리스트로 구현되어있음)
	// 멀티 클라이언트에 대한 각각의 클라이언트 소켓들을 관리하기 위해 사용
	virtual void OnAccept(int nErrorCode);
	void SendChatDataAll(TCHAR* pszMessage);
	void CloseClientSocket(CSocket* pClient);
};


