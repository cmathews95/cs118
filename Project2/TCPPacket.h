#ifndef TCP_H
#define TCP_H
#include <inttypes.h>
#include <bitset>
#include <string.h>
#include <stdexcept>
#include <iostream>

typedef uint16_t uint16;

const int ACKINDEX = 0;
const int SYNINDEX = 1;
const int FININDEX = 2;



class TCPPacket 
{
 public:
 TCPPacket(uint16 _seqNumber, uint16 _ackNumber, uint16 _windowSize, std::bitset<3> _flags, unsigned char* _body, int _bodyLength):
  seqNumber(_seqNumber), ackNumber(_ackNumber), windowSize(_windowSize), flags(_flags), body(_body), bodyLength(_bodyLength)
  {}
  TCPPacket(unsigned char* buff, int length);
  bool encode(unsigned char* buff);
  int getLengthOfEncoding() {return bodyLength + 8;}
  int getBodyLength() {return bodyLength;}

  
  uint16 getSeqNumber() {return seqNumber;}
  uint16 getAckNumber() {return ackNumber;}
  uint16 getWindowSize() {return windowSize;}
  bool getFIN() {return flags.test(FININDEX);}
  bool getACK() {return flags.test(ACKINDEX);}
  bool getSYN() {return flags.test(SYNINDEX);}


 private:
  uint16 seqNumber;
  uint16 ackNumber; 
  uint16 windowSize;
  std::bitset<3> flags;
  unsigned char * body;
  int bodyLength;
};

inline TCPPacket::TCPPacket(unsigned char* buff, int length) {
  if (length <= 8) {
    throw std::invalid_argument("No TCP Header Present");
  }
  seqNumber = (buff[0]+((buff[1]&0xff)<<8));
  ackNumber = (buff[2]+((buff[3]&0xff)<<8));
  windowSize = (buff[4]+((buff[5]&0xff)<<8));
  bool fin = ((buff[7] & 128) != 0);
  bool syn = ((buff[7] & 64) != 0);
  bool ack = ((buff[7] & 32) != 0);
  std::bitset<3> f;
  f.set(FININDEX,fin);
  f.set(SYNINDEX,syn);
  f.set(ACKINDEX,ack);
  flags = f;
  body = buff+8;
  bodyLength = length-8;
}

bool TCPPacket::encode(unsigned char* buf) {
  buf[0] = seqNumber & 0xff;
  buf[1] = (seqNumber>>8)&0xff;
  buf[2] = ackNumber & 0xff;
  buf[3] = (ackNumber>>8)&0xff;
  buf[4] = windowSize & 0xff;
  buf[5] = (windowSize>>8) & 0xff;
  buf[6] = 0;
  buf[7] = (128*flags.test(FININDEX))+(64*flags.test(SYNINDEX)) + (32*flags.test(ACKINDEX));
  memcpy(&buf[8],body,bodyLength);
  return true;
}




#endif
