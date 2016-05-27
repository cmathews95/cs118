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

using namespace std;

int MAX_PACKET_SIZE = 1032; 
int Connection = 0;
int socketfd;

// Simple State Abstraction to Implement TCP
enum States { CLOSED, LISTEN, SYN_RECV, ESTAB };

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
  while(1) {
    cout << "Listening for UDP Packets..." << endl;
    char* buf[MAX_PACKET_SIZE];
    struct sockaddr_in client_addr;
    int len = sizeof(client_addr);
    int recvlen = recvfrom(socketfd, buf, MAX_PACKET_SIZE, 0, (struct sockaddr *)&client_addr, (socklen_t *)&len);
    STATE = LISTEN;
    if (recvlen >= 0) {
      cout << "UDP PACKET RECEIVED..." << endl;
      // Check for 3 Way Handshake/Packet over existing Connection
      switch (STATE) {
        case CLOSED: 
	  break;
        case LISTEN:
	  // If SYN Received, send SYN-ACK, Change State to SYN_RECV
	  break;
        case SYN_RECV:
	  // If ACK Received, Change State to ESTAB,
	  // Need to check for Request
	  break;
        case ESTAB:
	  // Deal with Request
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
