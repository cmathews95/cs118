#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
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
#include "timer.h"
#include <algorithm>
using namespace std;

struct ackInfo
{
  timer RTTtimer;
  bool isRetrans=false;
} ackInfo;

const uint16 MAX_PACKET_LEN = 1032;  // Maximum Packet Length
const uint16 MAX_SEQ_NUM    = 30720; // Maximum Sequence Number
struct ackInfo ackArr[MAX_SEQ_NUM];
const uint16 MAX_BODY_LEN   = 1024;  // Initial Congestion Window Size:
int TIME_OUT           = 500000;    // Retransmission Timeout: 500 ms 
int Connection         = 0;         // Is a TCP connection established
int FILE_TRANSFER_INIT = 0;         // Was the file to be sent transferred
bool isInitRTT = true;




long long SRTT = 3000000;
long long DevRTT = 3000000;
long long RTO =  SRTT + 4*DevRTT;



double  alpha = 1.0/8.0;
double beta = 1.0/4.0;

int socketfd;
unsigned char *file_buf; // Malloc later based on file-Size

long long int file_len = 0;
long long int bytes_sent = 0;
struct timeval packet_TO; // Time Last Packet was Sent

// Simple State Abstraction to Implement TCP
enum States { CLOSED, LISTEN, SYN_RECV, FILE_TRANSFER, FIN_SENT, FIN_WAITING, TIMED_WAIT};
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
  uint16 diff;
  if (LastByteSent < LastByteAcked) {
    diff = (MAX_SEQ_NUM - LastByteAcked) + LastByteSent;
  }
  else {
    diff = LastByteSent - LastByteAcked;
  }
  if (diff < limit) {
    return limit - diff;
  }
  return 0;
}

// Send Packets based on BytesToSend & Bytes Sent

uint16 sendPackets(uint16 bytesToSend,uint16 LastByteSent, uint16 cwnd,uint16 ssthresh,struct sockaddr_in c_addr,bool hasPassedMaxBytes,long long int MaxBytesSent);
void updateRTO(double RTT);
timer * findClosestTimer(uint16 LastByteAcked, uint16 LastByteSent);
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
  uint16 CLIENT_SEQ_NUM = 0;
  uint16 CLIENT_WINDOW=0;
  uint16 cwnd = MAX_BODY_LEN;
  uint16 ssthresh = 15360;
  uint16 LastByteSent = 0;
  long long int  MaxBytesSent = 0;
  bool hasPassedMaxBytes = true;
  uint16 LastByteAcked = 0;

  STATE = LISTEN;
  while(1) {
    unsigned char buf[MAX_PACKET_LEN];
    struct sockaddr_in client_addr;

    socklen_t len = sizeof(client_addr);
   
    struct timeval time_out;
    time_out.tv_sec  = (TIME_OUT/1000000);
    time_out.tv_usec = (TIME_OUT%1000000);
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
	    TCPPacket recv_packet = TCPPacket(buf, recvlen);  
	    cout << "Receiving packet " << recv_packet.getAckNumber() << endl;
	     // If SYN Received, send SYN-ACK, Change State to SYN_RECV
	  if ( recv_packet.getSYN() && !recv_packet.getACK() && !recv_packet.getFIN() ){
	    CLIENT_SEQ_NUM = recv_packet.getSeqNumber();
	    CLIENT_WINDOW = recv_packet.getWindowSize();
	    srand(time(NULL));
	    LastByteSent=0;
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
	    cout << "Sending packet " << syn_ack_packet.getSeqNumber() << " " << cwnd << " " << ssthresh << " SYN"  << endl;
	    
	    ackArr[(LastByteSent+1)].RTTtimer.start(RTO);
	    STATE = SYN_RECV;
	  }
	  break;
	  }
        case SYN_RECV:
	  {
	  // If ACK Received, Change State to FILE_TRANSFER and begin to transfer the file. Start the congestion window and timeouts.
	  // Else retransmit SYN_ACK
	    if (recvlen < 0){ // TIME-OUT. Retransmit SYN_ACK
	    resend_fin_ack:
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
	      ackArr[LastByteSent+1].isRetrans=true;
	      cout << "Sending packet " << syn_ack_packet.getSeqNumber() << " " << cwnd << " " << ssthresh << " Retransmission SYN"  << endl;
	      break;
	    } 
	    TCPPacket recv_packet = TCPPacket(buf, recvlen);  
	    cout << "Receiving packet " << recv_packet.getAckNumber() << endl;
	    //CHECK IF ACK IS CORRECT
	    if ( recv_packet.getACK() && !recv_packet.getSYN() && !recv_packet.getFIN() ) {
	      LastByteSent = (LastByteSent+1)%MAX_SEQ_NUM;
	    CLIENT_SEQ_NUM = recv_packet.getSeqNumber();
	    CLIENT_WINDOW = recv_packet.getWindowSize();
	    LastByteAcked = recv_packet.getAckNumber();
	    STATE = FILE_TRANSFER;
	    if (!ackArr[LastByteAcked].isRetrans) {
	      updateRTO(ackArr[LastByteAcked].RTTtimer.getTimeMicro());
	    }
	    ackArr[LastByteAcked].RTTtimer.stop();
	    if (!FILE_TRANSFER_INIT){

	      cout << "Finding File..." << endl;
	      FILE *fd = fopen("large.txt", "rb");
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
	      //now we have to poll for everything.
	      TIME_OUT = 1000;
	    }
	    cout << "File Size : " << file_len << endl;

	    goto send;
	    }
	    else {
	      //problem, wrong ack or incorrect statements, resending FIN-ACK
	      goto resend_fin_ack;
	    }
	  break;
	  }
        case FILE_TRANSFER:
	  {
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

	    if ( recvlen < 0 )  {
	      timer * tim = findClosestTimer(LastByteAcked,LastByteSent);
	      if (tim != NULL) {
		if ( tim->hasFired()) {
		  cout << "TIMEOUT" << endl;
		  ssthresh = cwnd/2;
		  cwnd_STATE = SLOW_START;
		  cwnd = 1024;
		  
		  //Change window to front
		  int num = (LastByteSent >= LastByteAcked)?LastByteSent:(int)((int)MAX_SEQ_NUM+(int)LastByteSent);
		  for (int i = LastByteAcked;  i <= num; i++) {
		    ackArr[(i%MAX_SEQ_NUM)].isRetrans=true;
		    ackArr[(i%MAX_SEQ_NUM)].RTTtimer.stop();
		  }
		  int backTrack = 0;
		  if (LastByteSent < LastByteAcked){
		    cout << "Reversing order, we overflowed" << endl;
		    backTrack = MAX_SEQ_NUM - LastByteAcked + LastByteSent;
		  }
		  else {
		    backTrack = LastByteSent-LastByteAcked;
		  }
		  if (hasPassedMaxBytes) {
		    MaxBytesSent = bytes_sent+1;
		    cout << "Setting MaxBytesSent: " << MaxBytesSent << endl;
		    hasPassedMaxBytes = false;
		  }
		  LastByteSent = LastByteAcked;
		  bytes_sent -= backTrack;
		  cout << "After change: LastByteSent: " << LastByteSent << " LastByteAcked:  " << LastByteAcked << " " << bytes_sent << " " << backTrack << endl;
		  goto send;
		}
		else {break;}
	      } else {cout << "Found no timer" << endl; break;}
	    }
	  else{
	      TCPPacket recv_packet = TCPPacket(buf, recvlen);  
	      cout << "Receiving packet " << recv_packet.getAckNumber() << endl;
	      if ( recv_packet.getACK() && !recv_packet.getSYN() && 
		   !recv_packet.getFIN() ){
		CLIENT_SEQ_NUM = recv_packet.getSeqNumber();
		CLIENT_WINDOW = recv_packet.getWindowSize();
	      }
	      uint16 inc = 1;
	      if (recv_packet.getAckNumber() > LastByteAcked) inc = recv_packet.getAckNumber() - LastByteAcked;
	      LastByteAcked =  recv_packet.getAckNumber();
	      
	      if (!hasPassedMaxBytes) {
		cout << "Trying to do SOMETHING!!! With MaxBytesSent: " << MaxBytesSent << " LastByteSent " << LastByteSent << " LastAckSent " <<  LastByteAcked << endl;
		if (MaxBytesSent%MAX_SEQ_NUM > LastByteSent) {
		  if ((LastByteAcked > LastByteSent) && (LastByteAcked <= MaxBytesSent%MAX_SEQ_NUM)) {
		    bytes_sent+=LastByteAcked-LastByteSent;
		    LastByteSent = LastByteAcked;
		    cout << "Now: LastByteSent: " << LastByteSent << " LastByteAcked " << LastByteAcked << endl; 
		  }
		}
		else if (MaxBytesSent%MAX_SEQ_NUM < LastByteSent) {
		  if ((LastByteAcked <= MaxBytesSent%MAX_SEQ_NUM)) {
		    bytes_sent+=(LastByteAcked + (MAX_SEQ_NUM - LastByteSent));
		    LastByteSent=LastByteAcked;
		  }
		  else if (LastByteAcked > LastByteSent) {
		    bytes_sent+=LastByteAcked-LastByteSent;
		    LastByteSent = LastByteAcked;
		  }
		}
	      }
	      if (ackArr[LastByteAcked].RTTtimer.isRunning()) {
		if (!ackArr[LastByteAcked].isRetrans) {
		  updateRTO(ackArr[LastByteAcked].RTTtimer.getTimeMicro());
		}
		else {
		  ackArr[LastByteAcked].isRetrans = false;
		}
	      }
	      ackArr[LastByteAcked].RTTtimer.stop();
	      if (cwnd_STATE == SLOW_START) {
		if ((cwnd + MAX_BODY_LEN) >= ssthresh) {
		  cwnd_STATE = CONG_AVOID;
		}
		else {
		  cwnd+=MAX_BODY_LEN;
		  cout << "Slow Start, Cwnd is now: " << cwnd << endl;
		}
	      }
	      if (cwnd_STATE == CONG_AVOID) {
		cwnd+=ceil((inc * MAX_BODY_LEN)/((double)cwnd));
		cout << "CONG_AVOID, Cwnd is now: " << cwnd << endl;
	      }
	    }
	  send:
	  //Send what we need
	  
	    uint16 bytes_to_send = get_bytes_to_send(LastByteSent,LastByteAcked,cwnd,CLIENT_WINDOW);
	    //START RTT TIMER in sendPackets
	    uint16 _bytes_sent= sendPackets(bytes_to_send ,LastByteSent,cwnd,ssthresh,client_addr,hasPassedMaxBytes,MaxBytesSent);
	    if (_bytes_sent < 0) {
	    }
	    else {
	      LastByteSent= (LastByteSent+_bytes_sent)%MAX_SEQ_NUM;
	      bytes_sent+=_bytes_sent;
	      if (bytes_sent >= MaxBytesSent-1) {
		hasPassedMaxBytes = true;
	      }
	      cout << "I am now at LastByteSent: " << LastByteSent << " and LastByteAcked: " << LastByteAcked << endl;
	    }
	    if ((file_len == bytes_sent) && (LastByteSent==LastByteAcked)) {
	      //Send first FIN, change state to FIN_SENT
	      cout << "File Transmission Done!" << endl;
	      bitset<3> flags = bitset<3>(0x0);
	      flags.set(FININDEX,1);
	      TCPPacket fin_packet = TCPPacket(LastByteSent, 0, cwnd, flags,NULL,0);
	      unsigned char sendbuf[MAX_PACKET_LEN];
	      fin_packet.encode(sendbuf);
	      int send_status = sendto(socketfd, sendbuf, sizeof(unsigned char)*fin_packet.getLengthOfEncoding(), 0,(struct sockaddr *)&client_addr, len);
	      if (send_status < 0){
		cerr << "Error Sending Packet...\nServer Closing..." << endl;
		exit(1);
	      }
	      cout << "Initiating Closing of Connection" << endl;
	      cout << "Sending packet " << fin_packet.getSeqNumber() << " " << cwnd << " " << ssthresh << " FIN"  << endl;

	      RTO = 2 * RTO;
	      TIME_OUT = 2*RTO;
	      STATE=FIN_SENT;
	    }
	    
	    break;
	  }
      case FIN_SENT:
	{
	  if (recvlen < 0){
	    // Send FIN again
	  resend_fin:
	    bitset<3> flags = bitset<3>(0x0);
	    flags.set(FININDEX,1);
	    TCPPacket fin_packet = TCPPacket(LastByteSent, 0, cwnd, flags,NULL,0);
	    unsigned char sendbuf[MAX_PACKET_LEN];
	    fin_packet.encode(sendbuf);
	    int send_status = sendto(socketfd, sendbuf, sizeof(unsigned char)*fin_packet.getLengthOfEncoding(), 0,(struct sockaddr *)&client_addr, len);
	    if (send_status < 0){
	      cerr << "Error Sending Packet...\nServer Closing..." << endl;
	      exit(1);
	    }
	    cout << "Sending packet " << fin_packet.getSeqNumber() << " " << cwnd << " " << ssthresh << " Retransmission FIN"  << endl;
	    break;
	  }
	    TCPPacket recv_packet = TCPPacket(buf, recvlen);  
	    cout << "Receiving packet " << recv_packet.getAckNumber() << endl;
	    //check correct seq num
	  if ( recv_packet.getFIN() && recv_packet.getACK() && !recv_packet.getSYN() ){
	    LastByteSent = (LastByteSent+1)%MAX_SEQ_NUM;
	    CLIENT_SEQ_NUM = recv_packet.getSeqNumber();
	    CLIENT_WINDOW = recv_packet.getWindowSize();
	    LastByteAcked = recv_packet.getAckNumber();

	    STATE = FIN_WAITING;
	  }
	  else {
	    //retransmit 
	    goto resend_fin;
	  }
	  // Wait for FIN_ACK from the client, retransmit if neccisary. Once that is done, move to FIN_WAITING and wait for the next packet.
	  break;
	}
	  
        case FIN_WAITING:
	  {
	  //if we get the last FIN, send a FIN_ACK and then "close" the connection.
	    if ( recvlen < 0 ) {
	      //just loop?
	      break;
	    }
	  FIN_ACK:
	    TCPPacket recv_packet = TCPPacket(buf, recvlen);  
	    cout << "Receiving packet " << recv_packet.getAckNumber() << endl;
	  //CHECK IF ACK IS CORRECT
	    if ( !recv_packet.getACK() && !recv_packet.getSYN() && recv_packet.getFIN() ){
	      CLIENT_SEQ_NUM = recv_packet.getSeqNumber();
	      CLIENT_WINDOW = recv_packet.getWindowSize();
	      //SEND FIN_ACK
	      bitset<3> flags = bitset<3>(0x0);
	      flags.set(FININDEX,1);
	      flags.set(ACKINDEX,1);
	      TCPPacket fin_ack_packet = TCPPacket(LastByteSent, (CLIENT_SEQ_NUM+1)%MAX_SEQ_NUM, cwnd, flags,NULL,0);
	      unsigned char sendbuf[MAX_PACKET_LEN];
	      fin_ack_packet.encode(sendbuf);
	      int send_status = sendto(socketfd, sendbuf, sizeof(unsigned char)*fin_ack_packet.getLengthOfEncoding(), 0,(struct sockaddr *)&client_addr, len);
	      if (send_status < 0){
		cerr << "Error Sending Packet...\nServer Closing..." << endl;
		exit(1);
	      }
	      cout << "Sending packet " << fin_ack_packet.getSeqNumber() << " " << cwnd << " " << ssthresh << " FIN"  << endl;
	      if (STATE != TIMED_WAIT) 
		TIME_OUT*=2; // this happens anyway, at each timeout we need to wait twice the time.
	      
	      STATE = TIMED_WAIT;
	    }
	    else {
	    }
	  }	  
	  break;
      case TIMED_WAIT:
	{
	  if (recvlen < 0){
	    STATE = LISTEN;
	    Connection = 0;
	    cout << "Connection Closed...\nListening..." << endl;
	      // Listen for UDP Packets && Implement TCP 3 Way Handshake With States
	    CLIENT_SEQ_NUM = 0;
	    CLIENT_WINDOW=0;
	    cwnd = MAX_BODY_LEN;
	    ssthresh = 15360;
	    LastByteSent = 0;
	    LastByteAcked = 0;
	    TIME_OUT           = 500000;    // Retransmission Timeout: 500 ms 

	    
	    FILE_TRANSFER_INIT = 0;         // Was the file to be sent transferred
	    isInitRTT = true;




	    SRTT = 3000000;
	    DevRTT = 3000000;
	    RTO =  SRTT + 4*DevRTT;
	    file_len = 0;
	    bytes_sent = 0;
	    cwnd_STATE = SLOW_START;

	    break;
	  }else{
	    goto FIN_ACK;
	  }
	}
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


uint16 sendPackets(uint16 bytesToSend, uint16 LastByteSent, uint16 cwnd, uint16 ssthresh, struct sockaddr_in c_addr,bool hasPassedMaxBytes, long long int MaxBytesPassed){
  struct sockaddr_in client_addr = c_addr;
  socklen_t len = sizeof(client_addr);
  if (bytes_sent >= file_len) // File Transfer Complete
    return 0;
  uint16 bytesSentNow = 0;
  bytesToSend = min((long long)bytesToSend, (file_len-bytes_sent)); // Maximum amount of file left to send
  while( bytesSentNow < bytesToSend ) {
    bitset<3> flags = bitset<3>(0x0);	      
    uint16 fl = min((uint16)(bytesToSend-bytesSentNow), MAX_BODY_LEN);
    if(fl == 0) // File Transfer Complete
      return bytesSentNow;
    TCPPacket packet = TCPPacket(LastByteSent+bytesSentNow, 0, cwnd, flags, file_buf+bytes_sent+bytesSentNow,fl);

    unsigned char sendbuf[MAX_PACKET_LEN];
    packet.encode(sendbuf);

    string s = (ackArr[(LastByteSent+bytesSentNow+fl)%MAX_SEQ_NUM].isRetrans && !hasPassedMaxBytes)?" Retransmission":" ";
    cout << "Sending packet " << packet.getSeqNumber() <<" " << cwnd << " " << ssthresh << s << endl;
    
    int send_status = sendto(socketfd, sendbuf, sizeof(unsigned char)*packet.getLengthOfEncoding(), 0,(struct sockaddr *)&client_addr, len);
    if (send_status < 0){
      cerr << "Error Sending Packet...\nServer Closing..." << endl;
      return -1;
    }
    bytesSentNow+=fl;
    cout << "Starting timer with this RTO: " << RTO << " And this seq num: " << (LastByteSent+bytesSentNow)%MAX_SEQ_NUM << endl;
    ackArr[(LastByteSent+bytesSentNow)%MAX_SEQ_NUM].RTTtimer.start(RTO);
    if (hasPassedMaxBytes || (bytes_sent+bytesSentNow > MaxBytesPassed-1)) {
      ackArr[(LastByteSent+bytesSentNow)%MAX_SEQ_NUM].isRetrans=false;
      hasPassedMaxBytes=true;
    }
    gettimeofday(&packet_TO, NULL);
  }
  return bytesSentNow;
}


void updateRTO(double RTT) {
  if (isInitRTT){
    SRTT = RTT;
    DevRTT = SRTT/2;
    isInitRTT = false;
  }
  else {
    long long diff = RTT-SRTT;
    SRTT = SRTT + alpha * diff;
    DevRTT = DevRTT + beta*(abs(diff) - DevRTT);
  }
  RTO = SRTT + 4 * DevRTT; 

  cout << "UPDATING RTO: " << RTO << " The RTT I got in microseconds: " << RTT << endl;
}
timer * findClosestTimer(uint16 LastByteAcked, uint16 LastByteSent) {
  int num = (LastByteSent >= LastByteAcked)?LastByteSent:((int)MAX_SEQ_NUM+(int)LastByteSent);
  for (int i = LastByteAcked;  i <= num; i++) {
    if (ackArr[(i%MAX_SEQ_NUM)].RTTtimer.isRunning()) {
      return &ackArr[(i%MAX_SEQ_NUM)].RTTtimer;
    }
  }
  return 0x0;
}

