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
 HttpResponse(std::string _code, std::string _reason, char * _body,int _bodyLength) : HttpMessage(),statuscode(_code),reasoning(_reason),body(_body),bodyLength(_bodyLength),deallocFlag(0) {}
   HttpResponse(char * wire);
   ~HttpResponse() {if (deallocFlag) free(body);}
  std::string getStatusCode(void) const { return statuscode; }
  void setStatusCode(std::string _code) { statuscode=_code; }
  std::string getReasoning(void) const { return reasoning; }
  void setReasoning(std::string r) { reasoning = r; }
  char* getBody(void) const { return body; }
  int getBodyLength(void) const {return bodyLength;}
  void setBody(char*  b,int length) { body=b; bodyLength = length;}
  
  //setting Header fields
  
  char* toText(void);
  std::string getHeaderText(void);
 private:
  std::string statuscode;
  std::string reasoning;
   char * body;
   int bodyLength;
  
   int deallocFlag;

};

std::string HttpResponse::getHeaderText(void) {
  std::string text = getVersion() + " " + statuscode + " " +reasoning +"\r\n";
  std::map<std::string,std::string> headerFields = getHeaderFields();
  for(std::map<std::string,std::string>::iterator iter = headerFields.begin(); iter != headerFields.end(); ++iter)
    {
      text+= iter->first + ": " + iter->second + "\r\n";
    }
  text += "\r\n";
  return text;
}

char* HttpResponse::toText(void) {

  std::string text = getHeaderText();
  
  std::cout << "About to memcpy the text data " << bodyLength << std::endl;
  char * rt = (char*) malloc(sizeof(char) * (text.length() + bodyLength+1));
  

  strcpy(rt,text.data());
  
  if (body > 0) {
    std::cout << "Memcpying the body, which will be harder "<<bodyLength <<" " << strlen(body) << std::endl;
    memcpy(rt+text.length(),body,bodyLength);
  }
  rt[text.length()+bodyLength]='\0';
  return rt;
}

HttpResponse::HttpResponse(char * wire) : HttpMessage() {
  typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
  std::string s(wire);
  std::cout << "What is the s: "<< s << std::endl;
  boost::char_separator<char> CRLF{"\r\n"};
  boost::char_separator<char> space{" "};
  body = 0;
  deallocFlag = 1;
  reasoning="";
  statuscode="";
  std::string DCRLF = "\r\n\r\n";
  std::string str1 = s.substr(0,s.find(DCRLF));
  tokenizer allLines{str1,CRLF};
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
   std::map<std::string,std::string>  map = getHeaderFields();
  if (map.find(HttpHeaderFieldsMap[CONTENT_LENGTH]) != map.end() ) {
    std::string str_ = map[HttpHeaderFieldsMap[CONTENT_LENGTH]];
    int contentLength = atoi(str_.c_str());
    char * b = (char*) malloc(sizeof(char) *  (contentLength));
    memcpy(b,wire+str1.length()+DCRLF.length(),contentLength);
    body = b;
    bodyLength = contentLength;
    deallocFlag = 1;
  }
}

#endif

