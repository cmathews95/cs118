#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <cstring>
#include <string>
#include <vector>
#include <stdint.h>
#include <map>

enum HttpRequestConnection {KEEP_ALIVE, CLOSE};

const std::map<HttpRequestConnection, std::string> HttpRequestConnectionMap = {{KEEP_ALIVE,"keep-alive"},{CLOSE,"close"}};

enum HttpRequestMethod {GET};

const std::map<HttpRequestMethod, std::string> HttpRequestMethodMap = {{GET,"GET"}};

enum HttpRequestHeaderFields {HOST,USER_AGENT,FROM,CONNECTION}; //continued

std::map<HttpRequestHeaderFields, std::string> HttpRequestHeaderFieldsMap = {{HOST,"Host"},{USER_AGENT,"User-Agent"},{FROM,"From"},{CONNECTION,"Connection"}}; //continued

const std::string HttpVersionToken = "HTTP/1.0";

class HttpRequest
{
 public:
  HttpRequest(std::string _url, std::string _method, std::map<std::string,std::string> _headers) { url = _url; method = _method; headerFields=_headers; }
  HttpRequest(std::string _url, std::string _method) { url = _url; method = _method; headerFields;}

  std::string getUrl(void) const { return url; }
  void setUrl(std::string _url) { url =_url; } 
  std::string getMethod(void) const { return method; }
  void setMethod(std::string _method) { method = _method; }
  
  void setHeaderField(HttpRequestHeaderFields key, std::string value) { headerFields[(HttpRequestHeaderFieldsMap[key])]=value; }
  std::string getHeaderField(HttpRequestHeaderFields key) { return headerFields[(HttpRequestHeaderFieldsMap[key])]; }

  void setConnection(HttpRequestConnection conn) { headerFields["Connection"]=(conn==KEEP_ALIVE?"keep-alive":"close"); }

  std::vector<uint8_t> encode(void);
  std::string toText(void);
 private:
  std::string url;
  std::string method;
  std::map<std::string,std::string> headerFields;
};


std::vector<uint8_t> HttpRequest::encode() {
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
