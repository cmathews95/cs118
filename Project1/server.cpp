// server.cpp

#include <sys/types.h>
#include <fstream>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <string.h>

#include <stdio.h>

#include <errno.h>

#include <unistd.h>

#include <fcntl.h>

#include <thread>


// From showip.cpp

#include <sys/types.h>

#include <sys/socket.h>

#include <netdb.h>



#include <iostream>

#include <sstream>


#include "HttpRequest.h"

#include "HttpResponse.h"

#include "HttpMessage.h"


using namespace std;


#define MAXBUFLEN 4096

string _filedir, _port, _host;


void threadFunc(int client_socketfd)

{


	//		close(socketfd); // Don't want to be accepting a new connection while in the child process

	unsigned char buf[MAXBUFLEN];

	memset(&buf, '\0', sizeof(buf));


	int readStatus = read(client_socketfd, buf, sizeof(buf));


	if (readStatus < 0)

	{

		close(client_socketfd);

		cerr << "Error: Failed to read client request." << endl;

		exit(8);

	}


	stringstream ss;

	ss << buf << endl;

	if (ss.str() == "close\n")

	{

		terminate();

	}


	int i = 0;

	vector<unsigned char> vec;

	while (buf[i])

	{

		vec.push_back(buf[i]);

		i++;

	}


	HttpRequest req(vec);


	string code(""), reason("");
	char * body = 0;
	int bodyLength=0;

	// Response code referenced from https://developer.mozilla.org/en-US/docs/Web/HTTP/Response_codes

	cout << "Preparing response." << endl;

	if (req.getMethod() != "GET")

	{

		code = BAD_REQUEST;

		reason = "Bad method in request.";

	}

	else

	{

	  FILE *fd = fopen((_filedir + req.getUrl()).c_str(), "rb");		
	  cout << "Looking for file " << (_filedir + req.getUrl()) << endl;
	  
	  if (fd == NULL)

	    {
	      
	      code = NOT_FOUND;
	      
	      reason = "Page not found.";
	      
	    }
	  
	  else
	    
	    {
	      
	      code = OK;
	      
	      reason = "Request okay.";
	      fseek(fd, 0, SEEK_END); // seek to end of file
	      long size = ftell(fd); // get current file pointer
	      fseek(fd, 0, SEEK_SET);

	      char * resp_buf = (char *)malloc(size * sizeof(char));
	      int resp_readStatus = fread(resp_buf,size,1,fd);
	      fclose(fd);
	      if (resp_readStatus < 0)
		cerr << "Error: Failed to read from file." << endl;
	      body = resp_buf;
	      bodyLength = size;
	      
	    }
	  
	}
	
	HttpResponse resp(code, reason, body,bodyLength);
	
	resp.setHeaderField(CONTENT_LENGTH,to_string(bodyLength));
	
	char* respVec = resp.encode();
	
	
	int responseStatus = send(client_socketfd, respVec, bodyLength+resp.getHeaderText().length(), 0);
	cout <<"Wrote a response to the client, close connection next"<< endl;
	if (body > 0)
	  free(body);
	free(respVec);
	if (responseStatus < 0)

	{

		cerr << "Error: Failed to send response back to client." << endl;

		exit(10);

	}


	close(client_socketfd);




}


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


	_host = "localhost";

	_port = "4000";

	_filedir = ".";

	if (argc >= 2)

	{

		_host = argv[1];

	}

	if (argc >= 3)

	{

		_port = argv[2];

	}

	if (argc == 4)

	{

		_filedir = argv[3];
 
	}

	if (argc > 4)

	{

		cout << "Error: Too many arguments. Number of arguments should be less than 3." << endl;

		exit(2);

	}


	cout << "Initializing server with hostname= " << _host << ", port= " << _port << ", file directory= " << _filedir << endl;

	// Initialize the server

	const char* ip = dns(_host.c_str(), _port.c_str()).c_str();

	cout << "IP " << ip << endl;

	cout << "Port " << atoi(_port.c_str()) << endl;


	int socketfd = socket(AF_INET, SOCK_STREAM, 0);

	if (socketfd < 0)

	{

		cerr << "Error: Failed to open a socket." << endl;

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

		cerr << "Error: Failed to bind socket to address." << endl;

		exit(4);

	}


	// Start listening

	// Allow 8 backlogged connection in queue

	int listenStatus = listen(socketfd, 8);

	cout << "Listen status: " << listenStatus << endl;

	if (listenStatus < 0)

	{

		close(socketfd);

		cerr << "Error: Failed to listen for requests." << endl;

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


		if (client_socketfd < 0)

		{

			close(socketfd);

			close(client_socketfd);

			cerr << "Error: Failed to accept connection with client." << endl;

			exit(6);

		}


		char client_ip[INET_ADDRSTRLEN] = { '\0' };

		inet_ntop(client_sAddress.sin_family, &client_sAddress.sin_addr, client_ip, sizeof(client_ip));

		cout << "Accepted connection from: " << client_ip << ":" << ntohs(client_sAddress.sin_port) << endl;


		thread(threadFunc, client_socketfd).detach();

	}

	close(socketfd);

	return 0;





}
