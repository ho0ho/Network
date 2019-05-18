// CConnectSocket.cpp: 구현 파일
//

#include "pch.h"
#include "MFCSocket_2.h"
#include "CConnectSocket.h"
#include "MFCSocket_2Dlg.h"


// CConnectSocket

CConnectSocket::CConnectSocket()
{
}

CConnectSocket::~CConnectSocket()
{
}


// CConnectSocket 멤버 함수


void CConnectSocket::OnClose(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	ShutDown();
	Close();
	CSocket::OnClose(nErrorCode);

	AfxMessageBox(_T("ERROR: Disconnected fron server!"));
	::PostQuitMessage(0);
}


void CConnectSocket::OnReceive(int nErrorCode)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	TCHAR szBuffer[1024];
	::ZeroMemory(szBuffer, sizeof(szBuffer));
	if (Receive(szBuffer, sizeof(szBuffer)) > 0) {
		CMFCSocket2Dlg* pMain = (CMFCSocket2Dlg*)AfxGetMainWnd();
		pMain->m_List.AddString(szBuffer);
		pMain->m_List.SetCurSel(pMain->m_List.GetCount() - 1);
	}

	CSocket::OnReceive(nErrorCode);
}
