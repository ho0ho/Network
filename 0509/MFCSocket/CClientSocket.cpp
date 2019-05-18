// CClientSocket.cpp: 구현 파일
//

#include "pch.h"
#include "MFCSocket.h"
#include "CClientSocket.h"
#include "CListenSocket.h"	
#include "MFCSocketDlg.h"


// CClientSocket

CClientSocket::CClientSocket()
{
}

CClientSocket::~CClientSocket()
{
}


// CClientSocket 멤버 함수


void CClientSocket::SetListenSocket(CAsyncSocket* pListen)
{
	// TODO: 여기에 구현 코드 추가.
	m_pListenSocket = pListen;

}


void CClientSocket::OnClose(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다
	// clinetSocket 측에서 먼저 close를 완료한뒤(Onclose), 
	CSocket::OnClose(nErrorCode);
	// 여기서 서버측 정리를 하기 위해 이렇게 순서 정함
	CListenSocket* pServerSocket = (CListenSocket *)m_pListenSocket;
	pServerSocket->CloseClientSocket(this);
}


void CClientSocket::OnReceive(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	CString strTmp = _T(""), strIpAddress = _T("");
	UINT uPortNumber = 0;
	TCHAR szBuffer[1024];
	::ZeroMemory(szBuffer, sizeof(szBuffer));
		
	GetPeerName(strIpAddress, uPortNumber);
	if (Receive(szBuffer, sizeof(szBuffer)) > 0) {
		CMFCSocketDlg* pMain = (CMFCSocketDlg*)AfxGetMainWnd();
		strTmp.Format(_T("[%s:%d] : %s"), strIpAddress, uPortNumber, szBuffer);
		pMain->m_List.AddString(strTmp);
		pMain->m_List.SetCurSel(pMain->m_List.GetCount() - 1);

		CListenSocket* pServerSocket = (CListenSocket*)m_pListenSocket;
		pServerSocket->SendChatDataAll(szBuffer);
	}

	CSocket::OnReceive(nErrorCode);
}
