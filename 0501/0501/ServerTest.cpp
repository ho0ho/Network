#include <winsock2.h>
#include <stdio.h>

#define BUFFERSIZE 1024

void DisplayMessage() {
	LPVOID pMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& pMsg, 0, NULL);
	printf("%s\n", pMsg);
	LocalFree(pMsg);
}

int main() {
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("윈도우 소켓 초기화 실패!\n");
		return -1;
	}

	SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET) {
		DisplayMessage();
		return -1;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(40100);
	retval = bind(listenSock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return -1;
	}

	retval = listen(listenSock, SOMAXCONN);
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return -1;
	}
	
	SOCKET clientSock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	char buf[BUFFERSIZE];

	while (1) {
		addrlen = sizeof(clientaddr);
		clientSock = accept(listenSock, (SOCKADDR*)& clientaddr, &addrlen);
		if (clientSock == INVALID_SOCKET) {
			DisplayMessage();
			continue;
		}
		printf("\n[TCP 서버] 클라이언트 접속: IP주소=%s, 포트번호=%d\n", 
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		while (1) {
			retval = recv(clientSock, buf, BUFFERSIZE, 0);
			if (retval == SOCKET_ERROR) {
				DisplayMessage();
				break;
			}
			else if (retval == 0) {
				DisplayMessage();
				break;
			}
			else {
				buf[retval] = '\0';
				printf("\n[TCP 서버] IP주소=%s, 포트번호=%d의 받은 메시지 : %s\n",
					inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), buf);
			}
		}

		closesocket(clientSock);
		printf("\n[TCP 서버] 클라이언트 종료: IP주소=%s, 포트번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
	}

	closesocket(listenSock);

	WSACleanup();
	return 0;
}