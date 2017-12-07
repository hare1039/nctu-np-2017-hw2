#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "sig_alarm.hpp"

constexpr int TIMEOUT = 3;
std::vector<udp_sender::Packet> packets;
int now_transmitting = 0;

void redo(int sig)
{
	std::cout << "TIMEOUT!!!\n";
	for(int i(0); i<=now_transmitting; i++)
	{
		packets.at(i).send_sig(redo);
		alarm(TIMEOUT);
	}
}

int main(int argc, char *argv[])
{
	check_error("process arguments...", argc, [](int arg){return arg != 4;}, []{std::cout << "launch format error\n"; std::terminate();});
	struct sockaddr_in sock;
	udp_sender::parse_endpoint(sock, std::string(argv[1]) + ":" + std::string(argv[2]));
	
	int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	check_error("socket", socketfd);
//	int err = connect(socketfd, reinterpret_cast<struct sockaddr *>(&sock), sizeof(sock));
//	check_error("connect...", err);
	
	std::ifstream input(argv[3], std::ios::in | std::ios::binary);
	std::string file_data = [&input]{ std::stringstream ss; ss << input.rdbuf(); return ss.str();}();
	std::size_t size = file_data.size();

	for(int pos(0), i(0); pos < size; pos += udp_sender::packet_data_size)
	{
		udp_sender::Packet packet(socketfd, i++);
		packet.set_sockaddr(&sock)
		      .data_set(file_data.begin() + pos,
		                pos + udp_sender::packet_data_size < size?
		                    file_data.begin() + pos + udp_sender::packet_data_size:
						    file_data.end());
		packets.push_back(packet);
	}
	size = packets.size();
	std::cout << size << "\n";
	for(int i(0); i < size; i++)
	{
		packets.at(i)
			.set_ack_num(now_transmitting)
			.send_sig(redo);
		udp_sender::byte_t raw[udp_sender::packet_size];
		std::size_t len = recvfrom(socketfd, raw, udp_sender::packet_size, 0, NULL, NULL);
		
		int ack = udp_sender::Packet(raw, len).extract_ack();
		std::cout << "ack: " << len << std::endl; 
		now_transmitting = ack > now_transmitting ? ack: now_transmitting;
	}
	while(now_transmitting < size);
	return 0;
}
