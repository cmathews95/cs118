#include "HttpResponse.h"
#include <iostream>
#include <cstdlib>
using namespace std;

int main() {
  char * s = "This is what you wanted right? I can't really do this AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
  HttpResponse response("200","",s,strlen(s));
  std::cout << "what?: " << strlen(s) << std::endl;
  response.setHeaderField(CONTENT_LENGTH,to_string(strlen(s)));
  response.setHeaderField(FROM,"localhost");
  response.setConnection(CLOSE);
  cout<< "I created the object, about to encode it!" << "\n";
  char* vec = response.encode();
  cout << "About to decode" << "\n";
  HttpResponse response2(vec);
  response2.setHeaderField(DATE,"date");
  cout << "Encoding again!" << "\n";
  vec = response2.encode();
  int i;
  for (i = 0; i < strlen(vec); i++)
    std::cout << ' ' <<  vec[i];
  std::cout << "\n";
  

}
