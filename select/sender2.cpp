#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
#include <string>

#include <cstdint>
#include <cerrno>       // perror
#include <cstring>

#include <unistd.h>     // access(), fork(), alarm()
#include <sys/types.h>
#include <sys/socket.h> // socket(), connect()
#include <sys/select.h> // select()
#include <sys/time.h>   // struct timeval
#include <sys/wait.h>   // wait()
#include <netinet/in.h> // htons(), htonl()
#include <arpa/inet.h>  // inet_pton()
#include <signal.h>     // signal()
#include "packet.hpp"
#include "utility.hpp"


constexpr int TIMEOUT = 3;
std::vector<Packet> packets;
int now_transmitting = 0;

int select_timeout(int sockfd, int sec)
{
	fd_set rset;
	struct timeval tv;

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);

	tv.tv_sec = sec;
	tv.tv_usec = 0;

	return (select(sockfd + 1, &rset, NULL, NULL, &tv));
}

void redo()
{
	std::cout << "redo\n";
	for(int i(0); i<=now_transmitting; i++)
	{
		packets.at(i).send();
	}
}

int main(int argc, char *argv[])
{
	check_error("process arguments...", argc, [](int arg){return arg != 4;}, []{std::cout << "launch format error\n"; std::terminate();});
	struct sockaddr_in sockaddr;
	parse_endpoint(sockaddr, std::string(argv[1]) + ":" + std::string(argv[2]));


	int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	check_error("socket", socketfd);

	std::ifstream input(argv[3], std::ios::in | std::ios::binary);
	std::string file_data = [&input]{ std::stringstream ss; ss << input.rdbuf(); return ss.str();}();
	std::size_t size = file_data.size();

	{
		Packet p(socketfd, 0);
		p.set_sockaddr(sockaddr)
		 .set_filename(std::string(argv[3]));
		packets.push_back(p);
	}
	for(int pos(0), i(1); pos < size; pos += packet_data_size)
	{
		Packet packet(socketfd, i++);
		packet.set_sockaddr(sockaddr)
		      .data_set(file_data.begin() + pos,
		                pos + packet_data_size < size?
		                    file_data.begin() + pos + packet_data_size:
						    file_data.end());
		packets.push_back(packet);
	}
	size = packets.size();
	std::cout << size << "\n";
	while(now_transmitting < size - 1)
	{
		for(int i(0); i < size; i++)
		{
			packets.at(i).set_ack_num(now_transmitting).send();

			byte_t raw[packet_size];
			if(select_timeout(socketfd, TIMEOUT) == 0)
				redo();
			else
			{
				std::size_t len = recvfrom(socketfd, raw, packet_size, 0, NULL, NULL);
				Packet p(raw, len);
				int ack = p.extract_ack();
				std::cout << "ack: " << ack << std::endl;
				now_transmitting = ack > now_transmitting ? ack: now_transmitting;
			}
		}
	}
	{
		Packet p(socketfd, size);
		p.set_sockaddr(sockaddr)
		 .set_finish();
	    for(int i(20); i; i-- )
			p.send();
	}

	return 0;
}
