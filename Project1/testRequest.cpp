#include "HttpRequest.h"
#include <iostream>

using namespace std;

int main() {
  HttpRequest request("www.cnn.com","GET");
  request.setHeaderField(USER_AGENT,"CS118 Project");
  request.setConnection(CLOSE);
  vector<uint8_t> vec = request.encode();
  
  for (int i = 0; i < vec.size(); i++)
    std::cout << ' ' <<  vec[i];

}
