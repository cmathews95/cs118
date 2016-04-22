#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include "HttpMessage.h"
#include <string>


class HttpResponse : public HttpMessage
{
 public:
 HttpResponse(std::string _code, std::string _reason, std::string _body) : HttpMessage(),statuscode(_code),reasoning(_reason),body(_body) {}
  

  std::string getStatusCode(void) const { return statuscode; }
  void setStatusCode(std::string _code) { statuscode=_code; }
  
  //setting Header fields
  
  std::vector<unsigned char> encode(void);
  std::string toText(void);


 private:
  std::string statuscode;
  std::string reasoning;
  std::string body;
  std::map<std::string,std::string> headerFields;

};


std::string HttpResponse::toText(void) {
  std::string text = getVersion() + " " + statuscode + " " +reasoning +"\r\n";
  std::map<std::string,std::string> headerFields = getHeaderFields();
  for(std::map<std::string,std::string>::iterator iter = headerFields.begin(); iter != headerFields.end(); ++iter)
    {
      text+= iter->first + ": " + iter->second + "\r\n";
    }
  return text;
}



#endif

