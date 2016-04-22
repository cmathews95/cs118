#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <boost/foreach.hpp>

enum HttpHeaderFields {CONTENT_LENGTH,DATE,CONTENT_TYPE,HOST,USER_AGENT,FROM,CONNECTION}; //continued

std::map<HttpHeaderFields, std::string> HttpHeaderFieldsMap = {{CONTENT_LENGTH,"Content-Length"},{DATE,"Date"},{CONTENT_TYPE,"CONTENT_TYPE"},{HOST,"Host"},{USER_AGENT,"User-Agent"},{FROM,"From"},{CONNECTION,"Connection"}}; //continued


const std::string HttpVersionToken = "HTTP/1.0";


class HttpMessage
{
 public:
  HttpMessage(void) {httpVersion = HttpVersionToken; headerFields;}
  HttpMessage(std::map<std::string,std::string> _headers) { httpVersion = HttpVersionToken; headerFields=_headers; }
      std::string getVersion(void) const { return httpVersion; }
  void setVersion(std::string _version) { httpVersion =_version; } 

void setHeaderField(HttpHeaderFields key, std::string value) { headerFields[(HttpHeaderFieldsMap[key])]=value; }
  std::string getHeaderField(HttpHeaderFields key) { return headerFields[(HttpHeaderFieldsMap[key])]; }
  std::vector<unsigned char> encode(void) {  std::vector<unsigned char> wire;   const char * data = toText().data();   int length = strlen(data);   wire.assign(data,data+length);  return wire;}
  virtual std::string toText(void)=0;
 protected:
  std::map<std::string,std::string> getHeaderFields(void) {return headerFields;}
 private:
    std::string httpVersion;
    std::map<std::string,std::string> headerFields;
};











#endif
