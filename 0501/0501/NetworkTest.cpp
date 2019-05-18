#include <WinSock2.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("윈도우 소켓 초기화 실패!\n");
		return -1;
	}
	/*printf("윈도우 소켓 초기화 성공!\n"); */

	//const char *ipaddr = "230.200.12.5";
	//printf("IP 문자열 주소: %s\n", ipaddr);
	//printf("IP 문자열 주소 => 정수 = 0x%08X\n", inet_addr(ipaddr));

	//IN_ADDR in_addr;
	//in_addr.s_addr = inet_addr(ipaddr);
	//printf("IP 정수 => 문자열 주소 = %s\n", inet_ntoa(in_addr));

	unsigned short us = 0x1234;
	unsigned long ul = 0x12345678;

	printf("0x%08X => 0x%08X\n", us, htons(us));
	printf("0x%08X => 0x%08X\n", ul, htonl(ul));

	unsigned short n_us = htons(us);
	unsigned long n_ul = htonl(ul);

	printf("0x%08X => 0x%08X\n", n_us, ntohs(n_us));
	printf("0x%08X => 0x%08X\n", n_ul, ntohl(n_ul));

	WSACleanup(); 
	return 0;
}
