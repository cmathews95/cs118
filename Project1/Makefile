CC=g++
CFLAGS=-g -std=c++11 -pthread
USER=konradjniemiec
HEADERS=HttpResponse.h HttpRequest.h HttpMessage.h

all : web-client web-server


web-client : client.cpp $(HEADERS)
	$(CC) client.cpp $(CFLAGS) -o web-client

web-server : server.cpp $(HEADERS)
	$(CC) server.cpp $(CFLAGS) -o web-server

dist : project1-$(USER).tar.gz

project1-$(USER).tar.gz : client.cpp server.cpp $(HEADERS) Makefile 
	make clean
	tar -cvzf $@ * 

clean:
	rm -f  web-client web-server test *~ *.tar.gz
