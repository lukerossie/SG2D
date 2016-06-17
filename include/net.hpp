#pragma once

#ifdef __APPLE__
#include <SDL2_net/SDL_net.h>
#else
#include <SDL2/SDL_net.h>
#endif

#include <string>

void initNet();

namespace SG2D
{//max message size defaults to 100 in net.cpp
	extern int defaultPort;
	extern TCPsocket mysocket;
	
	TCPsocket connect(char const *ipAddress=0, int port=defaultPort);
	
	//Do not use this function on a connected socket. Server sockets are never connected to a remote host.
	// - SDL documentation
	TCPsocket accept();
	
	void close(TCPsocket sock);
	
	void send(TCPsocket who, std::string data);
	std::string recv(TCPsocket who);
}