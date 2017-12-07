#ifndef __PACKET_HPP__
#define __PACKET_HPP__
#include "utility.hpp"

typedef std::uint8_t byte_t;
constexpr std::size_t packet_data_size = 500;
constexpr std::size_t packet_header_size = 12; // total = 512                                                      
constexpr std::size_t packet_size = packet_data_size + packet_header_size;
class Packet
{
public:
	Packet(int fd, int seq): socket_fd(fd)
	{
		std::memset(data, 0, sizeof(data));
		seq_num.int_t = seq;
	}
	Packet(byte_t b[], int size)
	{
		std::memset(data, 0, sizeof(data));
		for(int i(0); i < size; i++)
			this->data[i] = b[i];
	}
	byte_t data[packet_size];
	int socket_fd;
	union
	{
		int int_t;
		byte_t byte_t[4];
	} seq_num, ack_seq_num;
	struct sockaddr_in addr;
	template<class iterator_type>
	Packet& data_set(iterator_type begin, iterator_type end)
	{
		std::memset(data, 0, sizeof(data));
		int i(0);
		for(; i < 4; i++)
            data[i] = seq_num.byte_t[i];
		for(; i < 8; i++)
			data[i] = ack_seq_num.byte_t[i];
		for(; i < 12; i++)
			data[i] = 0; // options: [8]: is file name
		                 // options: [9]: close connection
		                 //
		                 //

		for(iterator_type it(begin); it != end && i < packet_size; ++it, ++i)
		{
			data[i] = *it;
		}
		return *this;
	}

	Packet& set_ack_num(int n)
	{
		ack_seq_num.int_t = n;
		for(int i(4); i < 8; i++)
            data[i] = ack_seq_num.byte_t[i - 4];
		return *this;
	}
	Packet& set_sockaddr(struct sockaddr_in a)
	{
		this->addr = a;
		return *this;
	}

	Packet& set_filename(std::string && s)
	{
		std::memset(data + 12, 0, sizeof(data) - 12);
		data[8] = 1;
//		std::cout << "name: '" << s << "'" << "\n";
		for(int i(12); i - 12 < s.size(); i++)
			data[i] = s.at(i - 12);
		return *this;
	}
	bool is_filename_packet()
	{
		return (data[8])? true: false;
	}

	int seq_num_from_data()
	{
		for(int i(0); i<4; i++)
			seq_num.byte_t[i] = data[i];
		return seq_num.int_t;
	}
	
	Packet& set_finish()
	{
		std::memset(data + 12, 0, sizeof(data) - 12);
        data[9] = 1;
		return *this;
	}

	bool should_finish()
	{
		return (data[9])? true: false;
	}

	std::string get_data_as_string()
	{
		std::string d;
		for(int i(12); i<packet_size; i++)
			if(data[i] == '\0')
				break;
			else
				d += data[i];
		return d;
	}
	
	Packet& send()
	{
		int size = sizeof(sockaddr);
		std::cout << &addr << "\n";
		std::size_t len = sendto(socket_fd, data, packet_size, 0, (struct sockaddr*)&addr, size);
		check_error("sending...", len);
		return *this;
	}

	std::size_t addr_len()
	{
		return sizeof(this->addr);
	}
	int extract_ack()
	{
		int ack = 0;
		for(int i(4), k(0); i < 8; i++, k += 8)
            ack |= data[i] << k;
		
		return ack;
	}
};
#endif // __PACKET_HPP__
