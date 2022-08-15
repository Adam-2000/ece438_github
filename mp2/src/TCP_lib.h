#ifndef _TCP_LIB_H_
#define _TCP_LIB_H_

#include <sys/time.h>

#include <iostream>
#include <string>
using std::string;
#include <sys/socket.h>

#define FLAGBIT_SYN 0b10
#define FLAGBIT_FIN 0b1
#define FLAGBIT_ACK 0x10

typedef struct{
    int32_t sequence_number;
    int32_t acknowledge_number;
    uint64_t sent_time_sec;
    uint8_t flags;
} TCP_header;
#define HEAD_LEN sizeof(TCP_header)



int TCP_send(TCP_header* tcp_header, int __fd, const void *__buf, size_t __n, int __flags, const sockaddr *__addr, socklen_t __addr_len){
    string head_data = string((char*)tcp_header, HEAD_LEN) + string((char*)__buf, __n);
    int num_sent = sendto(__fd, head_data.c_str(), head_data.length(), __flags, __addr, __addr_len);
    // std::cout<<"TCP_send: "<< (int)tcp_header->flags << " " <<tcp_header->acknowledge_number << " " << tcp_header->sequence_number << " " << string((char*)__buf, __n) << std::endl;
    if(num_sent == -1){
        return -1;
    } else {
        if (num_sent < HEAD_LEN){
            std::cout<<"TCP_send: error: num_sent"<<std::endl;
        }
        return num_sent - HEAD_LEN;
    }
}

int TCP_receive(TCP_header& tcp_header_other, string&data, int __fd, void *__restrict__ __buf, size_t __n, int __flags, sockaddr *__restrict__ __addr, socklen_t *__restrict__ __addr_len){
    int num_recv= recvfrom(__fd, __buf, __n + HEAD_LEN, __flags, __addr, __addr_len);
    if(num_recv == -1){
        return -1;
    } else {
        num_recv -= HEAD_LEN;
        if (num_recv < 0){
            std::cout<<"TCP_send: error: num_sent"<<std::endl;
        }
        tcp_header_other = *(TCP_header*)__buf;
        // std::cout<<"TCP_receive: "<< (int)tcp_header_other.flags << " " <<tcp_header_other.sequence_number << " " << tcp_header_other.acknowledge_number << " " << string((char*)__buf+HEAD_LEN, num_recv) << std::endl;
        if(!tcp_header_other.flags){
            data = string((char*)__buf+HEAD_LEN, num_recv);
            return num_recv;
        } else {
            return 0;
        }
    }
}

#endif