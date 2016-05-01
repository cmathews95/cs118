#include "HttpResponse.h"
#include <iostream>

using namespace std;

int main() {
  HttpResponse response("200","","This is what you wanted right?");
  response.setHeaderField(CONTENT_LENGTH,"32");
  response.setHeaderField(FROM,"localhost");
  response.setConnection(CLOSE);
  cout<< "I created the object, about to encode it!" << "\n";
  vector<unsigned char> vec = response.encode();
  cout << "About to decode" << "\n";
  HttpResponse response2(vec);
  response2.setHeaderField(DATE,"date");
  cout << "Encoding again!" << "\n";
  vec = response2.encode();

  for (int i = 0; i < vec.size(); i++)
    std::cout << ' ' <<  vec[i];
  std::cout << "\n";
  

}
