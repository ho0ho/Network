// CListenSocket.cpp: 구현 파일
//

#include "pch.h"
#include "MFCSocket.h"
#include "CListenSocket.h"
#include "CClientSocket.h"


// CListenSocket

CListenSocket::CListenSocket()
{
}

CListenSocket::~CListenSocket()
{
}


// CListenSocket 멤버 함수


void CListenSocket::OnAccept(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	CClientSocket* pClient = new CClientSocket;
	if (Accept(*pClient)) {
		pClient->SetListenSocket(this);
		m_ptrClientSocketList.AddTail(pClient);
	}
	else {
		delete pClient;
		AfxMessageBox(_T("Error: Failed to accept new client!"));
	}

	CAsyncSocket::OnAccept(nErrorCode);
}


void CListenSocket::CloseClientSocket(CSocket* pClient)
{
	// TODO: 여기에 구현 코드 추가.
	POSITION pos;
	pos = m_ptrClientSocketList.Find(pClient);
	if (pos) {
		if (pClient) {
			pClient->ShutDown();
			pClient->Close();
		}
		m_ptrClientSocketList.RemoveAt(pos);
		delete pClient;
	}
}


void CListenSocket::SendChatDataAll(TCHAR* pszMessage)
{
	// TODO: 여기에 구현 코드 추가.
	POSITION pos = m_ptrClientSocketList.GetHeadPosition();
	CClientSocket* pClient = NULL;
	while (pos) {
		pClient = (CClientSocket*)m_ptrClientSocketList.GetNext(pos);	// GetNext()가 pos도 바꿔줌(다음 걸로) pos -> in이자 out param
		if (pClient)
			pClient->Send(pszMessage, lstrlen(pszMessage) * 2);		// *2의 이유? 한글이 2바이트일수 있는 유니코드까지 커버하기 위해
	}
}
