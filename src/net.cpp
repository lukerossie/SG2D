#include "../include/net.hpp"
#include "../include/graphics.hpp"//msgbox
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <future>

namespace SG2D
{
	int defaultPort=37006;

	constexpr int maxMessageSizeBytes=100;

	TCPsocket connect(char const *ipAddress, int port)
	{
		IPaddress myip;
		SDLNet_ResolveHost(&myip,ipAddress,port);
		return SDLNet_TCP_Open(&myip);
	}

	void close(TCPsocket sock)
	{
		SDLNet_TCP_Close(sock);
	}

	void send(TCPsocket who, std::string data)
	{
		if(!who)return;
		if(!data.size())return;
		#ifdef LOUD
		std::cout<<"<SEND,FOLLOWEDBYNEWLINE>\n"+data+"</SEND>";
		#endif // LOUD

		SDLNet_TCP_Send(who,data.c_str(),data.length());//should not need terminating null
		char const *error=SDLNet_GetError();
		if(error)printf("%s",error);
		//printf("SENT: %s",data.c_str());
		//std::async(SDLNet_TCP_Send,who,data.c_str(),data.length()+1);
	}
	std::string recv(TCPsocket who)
	{
		char buff[maxMessageSizeBytes+1];//null terminated
		for(int i=0; i<maxMessageSizeBytes+1; i++)
		{
			buff[i]=0;
		}
		auto bytes=SDLNet_TCP_Recv(who,buff,maxMessageSizeBytes);
        //buff[maxMessageSizeBytes]=0;
		if(bytes<=0)
		{
			return "";
		}
		return std::string(buff);
	}
}

void initNet()
{
	if(SDLNet_Init()==-1)
	{
		printf("SDLNet_Init error: %s", SDLNet_GetError());
	}
}

#ifdef SG2D_TEST
void net_test()
{

}
#endif
