#include "HttpRequest.h"
#include <iostream>

using namespace std;

int main() {
  HttpRequest request("www.cnn.com","GET");
  request.setHeaderField(USER_AGENT,"CS118 Project");
  request.setConnection(CLOSE);
  vector<unsigned char> vec = request.encode();
  HttpRequest request2(vec);
  request2.setHeaderField(HOST,"cnn.com");
  vec = request2.encode();

  for (int i = 0; i < vec.size(); i++)
    std::cout << ' ' <<  vec[i];

  

}
