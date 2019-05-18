#include <WinSock2.h>
#include <process.h>
#include <stdio.h>

#pragma warning(disable : 4996)

#define BUFFERSIZE 1024
#define WM_SOCKET (WM_USER + 1)

struct SOCKETINFO {
	int id;
	SOCKET sock;
	char buf[BUFFERSIZE];
	int recvbyte;
	int sendbyte;
	BOOL recvdelayed;
};

HWND hWnd;
SOCKET listenSock;
static int common_id = 0;
int nTotalSockets = 0;
SOCKETINFO* SocketInfoArray[FD_SETSIZE];

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void DisplayMessage() {
	LPVOID pMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& pMsg, 0, NULL);
	printf("%s\n", (char*)pMsg);
	LocalFree(pMsg);
}

bool CreateListenSocket() {
	int retval;
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET) {
		DisplayMessage();
		return false;
	}

	retval = WSAAsyncSelect(listenSock, hWnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE);
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return false;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(40100);

	retval = bind(listenSock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return false;
	}

	retval = listen(listenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return false;
	}

	return true;
}

BOOL AddSocketInfo(SOCKET sock) {
	if (nTotalSockets >= (FD_SETSIZE - 1)) {
		printf("[오류] 소켓정보를 추가할 수 없습니다~!\n");
		return FALSE;
	}

	SOCKETINFO* ptr = new SOCKETINFO;
	if (ptr == NULL) {
		printf("[오류] 메모리가 부족합니다!\n");
		return FALSE;
	}

	ptr->id = common_id++;
	ptr->sock = sock;
	ptr->recvbyte = 0;
	ptr->sendbyte = 0;
	SocketInfoArray[nTotalSockets++] = ptr;

	return TRUE;
}

void RemoveSocketInfo(int nIndex) {
	SOCKETINFO* ptr = SocketInfoArray[nIndex];
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR*)& clientaddr, &addrlen);
	printf("[TCP 서버] 클라이언트 종료: IP 주소 = %s, Port 번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	closesocket(ptr->sock);
	delete ptr;
	for (int i = nIndex; i < nTotalSockets; i++)
		SocketInfoArray[i] = SocketInfoArray[i + 1];
	nTotalSockets--;
}

int GetSocketInfo(SOCKET sock) {
	for (int i = 0; i < nTotalSockets; i++)
		if (SocketInfoArray[i]->sock == sock)
			return i;
	return -1;
}

bool CreateWndclass(HINSTANCE hInst) {
	WNDCLASS wndclass;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hInstance = hInst;
	wndclass.lpfnWndProc = (WNDPROC)WndProc;
	wndclass.lpszClassName = "MyWindowClass";
	wndclass.lpszMenuName = NULL;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	if (!RegisterClass(&wndclass)) return false;
	return true;
}

int main() {
	if (!CreateWndclass(NULL)) return -1;
	hWnd = CreateWindow("MywindowClass", "TCP서버", WS_OVERLAPPEDWINDOW, 0, 0, 600, 300, NULL, (HMENU)NULL, NULL, NULL);
	if (!hWnd) return -1;
	ShowWindow(hWnd, SW_SHOWNORMAL);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return -1;
	if (!CreateListenSocket()) {
		printf("대기소켓 생성 실패!\n");
		return -1;
	}

	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WSACleanup();
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CREATE:
		break;
	case WM_SOCKET:
	{
		SOCKETINFO* ptr;
		SOCKET client_sock;
		SOCKADDR_IN clientaddr;
		int addrlen;
		int retval;

		if (WSAGETSELECTERROR(lParam)) {
			RemoveSocketInfo(GetSocketInfo(wParam));
			break;		
		}
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_ACCEPT:
		{
			addrlen = sizeof(clientaddr);
			client_sock = accept(wParam, (SOCKADDR*)& clientaddr, &addrlen);

			if (client_sock == INVALID_SOCKET) {
				if (WSAGetLastError() != WSAEWOULDBLOCK)
					DisplayMessage();
				break;
			}
			printf("[TCP 서버] 클라이언트 접속: IP 주소 = %s, Port 번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			AddSocketInfo(client_sock);
			retval = WSAAsyncSelect(client_sock, hWnd, WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE);
			if (retval == SOCKET_ERROR) {
				DisplayMessage();
				RemoveSocketInfo(GetSocketInfo(client_sock));
			}

			// id 뿌려주기
			int rv;
			SOCKETINFO* p;
			for (int i = 0; i < nTotalSockets; i++) {
				SOCKETINFO* listener = SocketInfoArray[i];
				p = SocketInfoArray[nTotalSockets - 1];
				char msg[2];
				_itoa(p->id, msg, 10);			
				/*strcat(msg, "\0");*/
				rv = send(listener->sock, msg, sizeof(msg), 0);		
				/*send(listener->sock, inet_ntohl(client_sock.addr), size, 0)*/
			}
		}
		break;
		case FD_READ:
			ptr = SocketInfoArray[GetSocketInfo(wParam)];
			if (ptr->recvbyte > 0) {
				ptr->recvdelayed = TRUE;
				break;
			}
			retval = recv(ptr->sock, ptr->buf, BUFFERSIZE, 0);
			if (retval == SOCKET_ERROR) {
				if (WSAGetLastError() != WSAEWOULDBLOCK) {
					DisplayMessage();
					RemoveSocketInfo(GetSocketInfo(wParam));
				}
				break;
			}
			ptr->recvbyte = retval;
			ptr->buf[retval] = '\0';
			addrlen = sizeof(clientaddr);
			getpeername(wParam, (SOCKADDR*)& clientaddr, &addrlen);
			printf("[TCP%s:%d] %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), ptr->buf);
		case FD_WRITE:
		{
			ptr = SocketInfoArray[GetSocketInfo(wParam)];
			if (ptr->recvbyte <= ptr->sendbyte) break;

			/*retval = send(ptr->sock, ptr->buf + ptr->sendbyte, ptr->recvbyte - ptr->sendbyte, 0);*/
			for (int i = 0; i < nTotalSockets; i++) {
				SOCKETINFO* test = SocketInfoArray[i];
				retval = send(test->sock, ptr->buf + ptr->sendbyte, ptr->recvbyte - ptr->sendbyte, 0);
				if (retval == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						DisplayMessage();
						RemoveSocketInfo(wParam);
					}
					break;
				}

			}

			ptr->sendbyte += retval;
			if (ptr->recvbyte == ptr->sendbyte) {
				ptr->recvbyte = ptr->sendbyte = 0;
				if (ptr->recvdelayed) {
					ptr->recvdelayed = FALSE;
					PostMessage(hWnd, WM_SOCKET, wParam, FD_READ);
				}
			}
		}
		break;
		case FD_CLOSE:
			RemoveSocketInfo(GetSocketInfo(wParam));
			break;
		}
	}
	return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}