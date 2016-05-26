#ifndef TCP_H
#define TCP_H

class TCPPacket 
{
 public:
  TCPPacket() {}
  TCPPacket(char[] buff);
  int encode(char* buff);
  int getLengthOfEncoding();
 private:
  

}





#endif
