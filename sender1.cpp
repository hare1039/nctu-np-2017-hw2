#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "sig_alarm.hpp"

constexpr int TIMEOUT = 3;
std::vector<udp_sender::Packet> packets;
int now_transmitting = 0;



int main(int argc, char *argv[])
{
	check_error("process arguments...", argc, [](int arg){return arg != 4;}, []{std::cout << "launch format error\n"; std::terminate();});
	struct sockaddr_in sock;
	udp_sender::parse_endpoint(sock, std::string(argv[1]) + ":" + std::string(argv[2]));
	int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	check_error("socket", socketfd);
	int err = connect(socketfd, reinterpret_cast<struct sockaddr *>(&sock), sizeof(sock));
	check_error("connect...", err);
	
	std::ifstream input(argv[3], std::ios::in | std::ios::binary);
	std::string file_data = [&input]{ std::stringstream ss; ss << input.rdbuf(); return ss.str();}();
	std::size_t size = file_data.size();

	int now_rcved_seq_num = 0;
	for(int pos(0), i(0); pos < size; pos += udp_sender::packet_data_size)
	{
		packets.push_back(udp_sender::Packet(socketfd, i++)
		                  .data_set(file_data.begin() + pos,
		                            pos + udp_sender::packet_data_size < size?
		                                file_data.begin() + pos + udp_sender::packet_data_size:
			                            file_data.end()));
	}
	size = packets.size();
	std::cout << size << "\n";
	for(int i(0); i < size; i++)
	{
		packets.at(i)
			.set_ack_num(now_transmitting)
			.send_sig(+[](int sig) {
					std::cout << "TIMEOUT!!!\n";
				for(int i(0); i<=now_transmitting; i++)
			    {
				    packets.at(i).send_sig(+[](int i){});
				    alarm(TIMEOUT);
    			}
		    }
	    );
		udp_sender::byte_t raw[udp_sender::packet_size];
		std::size_t addr_len = packets.at(i).addr_len();
		std::size_t len = recvfrom(socketfd, raw, udp_sender::packet_size, 0, packets.at(i).addr, reinterpret_cast<socklen_t *>(&addr_len));
		int ack = udp_sender::Packet(raw, len).extract_ack();
		now_transmitting = ack > now_transmitting ? ack: now_transmitting;
	}
	
	return 0;
}
