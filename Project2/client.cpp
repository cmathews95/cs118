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
using namespace std;

const int BUFF_SIZE = 4096;
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
  if(argc <= 1) {
    cerr << "Not Enough Arguments...Pass in URL" << endl;
    return 1;
  }

  // Parse URL
  const char * host;
  const char * port;
  const char * path;
  const char * ipaddress;
  try {
    connection_info* req_data = urlScraper(argv[1]);
    if (req_data == NULL) {
      cerr << "Logic Error...\nClient Closing" << endl;
      exit(1);
    }
    cout << "Host Name: " <<  req_data->hostname << endl;
    cout << "Port: " <<  req_data->port << endl;
    cout << "Path: " <<  req_data->obj_path << endl;
    // Host & Port & path to CStrings
    host = req_data->hostname.c_str();
    port = req_data->port.c_str();
    path = req_data->obj_path.c_str();
    
    ipaddress = dns(host, port).c_str();
  }
  catch (...) {
    cerr << "Improperly formatted request" << endl;
    return 1;
  }

    
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

  // Setup to Send Packets to Server
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(port));
  serverAddr.sin_addr.s_addr = inet_addr(ipaddress);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
  
  // Initiate 3 Way Handshake & Send Request
  while(1) {
    switch(STATE){
      case CLOSED:
	// Send SYN and Change STATE to SYN_SENT
        break;
      case SYN_SENT:
	// Check if you received a SYN-ACK 
	// If so, send ACK + Req and Change STATE to ESTAB
	break;
      case ESTAB:
	// End...Only 1 Request per program execution
	break;
    }
  }

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
  hints.ai_socktype = SOCK_STREAM;
  
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
}
