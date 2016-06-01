#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <netdb.h>
#include <netinet/in.h>
#include <vector>
#include <fstream>

#include "TCPPacket.h"
using namespace std;


const uint16_t MAX_PACKET_LEN = 1032;
const uint16_t MAX_SEQUENCE_NUM = 30720; // 30KBYTES. Reset seq number if it reaches this val
const uint16_t INITIAL_CONGESTION_WINDOW = 1024;
const uint16_t BUFF_SIZE = 1033; // One extra bit to null terminate.
const uint16_t INITIAL_SLOWSTART_THRESH = 30720;
const uint16_t RETRANSMISSION_TIMEOUT = 500; // milliseconds
const uint16_t RECEIVER_WINDOW = 30720; 

int socketfd;
int Connection = 0;

// Simple State Abstraction to Implement TCP
enum States { CLOSED, SYN_SENT, ESTAB };

// Current State
States STATE = CLOSED;

// Struct for urlScraper Function
struct connection_info{
  string hostname;
  string port;
  string obj_path;
};

// Scrape Command Line Arguments and return struct
connection_info* urlScraper(char* url);
// Catch Singlas: If ^C, exit gracefully by closing socket
void signalHandler(int signal);
// Resole Hostname into IP (Simple DNS Lookup)
string dns(const char* hostname, const char* port);

int main(int argc, char* argv[]){
  if (signal(SIGINT, signalHandler) == SIG_ERR)
    cerr << "Can't Catch Signal..." << endl;

  // Check Arguments
  if(argc != 3) {
    cerr << "Error: incorrect number of arguments. Requires SERVER-HOST-OR-IP and PORT-NUMBER" << endl;
    return 1;
  }

  // Parse URL
  const char * host = argv[1];
  const char * port = argv[2];
  const char * ipaddress = dns(host,port).c_str();

    
  cout << "=====================================+=====================" << 
  endl;
  cout << "Connecting to Server with Hostname: " << host << " | Port: " << 
  port << endl;
  cout << "IP ADDRESS: " << ipaddress << endl;

  // Create UDP Socket
  if( (socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ){
    cerr << "Error Creating Socket...\nClient Closing..." << endl;
    exit(2);
  } 
  Connection = 1;

  // Generating a random sequence number to start with
  srand(time(NULL));
  uint16_t sequence_num = rand() % MAX_SEQUENCE_NUM;
  uint16_t next_byte_expected;

  // Setup to Send Packets to Server
  struct sockaddr_in serverAddr;
  socklen_t from_len = sizeof(serverAddr);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(port));
  serverAddr.sin_addr.s_addr = inet_addr(ipaddress);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
  cout << "Initiating Three Way Handshake..." << endl;
  
  // Variables to be used in switch (Was getting "crosses initialization" error)
  bitset<3> flags = bitset<3>(string("000"));
  TCPPacket syn_packet = TCPPacket(0,0,0,flags,NULL,0);
  TCPPacket ack_packet = TCPPacket(0,0,0,flags,NULL,0);
  TCPPacket recv_packet = TCPPacket(0,0,0,flags,NULL,0);
  

  //  unsigned char* sendBuf = malloc(sizeof(unsigned char) * BUFF_SIZE);
  unsigned char sendBuf[BUFF_SIZE];
  int send_status, recv_status; 
  //unsigned char* buf = malloc(sizeof(unsigned char) * BUFF_SIZE);
  unsigned char buf[BUFF_SIZE];
  ofstream file("ReceivedFile");
  vector<unsigned char> receivedData;
  


  //

  // Initiate 3 Way Handshake & Send Request
  while(1) {
    switch(STATE){
      case CLOSED:
	{
	// Send SYN and Change STATE to SYN_SENT
	
	// TCPPacket constructor arguments: seqnum, acknum, windowsize, flags, body, bodylen
	next_byte_expected = 0;
	// Flags indices go ACK, SYN, FIN (from TCPPacket.h)
	flags = bitset<3>(string("010"));
	syn_packet = TCPPacket(sequence_num, next_byte_expected, RECEIVER_WINDOW, flags, NULL, 0);
	syn_packet.encode(sendBuf);
	

	cout << "Sending syn request with sequence number " << sequence_num << endl;
	send_status = sendto(socketfd, sendBuf, sizeof(unsigned char) * syn_packet.getLengthOfEncoding(), 0, (struct sockaddr *) &serverAddr, from_len);

	if(send_status < 0)
	  {
	    cerr << "Error, failed to send syn." << endl;
	    //free(sendBuf);
	    exit(3);
	  }
	sequence_num = (sequence_num + 1) % MAX_SEQUENCE_NUM;

	STATE = SYN_SENT;
	//	free(sendBuf);
	break;
	}

      case SYN_SENT:
	{
	// Check if you received a SYN-ACK 
	// If so, send ACK + Req and Change STATE to ESTAB
	memset(buf, '\0', BUFF_SIZE);
	//**KN**
	//TODO PLEASE TIMEOUT IF SYN_ACK IS NOT RECEIEVED. DO NOT JUST LOOP.
	do
	  {
	    recv_status=recvfrom(socketfd, buf, sizeof(unsigned char) * BUFF_SIZE, 0,(struct sockaddr *) &serverAddr, & from_len);
	    if(recv_status<0)
	      {
		cerr << "Error: Failed to receive syn-ack" << endl;
		exit(4);
	      }

	    recv_packet=TCPPacket(buf, recv_status);
	    cout << "Receiving data packet " << recv_packet.getSeqNumber() << endl;
	  } while(!(recv_packet.getACK() && recv_packet.getSYN()));
	cout << "Received syn-ack packet " << recv_packet.getSeqNumber() << endl;
       
	next_byte_expected = (recv_packet.getSeqNumber()+ recv_packet.getBodyLength()+ 1) % MAX_SEQUENCE_NUM;
	
	// Sending the ack+req
	flags = bitset<3>(0x0);
	flags.set(ACKINDEX,1);
	ack_packet = TCPPacket(sequence_num, next_byte_expected, RECEIVER_WINDOW, flags, NULL, 0);
	
	ack_packet.encode(sendBuf);

	cout << "Sending ACK packet " << next_byte_expected << endl;
        send_status = sendto(socketfd, sendBuf, sizeof(unsigned char) * ack_packet.getLengthOfEncoding(), 0,(struct sockaddr *) &serverAddr, from_len);

        if(send_status < 0)
          {
            cerr << "Error, failed to send ack." << endl;
            //free(sendBuf);
	    exit(5);
          }
        sequence_num = (sequence_num + 1) % MAX_SEQUENCE_NUM;
	
	// After sending out ack+req, change state to ESTAB
	STATE = ESTAB;
	//        free(sendBuf);
	break;
	}


      case ESTAB:
	{
	// Receiving packets
	// Outputting to files
	// If receive fin, send fin-ack
	// Else, send regular ack.

	  //##################### INVALID LOGIC FOR ENDING THIS LOOP #####################//
	  cout << "Listening for Data..." << endl;
	  do
          {
            recv_status=recvfrom(socketfd, buf, sizeof(unsigned char) * BUFF_SIZE, 0, (struct sockaddr *) &serverAddr, & from_len);
            if(recv_status<0)
              {
                cerr << "Error: Failed to receive file" << endl;
                exit(6);
              }
            recv_packet=TCPPacket(buf, recv_status);
	    
	    // Add the packet's data contents to a vector, which we can later write to a file.                                       
	    receivedData.insert(receivedData.end(), &buf[8], &buf[recv_packet.getLengthOfEncoding()-1]);

	    cout << "Receiving data packet " << recv_packet.getSeqNumber() << endl;

          } while(!(true)); // Replace 'true' with a check for whether the packet is invalid
	  // Once we exit this loop, we have just received a valid packet.
	  if (recv_packet.getSeqNumber() == next_byte_expected) {
	    next_byte_expected = (next_byte_expected+ recv_packet.getBodyLength()+ 1) % MAX_SEQUENCE_NUM;

	    
	    //SAVE OUR STUFF TO THE FILE
	  }
	  
	  
	  
	  
	  //##############TODO#################//
	// Send the appropriate ack
	// If fin, send fin-ack. Else, send regular ack.
	if(recv_packet.getFIN())
	  {
	    cout << "Received fin, replying with a fin-ack." << endl;
	    flags = bitset<3>(string("101"));

	    TCPPacket fa_packet = TCPPacket(sequence_num, next_byte_expected,RECEIVER_WINDOW,flags,NULL,0);
	    
	    fa_packet.encode(sendBuf);


	    send_status = sendto(socketfd, sendBuf, sizeof(unsigned char) * fa_packet.getLengthOfEncoding(), 0, (struct sockaddr *) &serverAddr, from_len);

	    if(send_status < 0)
	      {
		cerr << "Error, failed to send fin-ack." << endl;
		//free(sendBuf);                                                                            
		exit(7);
	      }
	    // WE ALSO NEED TO SEND A FIN AND WAIT FOR FIN-ACK, do this next.



	    //AFTER WE DO THAT, ASSEMBLE THE FILE AND SAVE IT.

	    
	  }
	
	// Else, just regularly ack it.
	else
	  {
	    flags = bitset<3>(string("100"));
	    ack_packet = TCPPacket(sequence_num, next_byte_expected, RECEIVER_WINDOW,flags,NULL,0);
	    
	    ack_packet.encode(sendBuf);
	    //	    sendBuf[fa_packet.getLengthOfEncoding()] = '\0';
	    cout << "Sending ACK packet " << next_byte_expected << endl;
	    send_status=sendto(socketfd, sendBuf, sizeof(unsigned char) * ack_packet.getLengthOfEncoding(), 0, (struct sockaddr *) &serverAddr, from_len);
	    
	    if(send_status<0)
	      {
		cerr << "Error, failed to ack a packet." << endl;
		//free(sendBuf);
		
		exit(8);
	      }
	    sequence_num = (sequence_num + ack_packet.getBodyLength()) % MAX_SEQUENCE_NUM;
	  }
	break;
	}
    }
  }


  // Write the vector we've been maintaining to a file.
  for(vector<unsigned char>::size_type i = 0; i != receivedData.size(); i++) 
    {  
      file << receivedData[i];
    }

  file.close();
  close(socketfd);
  Connection = 0;
  return 0;
}

connection_info* urlScraper(char* url) {
  if(strlen(url) <=7)
    return NULL;

  string urlS = string(url, strlen(url));
  connection_info* temp = new connection_info();
  temp->hostname = "";
  temp->port = "";
  temp->obj_path = "";
  int i = 7;
  while (url[i]!='/' && url[i]!='\0'){
    temp->hostname += url[i];
    ++i;
  }
  while (i < strlen(url)){
    temp->obj_path += url[i];
    ++i;
  }
  i = 0;
  while (temp->hostname[i] != ':' && url[i]!='\0')i++;
  if (i+1 < strlen(url)){
    int x = i;
    i++;
    while (temp->hostname[i] != '\0'){
      temp->port += temp->hostname[i];
      i++;
    }
    temp->hostname.erase(x,temp->hostname.length());
  }
  if (temp->port == "")
    temp->port = "80";
  return temp;
}

string dns(const char* hostname, const char* port){
  int status = 0;
  struct addrinfo hints;
  struct addrinfo* res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  
  if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
    cerr << "getaddrinfo: " << gai_strerror(status) << endl;
    return NULL;
  }
  for (struct addrinfo* p = res; p != 0; p = p->ai_next) {
    struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;
    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    return string(ipstr);
  }
  return NULL;
}

void signalHandler(int signal){
  if(signal == SIGINT) {
    cerr << "\nReceived SIGINT...\nClient Closing..." << endl;
    if(Connection)
      close(socketfd);
    Connection = 0;
  }
  exit(0);
}
