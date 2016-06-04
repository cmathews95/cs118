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
#include <time.h>

#include "TCPPacket.h"
using namespace std;


const uint16_t MAX_PACKET_LEN = 1032;
const uint16_t MAX_SEQUENCE_NUM = 30720; // 30KBYTES. Reset seq number if it reaches this val
const uint16_t INITIAL_CONGESTION_WINDOW = 1024;
const uint16_t BUFF_SIZE = 1033; // One extra bit to null terminate.
const uint16_t INITIAL_SLOWSTART_THRESH = 30720;
// const uint16_t RETRANSMISSION_TIMEOUT = 500000; // 500 milliseconds
const int RETRANSMISSION_TIMEOUT = 500000; // 500 milliseconds
const uint16_t RECEIVER_WINDOW = 30720; 

int socketfd;
int Connection = 0;

// Simple State Abstraction to Implement TCP
enum States { CLOSED, SYN_SENT, ESTAB, FIN };

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
  ofstream file("ReceivedFile",ofstream::out);
  vector<unsigned char> receivedData;

  /*  // Variables for timeout
    */
  // Need a variable to end the loop (Once the FIN protocol has finished
  bool endLoop = 0;
  // Initiate 3 Way Handshake & Send Request
  int init_counter = 0;
  while(!endLoop) {
    switch(STATE){
      case CLOSED:
	{
	// Send SYN and Change STATE to SYN_SENT
	
	// TCPPacket constructor arguments: seqnum, acknum, windowsize, flags, body, bodylen
	next_byte_expected = 0;
	// Flags indices go ACK, SYN, FIN (from TCPPacket.h)
	flags = bitset<3>(0x0);
	flags.set(SYNINDEX,1);
	syn_packet = TCPPacket(sequence_num, next_byte_expected, RECEIVER_WINDOW, flags, NULL, 0);
	syn_packet.encode(sendBuf);
	
	//TODO Insert TIMEOUT
	cout << "Sending syn request with sequence number " << sequence_num << endl;
	send_status = sendto(socketfd, sendBuf, sizeof(unsigned char) * syn_packet.getLengthOfEncoding(), 0, (struct sockaddr *) &serverAddr, from_len);

	if(send_status < 0)
	  {
	    cout << "Could not send fin, exiting" << endl;
	    exit(1);
	    break;
	  }
	sequence_num = (sequence_num + 1) % MAX_SEQUENCE_NUM;

	STATE = SYN_SENT;
	break;
	}

      case SYN_SENT:
	{
	// Check if you received a SYN-ACK 
	// If so, send ACK + Req and Change STATE to ESTAB
	memset(buf, '\0', BUFF_SIZE);
	//**KN**

	struct timeval time_out;
	time_out.tv_sec = 0;
	time_out.tv_usec = RETRANSMISSION_TIMEOUT;
	if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,&time_out,sizeof(time_out)) < 0) {
	  perror("Error");
	}
	
	recv_status=recvfrom(socketfd, buf, sizeof(unsigned char) * BUFF_SIZE, 0,(struct sockaddr *) &serverAddr, & from_len);
	if(recv_status<0)
	  {
	    init_counter++;
	    if (init_counter ==3) {
	      cerr << "Error: Failed to receive Response from Server (Timed out). Exiting" << endl;
	      file.close();
	      exit(2);
	    }
	    
	    STATE = CLOSED;
	    break; // Breaks out of our switch statement. While loop loops again and we restart in the closed state.
	    //file.close();
	    //exit(4);
	  }
	
	recv_packet=TCPPacket(buf, recv_status);
	cout << "Receiving data packet " << recv_packet.getSeqNumber() << endl;
	    


	cout << "Received syn-ack packet " << recv_packet.getSeqNumber() << endl;
       
	next_byte_expected = (recv_packet.getSeqNumber()+ recv_packet.getBodyLength()+1) % MAX_SEQUENCE_NUM;
	
	// Sending the ack+req
	flags = bitset<3>(0x0);
	flags.set(ACKINDEX,1);
	ack_packet = TCPPacket(0, next_byte_expected, RECEIVER_WINDOW, flags, NULL, 0);
    
	ack_packet.encode(sendBuf);

	cout << "Sending ACK packet " << next_byte_expected << endl;
        send_status = sendto(socketfd, sendBuf, sizeof(unsigned char) * ack_packet.getLengthOfEncoding(), 0,(struct sockaddr *) &serverAddr, from_len);

        if(send_status < 0)
          {
            cerr << "Error, failed to send ack." << endl;
            //free(sendBuf);
	    file.close();
	    exit(5);
	    
          }
        //sequence_num = (sequence_num + 1) % MAX_SEQUENCE_NUM;
	
	// After sending out ack+req, change state to ESTAB
	time_out.tv_sec = 0;
	time_out.tv_usec = 0;
	if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,&time_out,sizeof(time_out)) < 0) {
	  perror("Error");
	}
	STATE = ESTAB;
	break;
	}


      case ESTAB:
	{
	// Receiving packets
	// Outputting to files
	// If receive fin, send fin-ack
	// Else, send regular ack.

	  cout << "Listening for Data..." << endl;

	  recv_status=recvfrom(socketfd, buf, sizeof(unsigned char) * BUFF_SIZE, 0, (struct sockaddr *) &serverAddr, & from_len);
	  if(recv_status<0)
	    {
	      cerr << "Error: Corrupted Packet" << endl;
	      break;
	    }
	  recv_packet=TCPPacket(buf, recv_status);
	  	  
	  cout << "Receiving data packet " << recv_packet.getSeqNumber() <<"with body length: " << recv_packet.getBodyLength() <<  endl;
	  
	
	  // Once we exit this loop, we have just received a valid packet.
	  if (recv_packet.getSeqNumber() == next_byte_expected) {
	    next_byte_expected = (next_byte_expected + recv_packet.getBodyLength()) % MAX_SEQUENCE_NUM;

	    if (recv_packet.getBodyLength() > 0) {
	      //SAVE OUR STUFF TO THE FILE
	      recv_packet.getBody()[recv_packet.getBodyLength()]='\0';
	      file << recv_packet.getBody();
	      file.flush();
	    }
	  }
	  
	  
	  
	  
	  //##############TODO#################//
	// Send the appropriate ack
	// If fin, send fin-ack. Else, send regular ack.
	if(recv_packet.getFIN())
	  {
	  sending_fin_ack: {
	    cout << "Received fin, replying with a fin-ack." << endl;
	    flags = bitset<3>(0x0);
	    flags.set(FININDEX,1);
	    flags.set(ACKINDEX,1);
	    TCPPacket fa_packet = TCPPacket(sequence_num, next_byte_expected,RECEIVER_WINDOW,flags,NULL,0);
	    
	    fa_packet.encode(sendBuf);


	    send_status = sendto(socketfd, sendBuf, sizeof(unsigned char) * fa_packet.getLengthOfEncoding(), 0, (struct sockaddr *) &serverAddr, from_len);

	    if(send_status < 0)
	      {
		cerr << "Error, failed to send fin-ack." << endl;
		file.close();
		exit(7);
	      }
	    }
	  sending_fin: {
	    cout << "Sending FIN" << endl;
	    flags = bitset<3>(0x0);
	    flags.set(FININDEX,1);
	    if (STATE==ESTAB) sequence_num++;
	    TCPPacket fin_packet = TCPPacket(sequence_num, 0, RECEIVER_WINDOW,flags,NULL,0);
	    fin_packet.encode(sendBuf);

	    send_status = sendto(socketfd, sendBuf, sizeof(unsigned char) * fin_packet.getLengthOfEncoding(), 0, (struct sockaddr *)\
				 &serverAddr, from_len);

	    if(send_status<0)
	      {
		cerr<< "Error: Failed to send FIN" << endl;
		file.close();
		exit(9);
	      }
	    
	    
	    STATE = FIN;
	    break;
	    }
	  }
	
	// Else, just regularly ack it.
	else
	  {
	    flags = bitset<3>(0x0);
	    flags.set(ACKINDEX,1);
	    ack_packet = TCPPacket(sequence_num, next_byte_expected, RECEIVER_WINDOW,flags,NULL,0);
	    
	    ack_packet.encode(sendBuf);
	    cout << "Sending ACK packet " << next_byte_expected << endl;
	    send_status=sendto(socketfd, sendBuf, sizeof(unsigned char) * ack_packet.getLengthOfEncoding(), 0, (struct sockaddr *) &serverAddr, from_len);
	    
	    if(send_status<0)
	      {
		cerr << "Error, failed to ack a packet." << endl;
		//free(sendBuf);
		file.close();
		exit(8);
	      }
	    sequence_num = (sequence_num + ack_packet.getBodyLength()) % MAX_SEQUENCE_NUM;
	  }
	break;


	case FIN:
	  {
	    // In this state, you've sent your fin-ack and fin.
	    // Keep receiving. If you receive a fin packet or you time out, resend your fin.
	    // If you receive a fin-ack, close the connection.

	    // ##############TODO#################
	    // Also have to update the timeout time.
	    
	    struct timeval time_out;
	    time_out.tv_sec = 1;
	    time_out.tv_usec = 0;
	    if (setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,&time_out,sizeof(time_out)) < 0) {
	      perror("Error Here");
	    }
	    
	    recv_status=recvfrom(socketfd, buf, sizeof(unsigned char) * BUFF_SIZE, 0, (struct sockaddr *) &serverAddr, & from_len);
            
	    bool resendFin = false;
	    if(recv_status<0)
              {
		cout << "I seem to have missed the fin-ack, resending the fin" << endl;
		// Resend fin
		goto sending_fin; 
              }
	    // You've received stuff
            recv_packet=TCPPacket(buf, recv_status);
      
	    if (( recv_packet.getFIN() && !recv_packet.getACK()) || resendFin ) // If you receive a fin, that means your fin-ack got dropped
	      {
		cout << "Resending FIN-ACK" << endl;
		goto sending_fin_ack;
	      }

	    else if ( recv_packet.getFIN() && recv_packet.getACK()) // Received a fin-ack to your fin.
	      {
		// At this point, you're done.
		endLoop = true;
		STATE=CLOSED;
		break;
	      }
	    else {
	       // Else, received some other random packet that you don't want, so just loop again.
	      goto sending_fin;
	    }
	     
	      break;
	  }
	}
    }
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
