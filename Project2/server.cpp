#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <netdb.h>
#include <fstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <thread>
#include <string.h>
#include <iostream>
#include <sstream>
#include "TCPPacket.h"
using namespace std;

const uint16 MAX_PACKET_LEN = 1032;  // Maximum Packet Length
const uint16 MAX_SEQ_NUM    = 30720; // Maximum Sequence Number
const uint16 CONGESTION_WIN = 1024;  // Initial Congestion Window Size:
const int TIME_OUT       = 500;   // Retransmission Timeout: 500 ms 
int Connection = 0;
int  socketfd;



// Simple State Abstraction to Implement TCP
enum States { CLOSED, LISTEN, SYN_RECV,FILE_TRANSFER,FIN_SENT,FIN_WAITING};

// Current State
States STATE = CLOSED;

// Resolve Hostname into IP (Simple DNS Lookup)
string dns(const char* hostname, const char* port);

// Catch Signals: If ^C, exit graecfully by closing socket
void signalHandler(int signal);

int main(int argc, char* argv[]) {
  if (signal(SIGINT, signalHandler) == SIG_ERR)
    cerr << "Can't Catch Signal..." << endl;

  string host = "localhost";
  string port = "4000";
  // Parse Command Line Arguments
  if (argc == 3) {
    host = argv[1];
    port = argv[2];
  }
  if (argc != 1 || argc > 3)
    cerr << "Invalid Command Line Arguments" << endl;
  
  cout << "==========================================================" << 
  endl;
  cout << "Initializing Server with Hostname: " << host << " | Port: " << 
  port << endl;
  
  // Resolve IP from Hostname
  const char* ip = dns(host.c_str(), port.c_str()).c_str();
  cout << "IP Address: " << ip << endl;
  
  if ( (socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
      cerr << "Error Creating Socket...\nServer Closing..." << endl;
      exit(1);
  }
  Connection = 1;
  
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(port.c_str()));
  serverAddr.sin_addr.s_addr = inet_addr(ip);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
  
  int bind_status;
  if ( (bind_status = bind(socketfd, (struct sockaddr*) &serverAddr, sizeof(serverAddr))) < 0 ){
    close(socketfd);	
    cerr << "Error Binding Socket to Address...\nServer Closing..." << endl;
    exit(2);
  } 
  // Listen for UDP Packets && Implement TCP 3 Way Handshake With States
  cout << "Listening for UDP Packets..." << endl;
  uint16 CLIENT_SEQ_NUM = 0;
  uint16 SERVER_SEQ_NUM = 0;
  uint16 CLIENT_WIN_SIZE = CONGESTION_WIN;
  uint16 SERVER_WIN_SIZE = CONGESTION_WIN;
  STATE = LISTEN;
  while(1) {
    cout << "Current State: " << STATE << endl;
    unsigned char buf[MAX_PACKET_LEN];
    struct sockaddr_in client_addr;
    int len = sizeof(client_addr);
    int recvlen = recvfrom(socketfd, buf, MAX_PACKET_LEN, 0, (struct sockaddr *)&client_addr, (socklen_t *)&len);
    if (recvlen >= 0) {
      cout << "UDP PACKET RECEIVED..." << endl;
      // Check for 3 Way Handshake/Packet over existing Connection
      switch (STATE) {
        case CLOSED: 
	  break;
        case LISTEN:
	  {
	  // If SYN Received, send SYN-ACK, Change State to SYN_RECV
	  TCPPacket recv_packet = TCPPacket(buf, recvlen);
	  if ( recv_packet.getSYN() && !recv_packet.getACK() && !recv_packet.getFIN() ){
	    CLIENT_SEQ_NUM = recv_packet.getSeqNumber();
	    SERVER_SEQ_NUM = rand() % MAX_SEQ_NUM;
	    // Send SYN-ACK
	    bitset<3> flags = bitset<3>(0x0);
	    flags.set(ACKINDEX,1);
	    flags.set(SYNINDEX,1);
	    TCPPacket syn_ack_packet = TCPPacket(SERVER_SEQ_NUM, (CLIENT_SEQ_NUM+1)%MAX_SEQ_NUM, SERVER_WIN_SIZE, flags,NULL,0);
	    unsigned char sendbuf[MAX_PACKET_LEN];
	    syn_ack_packet.encode(sendbuf);
	    int send_status = send(socketfd, sendbuf, sizeof(unsigned char)*syn_ack_packet.getLengthOfEncoding(),0);
	    if (send_status < 0){
	      cerr << "Error Sending Packet...\nServer Closing..." << endl;
	      exit(1);
	    }
	    STATE = SYN_RECV;
	  }
	  break;
	  }
        case SYN_RECV:
	  // If ACK Received, Change State to FILE_TRANSFER and begin to transfer the file. Start the congestion window and timeouts.
	  // Else retransmit SYN_ACK
	  break;
        case FILE_TRANSFER:
	  // Deal with Request, retransmission, and congestion windows. Once we have gotten ACKs for all files, send a FIN and move on to FIN_SENT
	break;
        case FIN_SENT:
	  // Wait for FIN_ACK from the client, retransmit if neccisary. Once that is done, move to FIN_WAITING and wait for the next packet.
	  break;
        case FIN_WAITING:
	  //if we get the last FIN, send a FIN_ACK and then "close" the connection.
	  break;

      }
    }
    
    close(socketfd);
    Connection = 0;
    return 0;
  }
  

  return 0;
}


std::string dns(const char* hostname, const char* port){
  int status = 0;
  struct addrinfo hints;
  struct addrinfo* res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  
  if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
    return NULL;
  }
  for (struct addrinfo* p = res; p != 0; p = p->ai_next) {
    struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    return std::string(ipstr);
  }
  return NULL;
}

void signalHandler(int signal){
  if(signal == SIGINT) {
    cerr << "\nReceived SIGINT...\nServer Closing..." << endl;
    if(Connection)
      close(socketfd);
    Connection = 0;
  }
  exit(0);
}
