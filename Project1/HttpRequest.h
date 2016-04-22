#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <boost/tokenizer.hpp>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <stdint.h>
#include <map>

enum HttpRequestConnection {KEEP_ALIVE, CLOSE};
enum HttpRequestMethod {GET};
std::map<HttpRequestMethod, std::string> HttpRequestMethodMap = {{GET,"GET"}};

enum HttpRequestHeaderFields {HOST,USER_AGENT,FROM,CONNECTION}; //continued

std::map<HttpRequestHeaderFields, std::string> HttpRequestHeaderFieldsMap = {{HOST,"Host"},{USER_AGENT,"User-Agent"},{FROM,"From"},{CONNECTION,"Connection"}}; //continued

const std::string HttpVersionToken = "HTTP/1.0";

class HttpRequest
{
 public:
  //constructors
  HttpRequest(std::string _url, std::string _method, std::map<std::string,std::string> _headers) { url = _url; method = _method; headerFields=_headers; }
 HttpRequest(std::string _url, std::string _method) : headerFields() { url = _url; method = _method;}
 HttpRequest(std::vector<unsigned char> wire);

  //get and set url and method
  std::string getUrl(void) const { return url; }
  void setUrl(std::string _url) { url =_url; } 
  std::string getMethod(void) const { return method; }
  void setMethod(std::string _method) { method = _method; }
  
  //setting Header fields
  void setHeaderField(HttpRequestHeaderFields key, std::string value) { headerFields[(HttpRequestHeaderFieldsMap[key])]=value; }
  std::string getHeaderField(HttpRequestHeaderFields key) { return headerFields[(HttpRequestHeaderFieldsMap[key])]; }
  //setting the Connection, a specific header
  void setConnection(HttpRequestConnection conn) { headerFields["Connection"]=(conn==KEEP_ALIVE?"keep-alive":"close"); }

  std::vector<unsigned char> encode(void);
  std::string toText(void);
 private:
  std::string url;
  std::string method;
  std::map<std::string,std::string> headerFields;
};


HttpRequest::HttpRequest(std::vector<unsigned char> wire) {
  typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
  std::string s(wire.begin(),wire.end());
  boost::char_separator<char> CRLF{"\r\n"};
  boost::char_separator<char> space{" "};
  tokenizer allLines{s,CRLF};
  url = "";
  method = "";
  headerFields; 
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
	std::string rest = (*it).substr(pos+1);
	first.erase(std::remove(first.begin(),first.end(),':'),first.end());
	headerFields[first]=rest;
      }
    }
  }
    
}





std::vector<unsigned char> HttpRequest::encode() {
  std::vector<uint8_t> wire;
  
  const char * data = toText().data();
  int length = strlen(data);

  wire.assign(data,data+length);
  return wire;
}


std::string HttpRequest::toText(void) {
  std::string text = method + " " + url + " " + HttpVersionToken+"\r\n";
  for(std::map<std::string,std::string>::iterator iter = headerFields.begin(); iter != headerFields.end(); ++iter)
    {
      text+= iter->first + ": " + iter->second + "\r\n";
    }
  return text;
}


#endif
