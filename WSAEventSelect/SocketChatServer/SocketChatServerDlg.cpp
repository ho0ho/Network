
// SocketChatServerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "SocketChatServer.h"
#include "SocketChatServerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSocketChatServerDlg 대화 상자



CSocketChatServerDlg::CSocketChatServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SOCKETCHATSERVER_DIALOG, pParent)
	, m_portNum(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSocketChatServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PORT, m_portNum);
	DDX_Control(pDX, IDC_LIST1, m_loglist);
	DDX_Control(pDX, IDC_LIST2, m_clientList);
}

BEGIN_MESSAGE_MAP(CSocketChatServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_BUTTON_START, &CSocketChatServerDlg::OnBnClickedButtonStart)
END_MESSAGE_MAP()


// CSocketChatServerDlg 메시지 처리기

BOOL CSocketChatServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		m_loglist.InsertString(-1, _T("윈도우 소켓 초기화 실패!\n"));
		return FALSE;
	}

	hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (!hcp) return FALSE;

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	CWinThread *th;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
		th = AfxBeginThread(WorkerThread, this);
		if (!th) return FALSE;
		/*CloseHandle(th);*/
	}
	
	   
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CSocketChatServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CSocketChatServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CSocketChatServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CSocketChatServerDlg::OnBnClickedButtonStart()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	UpdateData(TRUE); 
	if (!CreateListenSocket()) {
		SetLogList(_T("대기 소켓 생성 실패!\n"));
		return ;
	}

	SetLogList(_T("대기 소켓 생성 완료!"));
	CWinThread* listenTh = AfxBeginThread(ListenThread, this);
	if (!listenTh) return;

	UpdateData(FALSE);
}


void CSocketChatServerDlg::DisplayMessage()
{
	// TODO: 여기에 구현 코드 추가.
	LPVOID pMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& pMsg, 0, NULL);
	/*printf("%s\n", pMsg);*/
	m_loglist.InsertString(-1, (LPTSTR)pMsg);
	LocalFree(pMsg);
}

void CSocketChatServerDlg::SetLogList(LPTSTR logMsg)
{
	// TODO: 여기에 구현 코드 추가.
	/*int index = m_loglist.InsertString(-1, logMsg);
	m_loglist.SetCurSel(index);*/
 	m_loglist.AddString(logMsg);
}


UINT WorkerThread(LPVOID pParam) {
	CSocketChatServerDlg *dlg = (CSocketChatServerDlg *)pParam;
	/*HANDLE hcp = (HANDLE)pParam;*/
	while (1) {
		DWORD cbTransferred;
		SOCKET client_sock;
		SOCKETINFO* ptr;
		int retval = GetQueuedCompletionStatus(dlg->hcp, &cbTransferred, (LPDWORD)& client_sock, (LPOVERLAPPED*)& ptr, INFINITE);

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)& clientaddr, &addrlen);

		TCHAR buf[100];
		/*char buf[100];*/
		WCHAR uni[100];
		if (retval == 0 || cbTransferred == 0) {
			if (retval == 0) {
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &(ptr->overlapped), &temp1, FALSE, &temp2);
				dlg->DisplayMessage();				
			}
			closesocket(ptr->sock);
			/*printf("\n[TCP 서버] 클라이언트(%s : %d) 종료 \n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));*/
			int unilen = MultiByteToWideChar(CP_ACP, 0, inet_ntoa(clientaddr.sin_addr), strlen(inet_ntoa(clientaddr.sin_addr)), NULL, NULL);
			MultiByteToWideChar(CP_ACP, 0, inet_ntoa(clientaddr.sin_addr), strlen(inet_ntoa(clientaddr.sin_addr)), uni, unilen);
			uni[unilen] = '\0';
			wsprintf(buf, _T("[TCP 서버] 클라이언트(%s : %d) 종료"), uni, ntohs(clientaddr.sin_port));
			dlg->SetLogList(buf);
			
			delete ptr;
			continue;
		}

		if (ptr->recvbytes == 0) {
			ptr->recvbytes = cbTransferred;
			ptr->sendbytes = 0;		
		}
		else
			ptr->sendbytes += cbTransferred;

		if (ptr->recvbytes > ptr->sendbytes) {
			ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));
			ptr->wsabuf.buf = ptr->buf + ptr->sendbytes;
			ptr->wsabuf.len = ptr->recvbytes - ptr->sendbytes;
			DWORD sendbytes;
			

			int retval = WSASend(ptr->sock, &(ptr->wsabuf), 1, &sendbytes, 0, &(ptr->overlapped), NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					dlg->DisplayMessage();
					return -1;
				}
			}
		}
		else {
			ptr->recvbytes = 0;
			ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));
			ptr->wsabuf.buf = ptr->buf;
			ptr->wsabuf.len = BUFSIZE;

			DWORD recvbytes;
			DWORD flags = 0;
			int retval = WSARecv(ptr->sock, &(ptr->wsabuf), 1, &recvbytes, &flags, &(ptr->overlapped), NULL);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSA_IO_PENDING) {
					dlg->DisplayMessage();
					return -1;
				}
			}
		}
	}

	return UINT();
}


BOOL CSocketChatServerDlg::CreateListenSocket()
{
	// TODO: 여기에 구현 코드 추가.
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET) {
		DisplayMessage();
		return false;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(m_portNum);
	int retval = bind(listenSock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		AfxMessageBox(_T("Failed to bind listensock"));
		return FALSE;
	}

	retval = listen(listenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		AfxMessageBox(_T("Failed to listen listensock"));
		return FALSE;
	}

	return TRUE;
}

UINT ListenThread(LPVOID pParam) {
	CSocketChatServerDlg *test = (CSocketChatServerDlg *)pParam;
	while (1) {
		TCHAR buf[100] = { 0, };
		TCHAR uni[100] = { 0, };
		int retval;
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		SOCKET client_sock = accept(test->listenSock, (SOCKADDR*)& clientaddr, &addrlen);
		if (client_sock == SOCKET_ERROR) {
			test->DisplayMessage();
			continue;
		}

		test->m_ptrClientSocketList.AddTail(&client_sock);
		/*printf("\n[TCP 서버] 클라이언트(%s : %d) 접속", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));*/
		int unilen = MultiByteToWideChar(CP_ACP, 0, inet_ntoa(clientaddr.sin_addr), strlen(inet_ntoa(clientaddr.sin_addr)), NULL, NULL);
		MultiByteToWideChar(CP_ACP, 0, inet_ntoa(clientaddr.sin_addr), strlen(inet_ntoa(clientaddr.sin_addr)), uni, unilen);
		uni[unilen] = '\0';
		//sprintf(buf, "[TCP 서버] 클라이언트(%s : %d) 종료", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		wsprintf(buf, _T("[TCP 서버] 클라이언트(%s : %d) 접속"), uni, ntohs(clientaddr.sin_port));
		test->SetLogList(buf);

		HANDLE hResult = CreateIoCompletionPort((HANDLE)client_sock, test->hcp, (DWORD)client_sock, 0);
		if (hResult == NULL) break;

		SOCKETINFO * ptr = new SOCKETINFO;
		if (!ptr) {
			/*printf("[오류] 메모리가 부족합니다!\n");*/
			test->SetLogList(_T("[오류] 메모리가 부족합니다!"));
			break;
		}
		ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->recvbytes = ptr->sendbytes = 0;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;

		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(client_sock, &(ptr->wsabuf), 1, &recvbytes, &flags, &(ptr->overlapped), NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING)
				test->DisplayMessage();
			continue;
		}
	}

	return 0;
}
