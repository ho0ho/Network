#include <WinSock2.h>
#include <stdio.h>

#define BUFFERSIZE 1024

void DisplayMessage() {
	LPVOID pMsg;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)& pMsg, 0, NULL);
	printf("%s\n", (char*)pMsg);
	LocalFree(pMsg);
}

int recvn(SOCKET s, char* buf, int len, int flags) {
	int received;
	char* ptr = buf;
	int left = len;
	
	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}
	return (len - left);
}

int main() {
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("윈도우 소켓 초기화 실패!\n");
		return -1;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		DisplayMessage();
		return -1;
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(/*"192.168.1.15"*/"127.0.0.1");
	serveraddr.sin_port = htons(8192/*40100*/);

	retval = connect(sock, (SOCKADDR*)& serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) {
		DisplayMessage();
		return -1;
	}

	char buf[BUFFERSIZE];
	int len;
	while (1) {
		ZeroMemory(buf, sizeof(buf));
		printf("[문자열 입력] ");
		if (fgets(buf, BUFFERSIZE, stdin) == NULL) break;
		len = strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0) break;

		retval = send(sock, buf, strlen(buf), 0);
		if (retval == SOCKET_ERROR) {
			DisplayMessage();
			break;
		}
//		printf("[TCP 클라이언트] %d바이트를 전송\n", retval);
		retval = recvn(sock, buf, retval, 0);
		if (retval == SOCKET_ERROR || retval == 0) {
			DisplayMessage();
			break;
		}
		buf[retval] = '\0';
		/*printf("[TCP 클라이언트] %d 바이트를 받음, 데이터: %s\n", retval, buf);*/
		printf("[TCP] %s\n", buf);

	}

	closesocket(sock);

	WSACleanup();
	return 0;
}
