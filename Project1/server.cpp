// server.cpp
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

// From showip.cpp
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


#include <iostream>
#include <sstream>

#include "HttpRequest.h"
#include "HttpResponse.h"


using namespace std;

#define MAXBUFLEN 4096
/*
string hostname_to_ip(char *hostname)
{
	char ipString[20]; // Allocated a little extra space. IP Addresses should be 15 characters long. 1 more for null byte.
	struct hostent *host = gethostbyname(hostname);
	if (host == NULL)
	{
		cout << "Error: Could not resolve host name." << endl;
		exit(1);
	}
	
	struct in_addr  **addresses = host->h_addr_list;
	int i = 0;
	while (addresses[i])
	{
		strcpy(ipString, inet_ntoa(*addresses[i]));
	}

	string ret = ipString;
	return ret;
}
*/

std::string dns(const char* hostname, const char* port) {
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
		char ipstr[INET_ADDRSTRLEN] = { '\0' };
		inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
		return std::string(ipstr);
	}
	return NULL;
}



int main(int argc, char *argv[])
{

	string _host = "localhost";
	string _port = "4000";
	string _filedir = ".";
	if (argc >= 2)
	{
		_host = argv[1];
	}
	if (argc >= 3)
	{
		_port = argv[2];
	}
	if (argc = 4)
	{
		_filedir = argv[3];
	}
	if (argc > 4)
	{
		cout << "Error: Too many arguments. Number of arguments should be less than 3." << endl;
		exit(2);
	}
	// Initialize the server
	const char* ip = dns(_host.c_str(), _port.c_str()).c_str();	
	cout << "IP " << ip << endl;
	cout << "Port " << atoi(_port.c_str()) <<  endl;

	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd < 0)
	{
		cout << "Error: Failed to open a socket." << endl;
		exit(3);
	}

	struct sockaddr_in sAddress;
	sAddress.sin_family = AF_INET;
	sAddress.sin_port = htons(atoi(_port.c_str()));
	sAddress.sin_addr.s_addr = inet_addr(ip);
	memset(sAddress.sin_zero, '\0', sizeof(sAddress.sin_zero));

	int bindStatus = bind(socketfd, (struct sockaddr*) &sAddress, sizeof(sAddress));
	// cout << "Bind status: " << bindStatus << endl;
	if (bindStatus < 0)
	{
	  
		close(socketfd);
		cout << "Error: Failed to bind socket to address." << endl;
		exit(4);
	}

	// Start listening
		// Allow 8 backlogged connection in queue
	int listenStatus = listen(socketfd, 8);
	cout << "Listen status: " << listenStatus << endl;
	if (listenStatus < 0)
	{
		close(socketfd);
		cout << "Error: Failed to listen for requests." << endl;
		exit(5);
	}
	

	// Wait for a connection request to accept
	while (1)
	{
	  cout << "Waiting for connections. " << endl;
		struct sockaddr_in client_sAddress;
		socklen_t client_AddressSize = sizeof(client_sAddress);
		int client_socketfd = accept(socketfd, (struct sockaddr*)&client_sAddress, &client_AddressSize);
		cout << "Client socket: " << client_socketfd << endl;
		
		char client_ip[INET_ADDRSTRLEN] = {'\0'};
		inet_ntop(client_sAddress.sin_family, &client_sAddress.sin_addr, client_ip, sizeof(client_ip));
		cout << "Accepted connection from: " << client_ip << ":" << ntohs(client_sAddress.sin_port) << endl;

		if (client_socketfd < 0)
		{
			close(socketfd);
			close(client_socketfd);
			cout << "Error: Failed to accept connection with client." << endl;
			exit(6);
		}

		// Reading from the connection
		char buf[20] = {0};
		stringstream ss;

		while (1)
		  {
		    memset(buf, '\0', sizeof(buf));
		    int receptionStatus =  recv(client_socketfd, buf, 20, 0);
		    if (receptionStatus < 0)
		      {
			cout << "Error: Failed to receive message from client." << endl;
			exit (7);
		      }
		    ss << buf << endl;
		    cout << buf << endl;
		    
		    int sendStatus = send(client_socketfd, buf, 20, 0);
		    if (sendStatus < 0)
		    {
		      cout << "Error: Failed to send message to client." << endl;
		      exit (8);
		    }
		    if (ss.str() == "close\n")
		      {
			break;
		      }
		    ss.str("");
		  }
		close(client_socketfd);
		return 0;

	
		int pid = fork();
		if (pid == 0)
		{
			close(socketfd); // Don't want to be accepting a new connection while in the child process
			unsigned char buf[MAXBUFLEN];
			memset(&buf, '\0', sizeof(buf));
			
			int readStatus = read(client_socketfd, buf, sizeof(buf));
			if (readStatus < 0)
			{
				close(socketfd);
				close(client_socketfd);
				cout << "Error: Failed to read client request." << endl;
				exit(8);
			}

			int i = 0;
			vector<unsigned char> vec;
			while (buf[i])
			{
			  cout << "Reading buf" << endl;
				vec.push_back(buf[i]);
				i++;
			}

			HttpRequest req(vec);
			/*
			string code, reason, body;
			// Response code referenced from https://developer.mozilla.org/en-US/docs/Web/HTTP/Response_codes

			if (req.getMethod() != "GET")
			{
				code = "400";
				reason = "Bad method in request.";
			}
			else
			{
			  int resp_fd = open(req.getUrl().substr(1).c_str(), O_RDONLY);
				if (resp_fd < 0)
				{
					code = "404";
					reason = "Page not found.";
				}
				else
				{
					code = "200";
					reason = "Request okay.";
					char resp_buf[MAXBUFLEN];
					memset(resp_buf, '\0', sizeof(resp_buf));
					int resp_readStatus;

					resp_readStatus = read(resp_fd, resp_buf, sizeof(resp_buf));
					
					if (resp_readStatus < 0)
						cout << "Error: Failed to read from file." << endl;
					body = resp_buf;
					
				}
			}
			*/
		}
		else
		{
			cout << "In parent." << endl;
			exit(0);
		}
	}
}
