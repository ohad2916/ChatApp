#pragma once
#include <vector>
#include <climits>
#include <string>
#include <bitset>
#include "Client.h"
#include "Message.h"
#include <cstring>
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <atomic>
#include <list>
#include <set>
#pragma comment (lib,"ws2_32.lib")

#define BUFFER_SIZE 256

class serverHandle {
private:
	std::unordered_map<std::string, int> serverCommands;
	std::list<Client> clientList{};
	std::mutex clientList_Lock;
	std::mutex client_Lock;
	SOCKET listn_sock = INVALID_SOCKET;
	std::thread* pThread = nullptr;
	std::string server_input{};
	std::atomic<bool> stopCalled = false;
	std::unordered_map<std::string, Client*> namesTable;

public:
	
	//default constructor
	serverHandle(){
		serverCommands["/postinfo"] = 0;
		serverCommands["/stop"] = 1;
		serverCommands["/say"] = 2;
		serverCommands["/kick"] = 3;
	}
	//used to start a server
	void start(const wchar_t* address,short port) {
		// Initilize WinSock API
		WSADATA wsaData;
		int err;
		WORD ver = MAKEWORD(2, 2);
		if (WSAStartup(ver, &wsaData) != 0)
		{
			std::cerr << "Winsock dll not found!" << std::endl;
			return;
		}
		else
			std::cerr << "Found, Status:" << wsaData.szSystemStatus << std::endl;

		//Create a listening socket
		listn_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (listn_sock == INVALID_SOCKET) {
			std::cerr << "Failed creating a listening socket!" << WSAGetLastError() << std::endl;
			WSACleanup();
			return;
		}
		else
			std::cerr << "listening Socket OK!" << std::endl;
		//Bind the socket
		sockaddr_in service;
		service.sin_family = AF_INET;
		InetPton(AF_INET, address, &service.sin_addr.S_un.S_addr);
		service.sin_port = htons(5000);
		if (bind(listn_sock, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
			std::cerr << "Error binding!" << std::endl;
			return;
		}
		else
			std::cerr << "Bound succesfully!" << std::endl;
		if (listen(listn_sock, SOMAXCONN) == SOCKET_ERROR) {
			std::cerr << "Error listening!" << WSAGetLastError() << std::endl;
			return;
		}
	}
	//shutdown and cleanup
	void shutdown() {
		std::cout << "STOP called, shutting down in 3 seconds!" << std::endl;
		clientList_Lock.lock();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		for (Client& client : clientList) {
			if (client._state != Client::disconnected) {
				closesocket(client._connection);
				client._state = Client::disconnected;
			}
		}
		clientList_Lock.unlock();
		stopCalled = true;
		WSACleanup();
		std::this_thread::sleep_for(std::chrono::seconds(3));
		return;
	}
	//send a message to a specific client
	void sendMsg(Message& msg, Client& client) {
		send(client._connection, msg.getData(), msg._size, NULL);
	}

	//send a message to every client
	void sendMsgtoAll(const char* buffer, size_t len) {
		clientList_Lock.lock();
		for (Client& client : clientList) {
			send(client._connection, buffer, len, NULL);
		}
		clientList_Lock.unlock();

	}
	void sendMsgtoAll(Message& msg) {
		clientList_Lock.lock();
		for (Client& client : clientList) {
			send(client._connection, msg.getData(), msg._size, NULL);
		}
		clientList_Lock.unlock();
	}
	//handle listening socket, meant to run on secondery thread.
	void HandleListening() {
		std::cout << "Listening on thread:" << std::this_thread::get_id() << std::endl;
		while (!stopCalled) {
			//handle accepting new clients
			SOCKET acceptSocket = INVALID_SOCKET;
			sockaddr_in accept_addr;
			int accept_addr_size = sizeof(accept_addr);
			acceptSocket = accept(listn_sock, (sockaddr*)&accept_addr, &accept_addr_size);
			if (acceptSocket == INVALID_SOCKET) {
				if(!stopCalled)
					std::cerr << "Invalid connection attempted!" << std::endl;
			}
			else {
				clientList_Lock.lock();
				Client& new_client = clientList.emplace_front(Client(acceptSocket,accept_addr));
				std::cerr << "New client joined from: "<< new_client.ipv4_addr << std::endl;
				namesTable[new_client._name] = &new_client;
				//std::cerr << "strarting posting thread! for client:" << acceptSocket << std::endl;
				std::thread recvThread(&serverHandle::RecvFromClient, this, std::ref(new_client));
				new_client._state = Client::connected_thread_on;
				recvThread.detach();
				//std::cerr << "Client List size is:" << clientList.size() << std::endl;
				clientList_Lock.unlock();
			}
		}
	}
	//recieve from a specific client and post to screen
	void RecvFromClient(Client& client) {
		char recvBuffer[BUFFER_SIZE];
		while (client._state != Client::disconnected && !stopCalled){
			std::memset(recvBuffer, 0, BUFFER_SIZE);
			int byteCount = recv(client._connection, recvBuffer, 256, NULL);
			if (byteCount == SOCKET_ERROR) {
				std::cerr << client._name << " disconnected!" << std::endl;
				closesocket(client._connection);
				client._state = Client::disconnected;
				//return;
			}
			else if (byteCount == 2) {
				Message msg(Message::text, "\n\r");
				sendMsgtoAll(msg);
			}
			else {
				if (recvBuffer[0] == Message::namechange)
				{	
					recvBuffer[13] = NULL; //limit name length
					std::string entered_name(recvBuffer + 1);
					if (!namesTable.contains(entered_name)) {
						namesTable.erase(std::move(client._name));
						client._name = std::move(entered_name);
						namesTable[client._name] = &client;
					}
					else {
						Message error_msg(Message::text, std::format("[Server(Private)]:{} is already taken!", entered_name));
						sendMsg(error_msg, client);
					}
				}
				else {
						Message msg(Message::text, std::format("[{}]: {}", client._name, recvBuffer + 1));
						sendMsgtoAll(msg);
						std::cout << msg.getData() << std::endl;
					}
			}
		}
	}
	//void RefreshClientState() {
	//	while (!stopCalled) 
	//	{	
	//		for(Client& client : clientList){
	//			switch (client._state) {
	//				case 0:
	//					std::cerr << "strarting posting thread! for client:" << client._connection << std::endl;
	//					pThread = new std::thread(&serverHandle::RecvFromClient, this, std::ref(client), 4096);
	//					client._state = Client::connected_thread_on;
	//					break;
	//				case 1:							//client still connected and thread is already up
	//					break;
	//				case 2:							//Client deletion
	//					break; 
	//			}
	//			clientList_Lock.unlock();
	//		}
	//		std::this_thread::sleep_for(std::chrono::seconds(3));
	//	}
	//	return;
	//}
	

	//get information on current state of every client
	void postClientsInfo() {
		clientList_Lock.lock();
		for (Client& client : clientList) {
			std::string s_state = client._state == Client::disconnected ? "Disconnected" : "Connected";
			std::cout << client.ipv4_addr << '|' << client._name << '|' << s_state << std::endl;
		}
		clientList_Lock.unlock();
	}
	
	void kick(Client * client) {
		Message kick_msg(Message::text, std::format("[Server(Private)]: you were kicked!"));
		sendMsg(kick_msg, *client);
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		closesocket(client->_connection);
		std::cerr << client->_name << "Was kicked!" << std::endl;
		namesTable.erase(client->_name);
	}
	void kick(const std::string& name) {
		if (namesTable.contains(name)) {
			kick(namesTable[name]);
		}
		else
			std::cerr << "Name does not exist! kick by name failed!";
		return;
	}
	void handleServerInput() {
		while (true) {
			std::getline(std::cin,server_input);
			if (server_input[0] == '/') {
				size_t firstWS = server_input.find(' ');
				std::string command_name = server_input.substr(0, firstWS);
				if (command_name.size() > 10)
					std::cerr << "Invalid Command!" << std::endl;
				else {
					if (serverCommands.contains(command_name)) {
						std::string command_argument = server_input.substr(firstWS + 1, server_input.size());
						switch (serverCommands[command_name]) {
						case 0: //postinfo
							postClientsInfo();
							break;
						case 1: // stop
						{
							Message close_msg(Message::text, "[Server]: Server is shutting down...");
							sendMsgtoAll(close_msg);
							shutdown();
							return;
							break;
						}
						case 2: //say
						{
							Message msg(Message::text, std::format("[Server]:{}\n\r", std::move(command_argument)));
							sendMsgtoAll(msg);
							break;
						}
						case 3: //kick
							kick(command_argument);		
							break;
						}
					}
					else
						std::cerr << "Invalid Command!" << std::endl;
				}
				
			}
			else {
				/*Message msg(Message::text, std::format("[SERVER]:{}\n\r", std::move(server_input)));
				sendMsgtoAll(msg);*/
			}
		}
	}


};