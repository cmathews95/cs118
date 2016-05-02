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


void threadFunc(int client_socketfd)

{

	cout << "In child process." << endl;

	//		close(socketfd); // Don't want to be accepting a new connection while in the child process

	unsigned char buf[MAXBUFLEN];

	memset(&buf, '\0', sizeof(buf));

	cout << "Created buffer." << endl;


	int readStatus = read(client_socketfd, buf, sizeof(buf));

	cout << "Read Status: " << readStatus << endl;

	if (readStatus < 0)

	{

		close(client_socketfd);

		cout << "Error: Failed to read client request." << endl;

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

	cout << "Created Vector: " << endl;

	while (buf[i])

	{

		vec.push_back(buf[i]);

		i++;

	}

	cout << "Creating a request from the vector." << endl;

	HttpRequest req(vec);

	cout << "Created a request from the vector." << endl;


	string code, reason, body;

	// Response code referenced from https://developer.mozilla.org/en-US/docs/Web/HTTP/Response_codes

	cout << "Preparing response." << endl;

	if (req.getMethod() != "GET")

	{

		code = "BAD_REQUEST";

		reason = "Bad method in request.";

	}

	else

	{

		int resp_fd = open((_filedir + req.getUrl()).substr(1).c_str(), O_RDONLY);

		if (resp_fd < 0)

		{

			code = "NOT_FOUND";

			reason = "Page not found.";

		}

		else

		{

			code = "OK";

			reason = "Request okay.";

			char resp_buf[MAXBUFLEN];

			memset(resp_buf, '\0', sizeof(resp_buf));

			int resp_readStatus;


			resp_readStatus = read(resp_fd, resp_buf, sizeof(resp_buf));


			if (resp_readStatus < 0)

				cout << "Error: Failed to read from file." << endl;

			body = resp_buf;

		}

		close(resp_fd);

	}

	HttpResponse resp(code, reason, body);

	resp.setHeaderField(CONTENT_LENGTH, to_string(body.length()));

	vector<unsigned char> respVec = resp.encode();

	string respString = "";

	for (int i = 0; i < vec.size(); i++)

	{

		respString += vec[i];

	}

	cout << "Response: " << respString << endl;


	int responseStatus = send(client_socketfd, respString.c_str(), respString.length(), 0);

	if (responseStatus < 0)

	{

		cout << "Error: Failed to send response back to client." << endl;

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

	cout << "Port " << atoi(_port.c_str()) << endl;


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


		if (client_socketfd < 0)

		{

			close(socketfd);

			close(client_socketfd);

			cout << "Error: Failed to accept connection with client." << endl;

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