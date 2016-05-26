#include <sys/types.h>
#include <sys/socket.h>
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


string dns(const char* hostname, const char* port);

int main(int argc, char* argv[]) {
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
  
  int socketfd;
  
  if ( (socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
      cerr << "Error Creating Socket...\nServer Closing..." << endl;
      exit(1);
  }
  
  
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
  
  while(1) {
    cout << "Listening for UDP Packets..." << endl;
    char* buf[MAX_PACKET_SIZE];
    struct sockaddr_in client_addr;
    int len = sizeof(client_addr);
    int recvlen = recvfrom(socketfd, buf, MAX_PACKET_SIZE, 0, (struct sockaddr *)&client_addr, &len));
    if (recvlen >= 0) {
      cout << "UDP PACKET RECEIVED..." << endl;
    }
    
    close(socketfd);
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
