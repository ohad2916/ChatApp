#pragma once
#include <format>

struct Client
{	
	static int default_count;
	enum State {connected_no_hthread,connected_thread_on,disconnected};
	std::string _name;
	SOCKET _connection;
	State _state;
	sockaddr_in _addrinf;
	char ipv4_addr[INET_ADDRSTRLEN];

	Client(SOCKET& conn,sockaddr_in &addrinf) //deep copies
		:_connection(conn), _state(connected_no_hthread),_addrinf(addrinf) 
	{
		_name = std::format("unnamed_{}", default_count++);
		inet_ntop(AF_INET, &_addrinf.sin_addr.S_un.S_addr, ipv4_addr, INET_ADDRSTRLEN);
	}

};

