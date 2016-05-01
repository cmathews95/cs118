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

char ipstr[INET_ADDRSTRLEN] = {'\0'};
struct connection_info{
  std::string hostname;
  std::string port;
  std::string object;
};

connection_info*:struct urlScraper(char* url) {
  int i = 0;
  connection_info* temp = new connection_info();
  while(url[i]!='\0') {

  }

  return temp;

}
int dns(char* hostname)
{
  struct addrinfo hints;
  struct addrinfo* res;

  // prepare hints
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP

  // get address
  int status = 0;
  if ((status = getaddrinfo(hostname, "80", &hints, &res)) != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
    return 0;
  }

  // std::cout << "IP addresses for " << hostname << ": " << std::endl;

  for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
    // convert address to IPv4 address
    struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;

    // convert the IP to a string and print it:
    inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    // std::cout << "  " << ipstr << std::endl;
    return ipstr;
  }

  freeaddrinfo(res); // free the linked list

  return 1;
}



int main(int argc, char *argv[])
{
  if(argc <= 1) {
    std::cerr << "Not Enough Arguments...Pass in URL" << std::endl;
    return 1;
  }

  std::cout << "Resource URL: " << argv[1] << std::endl;
  // create a socket using TCP IP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  for ( int i = 0; i < argc; i++ ) {
    connection_info* req_data = urlScraper(argv[i]);
  }
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(40000);     // short, network byte order
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

  // connect to the server
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
  bool isEnd = false;
  std::string input;
  char buf[20] = {0};
  std::stringstream ss;

  while (!isEnd) {
    memset(buf, '\0', sizeof(buf));

    std::cout << "send: ";
    std::cin >> input;
    if (send(sockfd, input.c_str(), input.size(), 0) == -1) {
      perror("send");
      return 4;
    }


    if (recv(sockfd, buf, 20, 0) == -1) {
      perror("recv");
      return 5;
    }
    ss << buf << std::endl;
    std::cout << "echo: ";
    std::cout << buf << std::endl;

    if (ss.str() == "close\n")
      break;

    ss.str("");
  }

  close(sockfd);

  return 0;
}
