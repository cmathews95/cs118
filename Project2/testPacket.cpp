
#include "TCPPacket.h"
#include <iostream>
#include <assert.h>
using namespace std;

int main() {
  string bodyStr = "Body bytes bytes bytes \n \n bytes";
  unsigned char * buff = (unsigned char *)malloc((bodyStr.length()+1)*sizeof(unsigned char));
  memcpy(buff,bodyStr.c_str(),bodyStr.length());
  buff[bodyStr.length()]='\0';
  TCPPacket packet('A'+('A'<<8),'A'+('A'<<8),'A',bitset<3>(0x05),buff, bodyStr.length()+1);

  unsigned char * wire = (unsigned char *)malloc((packet.getLengthOfEncoding()+1)*sizeof(unsigned char));
  packet.encode(wire);

  TCPPacket packet2(wire,packet.getLengthOfEncoding());

  unsigned char * wire2 = (unsigned char *)malloc((packet2.getLengthOfEncoding())*sizeof(unsigned char));
  packet2.encode(wire2);
  assert(packet.getSeqNumber() == packet2.getSeqNumber());
  assert(packet.getAckNumber() == packet2.getAckNumber());
  assert(packet.getWindowSize() == packet2.getWindowSize());
  assert(packet.getFIN() == packet2.getFIN());
  assert(packet.getACK() == packet2.getACK());
  assert(packet.getSYN() == packet2.getSYN());

  std::cout<<"Original"<<endl;
  for (int i = 0; i < packet.getLengthOfEncoding(); i++)
    std::cout << ' ' <<  wire[i];
  cout << "new one: " << endl;
  for (int i = 0; i < packet2.getLengthOfEncoding(); i++)
    std::cout << ' ' <<  wire2[i];

  free(buff);
  free(wire);

}
