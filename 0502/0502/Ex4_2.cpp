#include <WinSock2.h>
#include <process.h>
#include <stdio.h>

#define BUFFERSIZE 1024

struct SOCKETINFO {
	SOCKET sock;
	char buf[BUFFERSIZE];
	int recvbyte;
	int sendbyte;
};

SOCKET listenSock;
int nTotalSockets = 0;
SOCKETINFO* SocketInfoArray[FD_SETSIZE];

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

	u_long on = 1;
	retval = ioctlsocket(listenSock, FIONBIO, &on);
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

unsigned int WINAPI WorkerThread(void* pParam) {
	int retval;
	FD_SET rset, wset;
	SOCKET clientSock;
	SOCKADDR_IN clientaddr;
	int addrlen;

	while (1) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(listenSock, &rset);
		for (int i = 0; i < nTotalSockets; i++) {
			if (SocketInfoArray[i]->recvbyte > SocketInfoArray[i]->sendbyte)
				FD_SET(SocketInfoArray[i]->sock, &wset);
			else
				FD_SET(SocketInfoArray[i]->sock, &rset);
		}

		retval = select(0, &rset, &wset, NULL, NULL);
		if (retval == SOCKET_ERROR) {
			DisplayMessage();
			break;
		}

		if (FD_ISSET(listenSock, &rset)) {
			addrlen = sizeof(clientaddr);
			clientSock = accept(listenSock, (SOCKADDR*)&clientaddr, &addrlen);
			if (clientSock == INVALID_SOCKET) {
				if (WSAGetLastError() != WSAEWOULDBLOCK)
					DisplayMessage();
			}
			else {
				printf("\n[TCP 서버] 클라이언트 접속: IP 주소 = %s, Port 번호 = %d\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
				if (AddSocketInfo(clientSock) == FALSE) {
					printf("[TCP 서버] 클라이언트 접속을 해제합니다!\n");
					closesocket(clientSock);
				}
			}
		}

		for (int i = 0; i < nTotalSockets; i++) {
			SOCKETINFO* ptr = SocketInfoArray[i];
			if (FD_ISSET(ptr->sock, &rset)) {
				retval = recv(ptr->sock, ptr->buf, BUFFERSIZE, 0);
				if (retval == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						DisplayMessage();
						RemoveSocketInfo(i);
					}
					continue;
				}
				else if (retval == 0) {
					RemoveSocketInfo(i);
					continue;
				}
				ptr->recvbyte = retval;
				addrlen = sizeof(clientaddr);
				getpeername(ptr->sock, (SOCKADDR*)& clientaddr, &addrlen);
				ptr->buf[retval] = '\0';
				printf("[TCP 서버] IP 주소 = %s, Port 번호 = %d의 메시지: %s\n", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), ptr->buf);
			}
			if (FD_ISSET(ptr->sock, &wset)) {
				retval = send(ptr->sock, ptr->buf + ptr->sendbyte, ptr->recvbyte - ptr->sendbyte, 0);
				if (retval == SOCKET_ERROR) {
					if (WSAGetLastError() != WSAEWOULDBLOCK) {
						DisplayMessage();
						RemoveSocketInfo(i);
					}
					continue;
				}
				ptr->sendbyte += retval;
				if (ptr->recvbyte == ptr->sendbyte)
					ptr->recvbyte = ptr->sendbyte = 0;
			}
		}
	}
	closesocket(listenSock);
	return 0;
}

int main() {
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("윈도우 소켓 초기화 실패!\n");
		return -1;
	}

	if (!CreateListenSocket()) {
		printf("대기 소켓 생성 실패!\n");
		return -1;
	}

	unsigned int threadID;
	WaitForSingleObject((HANDLE)_beginthreadex(0, 0, WorkerThread, 0, 0, &threadID), INFINITE);
	
	WSACleanup();
	return 0;
}
