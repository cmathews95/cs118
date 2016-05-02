#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include "HttpMessage.h"
#include <string>
#include <boost/tokenizer.hpp>

const std::string OK = "200";
const std::string BAD_REQUEST = "400";
const std::string NOT_FOUND = "404";

class HttpResponse : public HttpMessage
{
 public:
 HttpResponse(std::string _code, std::string _reason, std::string _body) : HttpMessage(),statuscode(_code),reasoning(_reason),body(_body) {}
   HttpResponse(std::vector<unsigned char> wire);

  std::string getStatusCode(void) const { return statuscode; }
  void setStatusCode(std::string _code) { statuscode=_code; }
  std::string getReasoning(void) const { return reasoning; }
  void setReasoning(std::string r) { reasoning = r; }
  std::string getBody(void) const { return body; }
  void setBody(std::string b) { body=b; }
  
  //setting Header fields
  
  std::string toText(void);


 private:
  std::string statuscode;
  std::string reasoning;
  std::string body;
  std::map<std::string,std::string> headerFields;

};


std::string HttpResponse::toText(void) {
  std::cout << "RESPONSE TOTEXT" << std::endl;
  std::string text = getVersion() + " " + statuscode + " " +reasoning +"\r\n";
  std::map<std::string,std::string> headerFields = getHeaderFields();
  for(std::map<std::string,std::string>::iterator iter = headerFields.begin(); iter != headerFields.end(); ++iter)
    {
      text+= iter->first + ": " + iter->second + "\r\n";
    }
  text += "\r\n";
  text += body;
  return text;
}

HttpResponse::HttpResponse(std::vector<unsigned char> wire) : HttpMessage() {
  typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
  std::string s(wire.begin(),wire.end());
  boost::char_separator<char> CRLF{"\r\n"};
  boost::char_separator<char> space{" "};
  body = "";
  reasoning="";
  statuscode="";
  std::string DCRLF = "\r\n\r\n";
  std::string str1 = s.substr(0,s.find(DCRLF));
  std::string str2 = s.substr(s.find(DCRLF));

  tokenizer allLines{str1,CRLF};
  if (str2.length() > 0) {
        str2.erase(str2.find(DCRLF),DCRLF.length());
	//std::cout << "Looking at body: "<< str2 << "\n";

    body = str2;
  }
  //std::cout <<"About to look at lines " << std::distance(allLines.begin(),allLines.end()) << "\n";
  for (tokenizer::iterator it = allLines.begin(); it != allLines.end(); ++it) {
    if (it == allLines.begin()) {
      //std::cout << "Looking at first line" << "\n";
      tokenizer tokens{*it,space};
      tokenizer::iterator it1= tokens.begin();
      //std::cout << "First " <<*it1<< "\n";
      setVersion(*it1);
      if (std::distance(tokens.begin(),tokens.end()) > 1) {
	std::advance(it1,1);
	//	std::cout << "Second" <<*it1<< "\n";
	statuscode = *it1;
	if (std::distance(tokens.begin(),tokens.end()) > 2) {
	  std::advance(it1,1);
	  //  std::cout << "Third" <<*it1<< "\n";
	  reasoning = *it1;
	}
      }
    }
    else {
      //      std::cout << "Looking at other header lines" << "\n";
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

#endif

