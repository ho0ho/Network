#include <WinSock2.h>
#include <process.h>
#include <stdio.h>

#define BUFSIZE 1024

struct SOCKETINFO {
	WSAOVERLAPPED overlapped;
	WSABUF wsabuf;		// WSASend()가 buf내용 받을때 WSABUF타입으로 받기때문에 필요함
						// wsabuf.buf는 char *타입으로, 내용에 대한 주소를 받을 수 있을 뿐, 실제 내용을 바로 받지는 못함
	SOCKET sock;
	char buf[BUFSIZE];	// 이 buf변수는 char[]형이라서 실제 데이터를 바로 받을 수 있음
	int recvbytes;
	int sendbytes;
};

SOCKET listenSock;

void DisplayMessage() {
	LPVOID pMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& pMsg, 0, NULL);
	printf("%s\n", pMsg);
	LocalFree(pMsg);
}

unsigned int WINAPI WorkerThread(void* pParam) {
	HANDLE hcp = (HANDLE)pParam;
	while (1) {
		DWORD cbTransferred;
		SOCKET client_sock;
		SOCKETINFO* ptr; 
		int retval = GetQueuedCompletionStatus(hcp, &cbTransferred, (LPDWORD)& client_sock, (LPOVERLAPPED*)& ptr, INFINITE);

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(ptr->sock, (SOCKADDR*)& clientaddr, &addrlen);
			   		 	  	  	 
		if (retval == 0 || cbTransferred == 0) {
			if (retval == 0) {
				DWORD temp1, temp2;
				WSAGetOverlappedResult(ptr->sock, &(ptr->overlapped), &temp1, FALSE, &temp2);
				DisplayMessage();
			}
			closesocket(ptr->sock);
			printf("\n[TCP 서버] 클라이언트(%s : %d) 종료 \n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
			delete ptr;
			continue;
		}

		if (ptr->recvbytes == 0) {
			ptr->recvbytes = cbTransferred;
			ptr->sendbytes = 0;
			ptr->buf[ptr->recvbytes] = '\0';
			printf("\n[TCP/%s:%d] %s \n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), ptr->buf);
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
					DisplayMessage();
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
					DisplayMessage();
					return -1;
				}
			}
		}
	}
	return 0;
}

bool CreateListenSocket() {
	listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET) {
		DisplayMessage();
		return FALSE;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(40100);
	int retval = bind(listenSock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return FALSE;
	}

	retval = listen(listenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return FALSE;
	}

	return TRUE;
}

int main() {
	WSADATA wsa;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("윈도우 소켓 초기화 실패!\n");
		return -1;
	}

	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) return - 1;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	HANDLE hThread;
	unsigned int threadID;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
		hThread = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, (void*)hcp, 0, &threadID);
		if (hThread == NULL) return -1;
		CloseHandle(hThread);
	}

	if (!CreateListenSocket()) {
		printf("대기 소켓 생성 실패! \n");
		return -1;
	}

	while (1) {
		int retval;
		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		SOCKET client_sock = accept(listenSock, (SOCKADDR*)& clientaddr, &addrlen);
		if (client_sock == SOCKET_ERROR) {
			DisplayMessage();	
			continue;
		}
		printf("\n[TCP 서버] 클라이언트(%s : %d) 접속", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		HANDLE hResult = CreateIoCompletionPort((HANDLE)client_sock, hcp, (DWORD)client_sock, 0);
		if (hResult == NULL) return -1;

		SOCKETINFO* ptr = new SOCKETINFO;
		if (!ptr) {
			printf("[오류] 메모리가 부족합니다!\n");
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
				DisplayMessage();
			continue;
		}
	}

	WSACleanup();
	return 0;
}