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
#include <algorithm>
using namespace std;

const uint16 MAX_PACKET_LEN = 1032;  // Maximum Packet Length
const uint16 MAX_SEQ_NUM    = 30720; // Maximum Sequence Number
const uint16 MAX_BODY_LEN   = 1024;  // Initial Congestion Window Size:
int TIME_OUT           = 500000;    // Retransmission Timeout: 500 ms 
int Connection         = 0;         // Is a TCP connection established
int FILE_TRANSFER_INIT = 0;         // Was the file to be sent transferred

int socketfd;
unsigned char *file_buf; // Malloc later based on file-Size
uint16 file_len = 0;

// Simple State Abstraction to Implement TCP
enum States { CLOSED, LISTEN, SYN_RECV,FILE_TRANSFER,FIN_SENT,FIN_WAITING};
enum cwndStates {SLOW_START, CONG_AVOID};

// Current States
States STATE = CLOSED;
cwndStates cwnd_STATE = SLOW_START;

// Resolve Hostname into IP (Simple DNS Lookup)
string dns(const char* hostname, const char* port);

// Catch Signals: If ^C, exit graecfully by closing socket
void signalHandler(int signal);

uint16 smallest(uint16 x, uint16 y, uint16 z) {
  return x < y ? (x < z ? x : z) : (y < z ? y : z);
}

uint16 get_bytes_to_send(uint16 LastByteSent, uint16 LastByteAcked, uint16 cwnd, uint16 fwnd) {
  uint16 limit = smallest(cwnd,fwnd,(MAX_SEQ_NUM+1)/2);
  if ((LastByteSent - LastByteAcked) < limit) {
    return limit - (LastByteSent - LastByteAcked);
  }
  return 0;
}

// Send Packets based on BytesToSend & Bytes Sent
int sendPackets(uint16 bytesToSend, uint16 bytesSent, uint16 cwnd);

int main(int argc, char* argv[]) {
  if (signal(SIGINT, signalHandler) == SIG_ERR)
    cerr << "Can't Catch Signal..." << endl;

  string host = "10.0.0.1";
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
  uint16 CLIENT_WINDOW=0;
  uint16 cwnd = MAX_BODY_LEN;
  uint16 ssthresh = MAX_SEQ_NUM;
  uint16 LastByteSent = 0;
  uint16 LastByteAcked = 0;


  STATE = LISTEN;
  while(1) {
    //    cout << "Current State: " << STATE << endl;
    unsigned char buf[MAX_PACKET_LEN];
    struct sockaddr_in client_addr;

    socklen_t len = sizeof(client_addr);
   
    struct timeval time_out;
    time_out.tv_sec = 0;
    time_out.tv_usec = TIME_OUT;
    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,&time_out,sizeof(time_out)) < 0) {
      perror("Error");
    }

    int recvlen = recvfrom(socketfd, buf, MAX_PACKET_LEN, 0, (struct sockaddr *)&client_addr, &len);

      // Check for 3 Way Handshake/Packet over existing Connection
      switch (STATE) {
        case CLOSED: 
	  break;
        case LISTEN:
	  {
	    if (recvlen < 0) break;
	    cout << "UDP PACKET RECEIVED..." << endl;
	    TCPPacket recv_packet = TCPPacket(buf, recvlen);  
	    cout << "Receiving data packet " << recv_packet.getSeqNumber() << endl;
	     // If SYN Received, send SYN-ACK, Change State to SYN_RECV
	  if ( recv_packet.getSYN() && !recv_packet.getACK() && !recv_packet.getFIN() ){
	    CLIENT_SEQ_NUM = recv_packet.getSeqNumber();
	    CLIENT_WINDOW = recv_packet.getWindowSize();
	    cout << "Receiving SYN packet: " << CLIENT_SEQ_NUM << endl;
	    srand(time(NULL));
	     //LastByteSent = rand() % MAX_SEQ_NUM;
	LastByteSent=0;
	 	cout << LastByteSent << endl;
	    // Send SYN-ACK
	    bitset<3> flags = bitset<3>(0x0);
	    flags.set(ACKINDEX,1);
	    flags.set(SYNINDEX,1);
	    TCPPacket syn_ack_packet = TCPPacket(LastByteSent, (CLIENT_SEQ_NUM+1)%MAX_SEQ_NUM, cwnd, flags,NULL,0);
	    unsigned char sendbuf[MAX_PACKET_LEN];
	    syn_ack_packet.encode(sendbuf);
	    int send_status = sendto(socketfd, sendbuf, sizeof(unsigned char)*syn_ack_packet.getLengthOfEncoding(), 0,(struct sockaddr *)&client_addr, len);
	    if (send_status < 0){
	      cerr << "Error Sending Packet...\nServer Closing..." << endl;
	      exit(1);
	    }
	    cout << "Sending data packet " << syn_ack_packet.getSeqNumber() << " " << cwnd << " SSThresh" << endl;
	    LastByteSent++;
	    STATE = SYN_RECV;
	  }
	  break;
	  }
        case SYN_RECV:
	  {
	  // If ACK Received, Change State to FILE_TRANSFER and begin to transfer the file. Start the congestion window and timeouts.
	  // Else retransmit SYN_ACK
	    if (recvlen < 0)break;
	    cout << "UDP PACKET RECEIVED..." << endl;
	    TCPPacket recv_packet = TCPPacket(buf, recvlen);  
	    cout << "Receiving data packet " << recv_packet.getSeqNumber() << endl;
	    //CHECK IF ACK IS CORRECT
	  if ( recv_packet.getACK() && !recv_packet.getSYN() && !recv_packet.getFIN() ){
	    CLIENT_SEQ_NUM = recv_packet.getSeqNumber();
	    CLIENT_WINDOW = recv_packet.getWindowSize();
	    LastByteAcked = recv_packet.getAckNumber();
	    cout << "Receiving ACK packet " << recv_packet.getAckNumber() << endl;
	    STATE = FILE_TRANSFER;
	    
	    if (!FILE_TRANSFER_INIT){
	      cout << "Finding File..." << endl;
	      FILE *fd = fopen("index.html", "rb");
	      fseek(fd,0,SEEK_END);
	      file_len = ftell(fd);
	      file_buf = (unsigned char *)malloc(file_len * sizeof(char));
	      fseek(fd, 0, SEEK_SET);
	      int resp_readStatus = fread(file_buf, file_len,1,fd);
	      fclose(fd);
	      if (resp_readStatus < 0){
		cerr << "Error: Failed to read from file..." << endl;
		cout << "Server Closing..." << endl;
		exit(1);
	      }
	      FILE_TRANSFER_INIT = 1;
	    }
	    cout << "==========FILE=====SIZE: " << file_len << " =====\n" << file_buf << endl;

	    uint16 bytes_to_send = get_bytes_to_send(LastByteSent,LastByteAcked,cwnd,CLIENT_WINDOW);
	    uint16 bytes_sent= sendPackets(bytes_to_send ,LastByteSent,cwnd);
	    if (bytes_sent < 0) {
	    }
	    else {
	      LastByteSent+=bytes_sent;
	    }
	    if ((file_len == LastByteSent) && (file_len==LastByteAcked)) {
	      //Send first ACK, change state to FIN_SENT
	    }
	    
	    
	  }
	  break;
	  }
        case FILE_TRANSFER:
	  {
	  //	  cout << "Transmitting File..." << endl;
	  // Deal with Request, retransmission, and congestion windows. Once we have gotten ACKs for all files, send a FIN and move on to FIN_SENT
	  // if timeout:
	  //    sshthresh = cwnd/2;
	  //    cwndState = SLOW_START
	  //    cwnd=1024;
	  //    change window to the front
	  // else:
	  //    if (ACK is within cwnd):
	  //        update LastByteAcked
	  //        if SLOW_START:
	  //           if (cwnd * 2 >= ssthresh)
	  //              cwndState = CONG_AVOID;
	  //              goto CONG_AVOID;
	  //           cwnd = cwnd *2;
	  //        if (CONG_AVOID)
	  //            cwnd+=Max Packet Length;	  
	  // Handle Ack

	  if ( recvlen < 0 /*TIMEOUT HAPPENS, MIGHT BE A DIFFERENT CASE */) {
	    ssthresh = cwnd/2;
	    cwnd_STATE = SLOW_START;
	    cwnd = 1024;
	    
	    //Change window to front
	    LastByteSent = LastByteAcked;
	  }else{
	    cout << "UDP PACKET RECEIVED..." << endl;
	    TCPPacket recv_packet = TCPPacket(buf, recvlen);  
	    cout << "Receiving data packet " << recv_packet.getSeqNumber() << endl;
	    if ( recv_packet.getACK() && !recv_packet.getSYN() && 
		 !recv_packet.getFIN() ){
	      cout << "ACK Received..." << endl;
	      CLIENT_SEQ_NUM = recv_packet.getSeqNumber();
	      CLIENT_WINDOW = recv_packet.getWindowSize();
	      cout << "Receiving ACK packet " << recv_packet.getAckNumber() << endl;
	    }
	    //CHECK RTT TIMER
	    LastByteAcked = (recv_packet.getAckNumber()>=LastByteAcked) ? recv_packet.getAckNumber() : LastByteAcked;
	    if (cwnd_STATE == SLOW_START) {
	      if (cwnd * 2 >= ssthresh) {
		cwnd_STATE = CONG_AVOID;
	      }
	      else {
		cwnd=cwnd*2;
	      }
	    }
	    if (cwnd_STATE == CONG_AVOID) {
	      cwnd+=MAX_BODY_LEN;
	    } 
	    
	  }
	  //Send what we need
	  uint16 bytes_to_send = get_bytes_to_send(LastByteSent,LastByteAcked,cwnd,CLIENT_WINDOW);
	  //START RTT TIMER
	  uint16 bytes_sent= sendPackets(bytes_to_send ,LastByteSent,cwnd);
	  if (bytes_sent < 0) {
	  }
	  else {
	    LastByteSent+=bytes_sent;
	  }
	  if ((file_len == LastByteSent) && (file_len==LastByteAcked)) {
	    //Send first ACK, change state to FIN_SENT
	  }

	break;
	  }
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

int sendPackets(uint16 bytesToSend, uint16 lastByteSent, uint16 cwnd){
  if (lastByteSent == file_len) // File Transfer Complete
    return lastByteSent;
  if (lastByteSent > file_len) // Error, more bytes sent then size of file
    return -1;
  uint16 BTS  = bytesToSend;
  uint16 LBS  = lastByteSent;
  uint16 CWND = cwnd;
  while( LBS < BTS ) {
    bitset<3> flags = bitset<3>(0x0);	      
    uint16 fl = min(BTS, (uint16)(file_len-LBS)); // Maximum amount of file left to send
    fl = min(fl, MAX_BODY_LEN);
    if(fl <= 0) // File Transfer Complete
      return LBS;
    TCPPacket packet = TCPPacket(LBS, 0, cwnd, flags, file_buf+LBS,fl);
    unsigned char sendbuf[MAX_PACKET_LEN];
    packet.encode(sendbuf);
    cout << "Sending data packet " << packet.getSeqNumber() << cwnd << " SSThresh" << endl;
    cout << "Sent Body: " << packet.getBodyLength() << " | LastByteSent: " << LBS << endl;
    
    int send_status = sendto(socketfd, sendbuf, sizeof(unsigned char)*packet.getLengthOfEncoding(), 0,(struct sockaddr *)&client_addr, len);
    if (send_status < 0){
      cerr << "Error Sending Packet...\nServer Closing..." << endl;
      return -1;
    }
    
    LBS+=packet.getBodyLength();
    cout << "File Sent..." << endl;	  
  }
  return LBS;
}
