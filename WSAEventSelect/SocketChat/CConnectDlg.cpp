// CConnectDlg.cpp: 구현 파일
//

#include "pch.h"
#include "CConnectDlg.h"
#include "afxdialogex.h"
#include "resource.h"


// CConnectDlg 대화 상자

IMPLEMENT_DYNAMIC(CConnectDlg, CDialogEx)

CConnectDlg::CConnectDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CONNECT, pParent)
	, m_ipaddress(_T(""))
{

}

CConnectDlg::~CConnectDlg()
{
}

void CConnectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConnectDlg, CDialogEx)
	ON_BN_CLICKED(ID_BUTTON_CONNECTS, &CConnectDlg::OnBnClickedButtonConnects)
END_MESSAGE_MAP()


// CConnectDlg 메시지 처리기


BOOL CConnectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	SetDlgItemText(IDC_IPADDRESS, _T("127.0.0.1"));

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CConnectDlg::OnBnClickedButtonConnects()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	GetDlgItemText(IDC_IPADDRESS, m_ipaddress);
	CDialogEx::OnOK();
}
