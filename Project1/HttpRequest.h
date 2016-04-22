#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <boost/tokenizer.hpp>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <stdint.h>
#include <map>
#include <utility>
#include "HttpMessage.h"
enum HttpRequestConnection {KEEP_ALIVE, CLOSE};
enum HttpRequestMethod {GET};
std::map<HttpRequestMethod, std::string> HttpRequestMethodMap = {{GET,"GET"}};



class HttpRequest : public HttpMessage
{
 public:
  //constructors
 HttpRequest(std::string _url, std::string _method, std::map<std::string,std::string> _headers):HttpMessage(_headers) { url = _url; method = _method;}
 HttpRequest(std::string _url, std::string _method) : HttpMessage() { url = _url; method = _method;}
 HttpRequest(std::vector<unsigned char> wire);

  //get and set url and method
  std::string getUrl(void) const { return url; }
  void setUrl(std::string _url) { url =_url; } 
  std::string getMethod(void) const { return method; }
  void setMethod(std::string _method) { method = _method; }
  
  //setting Header fields
  //setting the Connection, a specific header
  void setConnection(HttpRequestConnection conn) { setHeaderField(CONNECTION,(conn==KEEP_ALIVE?std::string("keep-alive"):std::string("close"))); }

  std::string toText(void);


 private:
  std::string url;
  std::string method;
  
};


HttpRequest::HttpRequest(std::vector<unsigned char> wire) : HttpMessage() {
  typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
  std::string s(wire.begin(),wire.end());
  boost::char_separator<char> CRLF{"\r\n"};
  boost::char_separator<char> space{" "};
  tokenizer allLines{s,CRLF};
  url = "";
  method = "";
  for (tokenizer::iterator it = allLines.begin(); it != allLines.end(); ++it) {
    if (it == allLines.begin()) {
          tokenizer tokens{*it,space};
	  tokenizer::iterator it1= tokens.begin();
	  method = *it1;
	  std::advance(it1,1);
	  url = *it1;
    }
    else {
      std::string::size_type pos = (*it).find(" ");
      if (pos > 0) {
	std::string first = (*it).substr(0,pos);
	first.erase(std::remove(first.begin(),first.end(),':'),first.end());
	for (auto i = HttpHeaderFieldsMap.begin(); i != HttpHeaderFieldsMap.end(); ++i) {
	  if (first.compare(i->second)==0) {
	    	std::string rest = (*it).substr(pos+1);
		setHeaderField(i->first,rest);
	    break;
	  }
	}

      }
    }
  }
    
}



std::string HttpRequest::toText(void) {
  std::string text = method + " " + url + " " + HttpVersionToken+"\r\n";
  std::map<std::string,std::string> map = getHeaderFields();
  std::cout<<"Supposed length: " << map.size()  << "\n" <<  std::cout.flush();
  for(std::map<std::string,std::string>::iterator iter = map.begin(); iter != map.end(); iter++)
    {
        std::cout<<"Adding this: " << iter->first  << "\n" <<  std::cout.flush();
      text+= iter->first + ": " + iter->second + "\r\n";
    }
  return text;
}


#endif
