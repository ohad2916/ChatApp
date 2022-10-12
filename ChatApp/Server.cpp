#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include "ServerHandle.h"
#include <chrono>
#pragma comment (lib,"ws2_32.lib")
#define SERVER_IP L"192.168.0.162"
#define SERVER_PORT 5000


int Client::default_count{ 0 };
int main() {
	serverHandle server;
	server.start(SERVER_IP,SERVER_PORT);
	std::thread ListeningThread(&serverHandle::HandleListening, &server);
	//std::thread HandleStateThread = std::thread(&serverHandle::RefreshClientState, &server);
	server.handleServerInput();
	ListeningThread.join();
	return 0;
}	