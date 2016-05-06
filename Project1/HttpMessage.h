#ifndef HTTP_MESSAGE_H
#define HTTP_MESSAGE_H
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <boost/foreach.hpp>

enum HttpHeaderFields {CONTENT_LENGTH,DATE,CONTENT_TYPE,HOST,USER_AGENT,FROM,CONNECTION}; //continued
enum HttpConnectionField {KEEP_ALIVE, CLOSE};
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
  char*  encode(void) {  return toText(); }

    void setConnection(HttpConnectionField conn) { setHeaderField(CONNECTION,(conn==KEEP_ALIVE?std::string("keep-alive"):std::string("close"))); }


  virtual char* toText(void)=0;
 protected:
  std::map<std::string,std::string> getHeaderFields(void) {return headerFields;}
 private:
    std::string httpVersion;
    std::map<std::string,std::string> headerFields;
};











#endif
