#ifndef __UTLITY_HPP__
#define __UTLITY_HPP__
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
#endif // __UTLITY_HPP__
