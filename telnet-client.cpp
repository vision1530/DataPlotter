#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "libtelnet.h"

#pragma comment(lib, "ws2_32.lib")

/* -------------------------------------------------
 * TELNET 옵션 설정
 * ------------------------------------------------- */
static const telnet_telopt_t telopts[] = {
	{ TELNET_TELOPT_ECHO,  TELNET_WILL, TELNET_DO },
	{ TELNET_TELOPT_TTYPE, TELNET_WILL, TELNET_DONT },
	{ -1, 0, 0 }
};

/* -------------------------------------------------
 * TELNET 이벤트 핸들러
 * ------------------------------------------------- */
static void telnet_event_handler(telnet_t *telnet,
	telnet_event_t *ev,
	void *user_data)
{
	SOCKET sock = *(SOCKET *)user_data;

	switch (ev->type) {

	case TELNET_EV_SEND:
		/* libtelnet이 처리한 데이터를 실제 소켓으로 전송 */
		send(sock,
			(const char *)ev->data.buffer,
			(int)ev->data.size,
			0);
		break;

	case TELNET_EV_DATA:
		/* 서버에서 받은 순수 텍스트 */
		fwrite(ev->data.buffer, 1, ev->data.size, stdout);
		fflush(stdout);
		break;

	case TELNET_EV_ERROR:
		fprintf(stderr,
			"TELNET error (%d) %s:%d (%s): %s\n",
			ev->error.errcode,
			ev->error.file,
			ev->error.line,
			ev->error.func,
			ev->error.msg);
		break;

	default:
		break;
	}
}

/* ================= MAIN ================= */

int main(void)
{
	WSADATA wsa;
	SOCKET sock;
	struct sockaddr_in server;
	telnet_t *telnet;

	/* ---------- signal parameters ---------- */
	const double fs = 1000.0;   // sampling rate (Hz)
	const double dt = 1.0 / fs;
	const double f1 = 300.0;    // ch1 frequency
	const double f2 = 700.0;    // ch2 frequency
	const double amp = 5.0;

	double t = 0.0;              // time since connection (sec)

	char txbuf[256];

	/* ---------- Winsock init ---------- */
	WSAStartup(MAKEWORD(2, 2), &wsa);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	server.sin_family = AF_INET;
	server.sin_port = htons(23);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	connect(sock, (struct sockaddr *)&server, sizeof(server));

	printf("Connected\n");

	telnet = telnet_init(telopts, telnet_event_handler, 0, &sock);

	/* ---------- streaming loop ---------- */
	while (1) {

		double ch1 = amp * sin(2.0 * M_PI * f1 * t);
		double ch2 = amp * sin(2.0 * M_PI * f2 * t);

		/* Point protocol with explicit time */
		snprintf(txbuf, sizeof(txbuf),
			"$$P%.6f,%.6f,%.6f;\r\n",
			t, ch1, ch2);

		telnet_send(telnet, txbuf, (int)strlen(txbuf));

		t += dt;

		Sleep((DWORD)(dt * 1000.0));
	}

	/* not reached */
	telnet_free(telnet);
	closesocket(sock);
	WSACleanup();

	return 0;
}