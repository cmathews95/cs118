#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <netdb.h>
#include "HttpRequest.h"
#include <netinet/in.h>
#include <vector>
#include "HttpMessage.h"
#include "HttpResponse.h"
#include <fstream>
const int BUFF_SIZE = 4096;

struct connection_info{
  std::string hostname;
  std::string port;
  std::string obj_path;
};

connection_info* urlScraper(char* url) {
  if(strlen(url) <=7)
    return NULL;

  std::string urlS = std::string(url, strlen(url));
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

int main(int argc, char* argv[]){
  // Check Arguments
  if(argc <= 1) {
    std::cerr << "Not Enough Arguments...Pass in URL" << std::endl;
    return 1;
  }
  std::cout << "Resource URL: " << argv[1] << std::endl;

  // Parse URL
  const char * host;
  const char * port;
  const char * path;
  const char * ipaddress;
  try {
    connection_info* req_data = urlScraper(argv[1]);
    if (req_data == NULL) {throw std::logic_error("ERROR");}
    std::cout << "Host Name: " <<  req_data->hostname << std::endl;
    std::cout << "Port: " <<  req_data->port << std::endl;
    std::cout << "Path: " <<  req_data->obj_path << std::endl;
    // Host & Port & path to CStrings
    host = req_data->hostname.c_str();
    port = req_data->port.c_str();
    path = req_data->obj_path.c_str();
    
    ipaddress = dns(host, port).c_str();
    std::cout << "IP ADDRESS: " << ipaddress << std::endl;
  }
  catch (...) {
    std::cerr << "Improperly formatted request" << std::endl;
    return 1;
  }
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  // Setup Connection to Server
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(port));
  serverAddr.sin_addr.s_addr = inet_addr(ipaddress);
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

  // Connection
  if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    perror("connect");
    return 2;
  }

  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
    perror("getsockname");
    return 3;
  }

  char ipstr[INET_ADDRSTRLEN] = {'\0'};
  inet_ntop(clientAddr.sin_family, &clientAddr.sin_addr, ipstr, sizeof(ipstr));
  std::cout << "Set up a connection from: " << ipstr << ":" <<
    ntohs(clientAddr.sin_port) << std::endl;
  
  // send/receive data to/from connection
  std::string input;
  std::stringstream ss;
  std::cout << "size matters" << std::endl;
  HttpRequest request(path, "GET");
  request.setHeaderField(HOST, host);
  request.setConnection(CLOSE);
  char* req  = request.encode();
  

  std::cout << "REQUEST: " << req << std::endl;
  
  if (send(sockfd, req, strlen(req), 0) == -1) {
    perror("send");
    return 4;
  }
  
  long size = sizeof(char)*2*BUFF_SIZE;
  long read_so_far = 0;
  char* buf = (char*)malloc(size);
  int read = 0;
  int rv = 0;
  while ((rv=recv(sockfd, buf+read_so_far, BUFF_SIZE, 0)) > 0) {
    read_so_far += rv;
    if (read_so_far+BUFF_SIZE > size) {
      size = size * size;
      buf = (char*)realloc(buf,size);
    }
  }
  if (rv == -1) {
    perror("recv");
    return 5;
  }

  // Turn reponse into vector<char>

  //Turn Vector into Response Object
  int goodResponse = 0;
  HttpResponse response(buf);
  std::cout << "What do I have here? : " << response.getBodyLength() << std::endl;
  if (response.getStatusCode().compare(OK)==0){
    int len = atoi(response.getHeaderField(CONTENT_LENGTH).c_str());
    std::cout << "LENGTH: " << len << std::endl;
    try{
      std::string file_name = "";
      int e = strlen(path)-1;
      while(path[e]!='/')e--;
      e++;
      while(path[e]!='\0'){
	file_name+=path[e];
	e++;
      }
      std::cout << "FILE NAME: " << file_name << std::endl;
      std::ofstream fs(file_name);
      fs << response.getBody();
      fs.flush();
      fs.close();
    }catch (...){
      std::cerr << "Error Creating/Saving File" << std::endl;
    }
  }else if(response.getStatusCode().compare(BAD_REQUEST)==0) {
    std::cerr << "Bad Request: " << response.getReasoning() << std::endl;
  }else{
    std::cerr << "Requested File Not Found" << std::endl;
  }

  free(buf);

  close(sockfd);
  return 0;
}
