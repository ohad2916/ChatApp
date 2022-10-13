
#define BUFFER_SIZE 512

#include "ServerHandle.h"

#define SERVER_IP L"192.168.0.162"
#define SERVER_PORT 5000
#define BUFFER_SIZE 512


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