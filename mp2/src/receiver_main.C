/* 
 * File:   receiver_main.c
 * Author: 
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include <vector>
#include "error.h"
#include <iostream>
#include <string>
using std::string;
#include <unistd.h>
#include "TCP_lib.h"

struct sockaddr_in si_me, si_other;
int s, slen;

void diep(string s) {
    perror(s.c_str());
    exit(1);
}

#define FLAGBIT_SYN 0b10
#define FLAGBIT_FIN 0b1
#define FLAGBIT_ACK 0x10

#define HEAD_LEN sizeof(TCP_header)
#define MSS (1472 - HEAD_LEN)
#define MAXBUFLEN (1472+1)
#define STORAGE_SIZE 2000
#define MAX_RECV_WINDOW_SIZE 2048

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

class packets_buffer {
public:
    packets_buffer():head(0), after_tail(0), packets(std::vector<string>()), flags(std::vector<bool>()){}
    void gotpacket(string& ret, const uint32_t seq, const string& data){
        // uint32_t next_after_tail;
        ret = string();
        std::vector<bool>::iterator i_flag;
        std::vector<string>::iterator i_packets;
        // if((int64_t)after_tail - (int64_t)head >= MAX_RECV_WINDOW_SIZE){
        //     return;
        // }
        if(seq == head){
            if(seq >= after_tail){
                head = after_tail = seq + 1;
                ret = data;
                return;
            }
            ret = data;
            i_flag = flags.begin()+1;
            i_packets = packets.begin()+1;
            for(head = head + 1; head < after_tail; head++){
                if(!(*i_flag)){
                    break;
                }
                ret += *i_packets;
                i_flag++;
                i_packets++;
            }
            packets.erase(packets.begin(), i_packets);
            flags.erase(flags.begin(), i_flag);
            return;
        } else if (seq > head){
            if(seq < after_tail){
                if(!flags[seq - head]){
                    flags[seq - head] = true;
                    packets[seq - head] = data;
                }
            } else {
                flags.insert(flags.end(), seq - after_tail, false);
                packets.insert(packets.end(), seq - after_tail, string());
                flags.push_back(false);
                packets.push_back(data);
                after_tail = seq + 1;
            }
        }
    }
    int32_t get_ack(){
        return (int32_t)head - 1;
    }

private:
    uint32_t head;
    uint32_t after_tail;
    std::vector<string> packets;
    std::vector<bool> flags;
};

void reliablyReceive(unsigned short int myUDPport, char* destinationFile) {
    
    slen = sizeof (si_other);
    FILE* fp = fopen(destinationFile, "wb");

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        diep("socket");

    memset((char *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(myUDPport);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    printf("Now binding\n");
    if (bind(s, (struct sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");


	/* Now receive data and send acknowledgements */    
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int numbytes;
    char address_string[INET6_ADDRSTRLEN];
    char receive_buf[MAXBUFLEN];
    receive_buf[MAXBUFLEN] = '\0';
    TCP_header tcp_header;
    memset((char*)&tcp_header, 0, sizeof(tcp_header));
    TCP_header tcp_header_other;
    string receive_data;
    char send_buf[] = "";
    string send_data(send_buf);
    enum recv_state {s_0, s_reveived_syn, s_receiving, s_received_fin, s_received_fin_ack};
    recv_state state = s_0;
    unsigned current_ack = 0;
    unsigned largest_ack = 0;
    string data_storage;
    packets_buffer pkt_buf;
    struct timeval tv;
	printf("listener: waiting to recvfrom...\n");
    /* Start three-way handshake*/
    while(1){
        switch(state){
            case s_0: {
                // std::cout << "receive: state: " << state << std::endl;
                if ((numbytes = TCP_receive(tcp_header_other, receive_data, s, receive_buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom: s_0");
                    break;
                }
                if(tcp_header_other.flags & FLAGBIT_SYN){
                    tcp_header.sequence_number = -1;
                    tcp_header.acknowledge_number = tcp_header_other.sequence_number;
                    tcp_header.flags = FLAGBIT_SYN;
                    state = s_reveived_syn;
                } else {
                    // std::cout<<"Receiver: s_0; No hand shake"<<std::endl;
                }
                break;
            }
            case s_reveived_syn : {
                // std::cout << "receive: state: " << state << std::endl;
                tcp_header.sequence_number = -1;
                tcp_header.acknowledge_number = tcp_header_other.sequence_number;
                tcp_header.flags = FLAGBIT_SYN;
                while ((numbytes = TCP_send(&tcp_header, s, "", 0, 0, (sockaddr*)&their_addr, addr_len)) == -1) {
                    perror("receiver: s_0; handshake send");
                    usleep(1000);
                }
                if ((numbytes = TCP_receive(tcp_header_other, receive_data, s, receive_buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom: s_0");
                    break;
                }
                while(tcp_header_other.flags & FLAGBIT_SYN){
                    while ((numbytes = TCP_send(&tcp_header, s, "", 0, 0, (sockaddr*)&their_addr, addr_len)) == -1) {
                        perror("receiver: s_0; handshake send");
                        std::cout << "receiver: 1" << std::endl;
                        usleep(1000);
                    }  
                    if ((numbytes = TCP_receive(tcp_header_other, receive_data, s, receive_buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                        perror("recvfrom: s_0");
                        break;
                    }
                }
                if(tcp_header_other.flags & FLAGBIT_FIN){
                    state = s_received_fin;
                    tcp_header.flags = FLAGBIT_ACK;
                    tcp_header.sent_time_sec = tcp_header_other.sent_time_sec;
                    while ((numbytes = TCP_send(&tcp_header, s, "", 0, 0, (sockaddr*)&their_addr, addr_len)) == -1) {
                        perror("receiver: s_0; handshake send");
                        usleep(1000);
                    }
                    break;
                }
                pkt_buf.gotpacket(data_storage, tcp_header_other.sequence_number, receive_data);
                fwrite(data_storage.c_str(), data_storage.length(), sizeof(char), fp);
                tcp_header.acknowledge_number = pkt_buf.get_ack();
                tcp_header.flags = FLAGBIT_ACK;
                tcp_header.sent_time_sec = tcp_header_other.sent_time_sec;
                while ((numbytes = TCP_send(&tcp_header, s, "", 0, 0, (sockaddr*)&their_addr, addr_len)) == -1) {
                    perror("receiver: s_0; handshake send");
                    std::cout << "receiver: 1" << std::endl;
                    usleep(1000);
                }
                // std:
                // std::cout << data_storage << state << std::endl;
                state = s_receiving;
                break;
            }
            case s_receiving : {
                // std::cout << "receive: state: " << state << std::endl;
                if ((numbytes = TCP_receive(tcp_header_other, receive_data, s, receive_buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom: s_0");
                    break;
                }
                if(tcp_header_other.flags & FLAGBIT_FIN){
                    state = s_received_fin;
                    tcp_header.flags = FLAGBIT_ACK | FLAGBIT_FIN;
                    tcp_header.sent_time_sec = tcp_header_other.sent_time_sec;
                    while ((numbytes = TCP_send(&tcp_header, s, "", 0, 0, (sockaddr*)&their_addr, addr_len)) == -1) {
                        perror("receiver: s_0; handshake send");
                        usleep(100);
                    }
                    break;
                }
                pkt_buf.gotpacket(data_storage, tcp_header_other.sequence_number, receive_data);
                fwrite(data_storage.c_str(), data_storage.length(), sizeof(char), fp);
                tcp_header.acknowledge_number = pkt_buf.get_ack();
                tcp_header.flags = FLAGBIT_ACK;
                tcp_header.sent_time_sec = tcp_header_other.sent_time_sec;
                while ((numbytes = TCP_send(&tcp_header, s, "", 0, 0, (sockaddr*)&their_addr, addr_len)) == -1) {
                    perror("receiver: s_0; handshake send");
                    usleep(100);
                }
                break;
            }
            case s_received_fin : {
                // std::cout << "receive: state: " << state << std::endl;
                tcp_header.flags = FLAGBIT_FIN;
                while ((numbytes = TCP_send(&tcp_header, s, "", 0, 0, (sockaddr*)&their_addr, addr_len)) == -1) {
                    perror("receiver: s_received_fin");
                    usleep(100);
                }
                tv.tv_sec = 5;
                tv.tv_usec = 0;
                while (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                    perror("Sender: set timeout Error");
                    usleep(100);
                }
                if ((numbytes = TCP_receive(tcp_header_other, receive_data, s, receive_buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom: s_received_fin");
                    // Whatever
                    goto Finished;
                }
                if (tcp_header_other.flags & FLAGBIT_FIN){
                    tcp_header.flags = FLAGBIT_ACK | FLAGBIT_FIN;
                    while ((numbytes = TCP_send(&tcp_header, s, "", 0, 0, (sockaddr*)&their_addr, addr_len)) == -1) {
                        perror("receiver: s_0; handshake send");
                        usleep(100);
                    }
                    break;
                }
                if (!(tcp_header_other.flags & FLAGBIT_ACK)){
                    // break;
                    std::cout << "Receiver: Err no fin ack?" << std::endl;
                }
                goto Finished;
                break;
            }
            default : {
                std::cout << "receive: state: " << state << "current_ack: " << current_ack << "largest_ack: " << largest_ack << std::endl;
                std::cout << "Receiver: why default?" << std::endl;
                break;
            }
        }
    }
Finished:;

	printf("listener: got packet from %s\n", inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),address_string, sizeof address_string));
    fseek(fp, 0, SEEK_END);
    unsigned long long int len_file = (unsigned long)ftell(fp);	
    fseek(fp, 0, SEEK_SET);
	printf("listener: packet is %lld bytes long\n", len_file);
	// printf("listener: packet contains \"%s\"\n", receive_buf);
    // std::cout << "listener: packet data contains" << " "<< receive_data << std::endl;
    
    close(s);
    fclose(fp);
	printf("%s received.\n", destinationFile);
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {

    unsigned short int udpPort;

    if (argc != 3) {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);

    reliablyReceive(udpPort, argv[2]);
}

