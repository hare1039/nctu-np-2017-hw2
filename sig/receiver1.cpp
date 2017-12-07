#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <functional>
#include <string>
#include <chrono>
#include <thread>
#include <map>
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
#include "../packet.hpp"
#include "../utility.hpp"


constexpr int TIMEOUT = 1;
std::vector<Packet> packets;
int now_transmitting = 0;
bool close_alarm = false;

int main(int argc, char *argv[])
{
	check_error("process arguments...", argc, [](int arg){return arg != 2;}, []{std::cout << "launch format error\n"; std::terminate();});
	struct sockaddr_in sockaddr;
	parse_endpoint(sockaddr, ":" + std::string(argv[1]));

	int socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	check_error("socket", socketfd);

	int err = bind(socketfd, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
	check_error("binding...", err);
	
	siginterrupt(SIGALRM, 1);
    signal(SIGALRM,
           +[](int sig)
           {
               std::cout << "TIMEOUT!!!\n";
               for(int i(0); i<=now_transmitting; i++)
               {
                   packets.at(i).send();
               }
			   if(close_alarm)
			   {
				   alarm(0);
				   std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				   close_alarm = false;
			   }
			   else
				   alarm(TIMEOUT);
			   
           });

	std::ofstream out_file;
	std::cout << "started\n";
	std::string cur_file_path;
	std::map<int, std::vector<byte_t>> file_blocks;
	for(;;)
	{
		byte_t raw[packet_size];
		std::memset(raw, 0, packet_size);
		struct sockaddr_in client;
		socklen_t so_size = sizeof(struct sockaddr_in);
		int len = recvfrom(socketfd, raw, packet_size, 0, (struct sockaddr *)&client, &so_size);
		check_error("recvfrom...", len, [](int i){return i < 0;}, [](){ alarm(0);});
		Packet p(raw, len);
		if(p.is_filename_packet())
		{
			if(cur_file_path != p.get_data_as_string())
			{
				std::cout << "new file " << p.get_data_as_string() << "\n";
				cur_file_path = p.get_data_as_string();
				out_file.open("trans_" + cur_file_path, std::ios::out | std::ios::binary);
			}
		}
		else if(p.should_finish())
		{
			if(file_blocks.empty())
				continue;
			std::cout << "finished\n";
			
			for(auto &blocks: file_blocks)
			{
				std::stringstream ss(std::string(blocks.second.begin(), blocks.second.end()));
				out_file << ss.rdbuf();
			}
			alarm(0);
			now_transmitting = 0;
			close_alarm = true;
			{std::map<int, std::vector<byte_t>>().swap(file_blocks);}
			out_file.close();
			cur_file_path = "";
		}
		else
		{
			std::vector<byte_t> data;
			for(int i(12); i < len && i < packet_size; i++)
			    if(raw[i])
					data.push_back(p.data[i]);
			
			file_blocks.insert(std::pair<int, std::vector<byte_t>>(p.seq_num_from_data(), data)); 
			
		}
		{
			Packet ret(socketfd, p.seq_num_from_data());
			ret.set_ack_num(p.seq_num_from_data()).set_sockaddr(client);
			packets.push_back(ret);
			packets.back().send();
			if(close_alarm)
				alarm(0);
			else
				alarm(TIMEOUT);
			now_transmitting = p.extract_ack() < now_transmitting? now_transmitting: p.extract_ack();			
		}
	}
	return 0;
}



