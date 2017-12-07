#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../packet.hpp"

constexpr int TIMEOUT = 3;
std::vector<Packet> packets;
int now_transmitting = 0;

int main(int argc, char *argv[])
{
	check_error("process arguments...", argc, [](int arg){return arg != 4;}, []{std::cout << "launch format error\n"; std::terminate();});
	struct sockaddr_in sock;
	parse_endpoint(sock, std::string(argv[1]) + ":" + std::string(argv[2]));

	int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	check_error("socket", socketfd);

	std::ifstream input(argv[3], std::ios::in | std::ios::binary);
	std::string file_data = [&input]{ std::stringstream ss; ss << input.rdbuf(); return ss.str();}();
	std::size_t size = file_data.size();

	siginterrupt(SIGALRM, 1);
	signal(SIGALRM,
		   +[](int sig)
		   {
			   std::cout << "TIMEOUT!!!\n";
			   for(int i(0); i<=now_transmitting; i++)
			   {
				   packets.at(i).send();
				   alarm(TIMEOUT);
			   }
		   });
	
	{
        Packet p(socketfd, 0);
        p.set_sockaddr(sock)
         .set_filename(std::string(argv[3]));
        packets.push_back(p);
    }
    for(int pos(0), i(1); pos < size; pos += packet_data_size)
    {
        Packet packet(socketfd, i++);
        packet.set_sockaddr(sock)
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
			alarm(TIMEOUT);
            byte_t raw[packet_size];            
			std::size_t len = recvfrom(socketfd, raw, packet_size, 0, NULL, NULL);
			Packet p(raw, len);
			int ack = p.extract_ack();
			std::cout << "ack: " << ack << std::endl;
			now_transmitting = ack > now_transmitting ? ack: now_transmitting;
        }
    }
    {
        Packet p(socketfd, size);
        p.set_sockaddr(sock)
         .set_finish();
        for(int i(20); i; i-- )
            p.send();
    }
	return 0;
}
