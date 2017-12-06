#include <iostream>
#include <functional>
#include <string>

#include <cstdint>
#include <cerrno>	    // perror
#include <cstring>

#include <unistd.h>	    // access(), fork(), alarm()
#include <sys/types.h>
#include <sys/socket.h>	// socket(), connect()
#include <sys/select.h>	// select()
#include <sys/time.h>	// struct timeval
#include <sys/wait.h>	// wait()
#include <netinet/in.h>	// htons(), htonl()
#include <arpa/inet.h>	// inet_pton()
#include <signal.h>	    // signal()


inline void check_error(std::string && type,
                        int err,
                        std::function<bool(int)> &&pred = [](int i){return i < 0;},
                        std::function<void(void)> &&at_error = [](){std::terminate();})
{
	if(pred(err))
	{
		std::cout << type << " failed. err: [" << err << "]. Reason: " << std::strerror(errno) << std::endl;
		at_error();
	}
}

namespace udp_sender
{
	typedef std::uint8_t byte_t;
	constexpr std::size_t packet_data_size = 500;  
	constexpr std::size_t packet_header_size = 12; // total = 512
	constexpr std::size_t packet_size = packet_data_size + packet_header_size;
	/* 
	   [header] 
	   [0-3 : seq_num] 
	   [4-7 : ack_seq_num] 
	   [8-11: /user define/]
	*/
	
	
	int parse_endpoint(struct sockaddr_in & sv, std::string && target)
	{
		auto pos = target.find(":");
		std::string addr = target.substr(0, pos);
		std::string port = target.substr(pos+1);
 
		std::memset(&sv, 0, sizeof(sockaddr_in));
		sv.sin_family = AF_INET;
		sv.sin_port = htons(std::stoi(port));
		sv.sin_addr.s_addr = ((addr == "")? INADDR_ANY: inet_addr(addr.c_str()));
		return 0;
	}

	class Packet
	{
	public:
		Packet(int fd, int seq): socket_fd(fd), seq_num(seq) {}
		Packet(byte_t b[], int size) { std::copy(b, b + size, this->data); }
		byte_t data[packet_size];
		int socket_fd;
		int seq_num;
		int ack_seq_num;
		struct sockaddr * addr;
		template<class iterator_type>
		Packet& data_set(iterator_type begin, iterator_type end)
		{
			std::memset(data, 0, sizeof(data));
			int i(0);
			for(; i < 4; i++)
				data[i] = static_cast<char*>(static_cast<void*>(&seq_num))[i];
			for(; i < 8; i++)
				data[i] = static_cast<char*>(static_cast<void*>(&ack_seq_num))[i];
			for(; i < 12; i++)
				data[i] = 0; // DKNWTDN

			for(iterator_type it(begin); it != end && i < packet_size; ++it, ++i)
			{
				data[i] = *it;
			}
			return *this;
		}

		Packet& set_ack_num(int n)
		{
			ack_seq_num = n;
			for(int i(4); i < 8; i++)
                data[i] = static_cast<char*>(static_cast<void*>(&ack_seq_num))[i];
			return *this;
		}
		Packet& set_sockaddr(struct sockaddr * a)
		{
			this->addr = a;
			return *this;
		}
		Packet& send_sig(void (*on_failure)(int))
		{
			sendto(socket_fd, &data, sizeof(data) + 1, 0, this->addr, sizeof(this->addr));
			signal(SIGALRM, on_failure);
			siginterrupt(SIGALRM, 1);
			std::cout << "signaled\n";
			return *this;
		}

		std::size_t addr_len()
		{
			return sizeof(this->addr);
		}
		int extract_ack()
		{
			return int(data[4] << 24 |
			           data[5] << 16 |
			           data[6] << 8  |
			           data[7]);
		}
	};
}
