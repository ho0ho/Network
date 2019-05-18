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

SOCKET listenSock, clientSock;
HANDLE hEvent;

void DisplayMessage() {
	LPVOID pMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& pMsg, 0, NULL);
	printf("%s\n", pMsg);
	LocalFree(pMsg);
}

void CALLBACK CompletionRoutine(DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags) {
	SOCKETINFO* ptr = (SOCKETINFO*)lpOverlapped;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (SOCKADDR*)& clientaddr, &addrlen);

	if (dwError != 0 || cbTransferred == 0) {
		if (dwError != 0) DisplayMessage();
		closesocket(ptr->sock);
		printf("\n[TCP 서버] 클라이언트(%s : %d) 종료", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
		delete ptr;
		return;
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
		int retval = WSASend(ptr->sock, &(ptr->wsabuf), 1, &sendbytes, 0, &(ptr->overlapped), CompletionRoutine);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				DisplayMessage();
				return;
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
		int retval = WSARecv(ptr->sock, &(ptr->wsabuf), 1, &recvbytes, &flags, &(ptr->overlapped), CompletionRoutine);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				DisplayMessage();
				return;
			}			
		}
	}
}

unsigned int WINAPI WorkerThread(void* pParam) {
	HANDLE hEvent = (HANDLE)pParam;
	while (1) {
		while (1) {
			DWORD result = WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
			if (result == WAIT_OBJECT_0) break;
			if (result != WAIT_IO_COMPLETION) return -1;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(clientSock, (SOCKADDR*)& clientaddr, &addrlen);
		printf("\n[TCP 서버] 클라이언트(%s : %d) 접속", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		SOCKETINFO * ptr = new SOCKETINFO;
		if (ptr == NULL) {
			printf("\n[오류] 메모리가 부족합니다!\n");
			return -1;
		}

		ZeroMemory(&(ptr->overlapped), sizeof(ptr->overlapped));
		ptr->sock = clientSock;
		ptr->recvbytes = ptr->sendbytes = 0;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;

		DWORD recvbytes;
		DWORD flags = 0;
		int retval = WSARecv(ptr->sock, &(ptr->wsabuf), 1, &recvbytes, &flags, &(ptr->overlapped), CompletionRoutine);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				DisplayMessage();
				return -1;
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

	hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (hEvent == NULL) return false;

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
	if (!CreateListenSocket()) {
		printf("대기 소켓 생성 실패! \n");
		return -1;
	}

	unsigned int threadID;
	CloseHandle((HANDLE)_beginthreadex(0, 0, WorkerThread, (void*)hEvent, 0, &threadID));
	while (1) {
		clientSock = accept(listenSock, NULL, NULL);
		if (clientSock == INVALID_SOCKET) {
			DisplayMessage();
			continue;
		}
		if(!SetEvent(hEvent)) break;
	}

	WSACleanup();
	return 0;
}
