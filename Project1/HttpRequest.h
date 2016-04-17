#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include <string>
#include <vector>
#include <stdint.h>
#include <map>
enum HttpRequestConnection {KEEP_ALIVE, CLOSE};
enum HttpRequestMethod:string {GET};

enum HttpRequestHeaderFields:string {HOST="Host",USER_AGENT="User-Agent",FROM="From",CONNECTION="Connection"}; //continued

string HttpVersionToken = "HTTP/1.0";

class HttpRequest
{
 public:
  HttpRequest(std::string _url, std::string _method, std::map<std::string,std::string> _headers) { url = _url; method = _method; headerFields=_headers; }
  HttpRequest(std::string _url, std::string _method) { url = _url; method = _method; headerFields;}

  std::string getUrl(void) { return url; }
  void setUrl(std::string _url) { url =_url; } 
  std::string getMethod(void) { return method; }
  void setMethod(std::string _method) { method = _method; }
  
  void setHeaderField(HttpRequestHeaderFields key, std::string value) { headerFields[key]=value; }
  std::string getHeaderField(HttpRequestHeaderFields key) { return headerFields[key]; }

  void setConnection(HttpRequestConnection conn) { headerFields["Connection"]=(conn==KEEP_ALIVE?"keep-alive":"close"); }

  vector<uint8_t> encode(void);
  std::string toText(void);
 private:
  std::string url;
  std::string method;
  std::map<std::string,std::string> headerFields;
};


vector<uint8_t> HttpRequest::encode() {
  vector<uint8_t> wire;
  

  return wire;
}
std::string toText(void) {
  std::string text = method + " " + url + " " + HttpVersionToken+"\r\n";
  for(std::map<string,string>::iterator iter = headerFields.begin(); iter != headerFields.end(); ++iter)
    {
      text+= iter->first + ": " + iter->second + "\r\n";
    }
  return text;
}


#endif
