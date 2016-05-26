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

  cout << "Listening for Connection..." << endl;
  
  int listenStatus;
  if ( (listenStatus = listen(socketfd,0)) < 0) {
    cerr << "Error Listening for Requests...\nServer Closing..." << endl;
    close(socketfd);
    exit(3);
  }
  
  while(1) {
    struct sockaddr_in client_address;
    socklen_t client_address_size = sizeof(client_address);
    int client_socketfd;
    if ( (client_socketfd = accept(socketfd, (struct sockaddr*)&client_address, &client_address_size)) < 0) {
      cerr << "Error Accepting Client Connection...\nServer Closing..." << endl;
      close(socketfd);
      close(client_socketfd);
      exit(4);
    }
    
    cout << "Connected to Client with socket: " << client_socketfd << endl;

    char client_ip[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(client_address.sin_family, &client_address.sin_addr, client_ip, sizeof(client_ip));
    cout << "Accepted Connection From Client:\n      IP: " << client_ip << "\n      Port: " << ntohs(client_address.sin_port) << endl;
    
    close(socketfd);
    close(client_socketfd);
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
