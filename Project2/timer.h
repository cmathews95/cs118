#include <ctime>


class timer
{
 public:
  void start(float _time) { time=_time; startTime=clock(); running = true;}
  bool hasFired() { return (  ((float)(clock() - startTime) * 1000000.0f) /((float)CLOCKS_PER_SEC) > time);}
  bool isRunning() {return running;}
  float getTime() {  return  ((float)(clock() - startTime) * 1000000.0f) /((float)CLOCKS_PER_SEC); }
  void stop() {running = false;}
  void reset() { startTime=clock();}
 private:
  std::clock_t startTime;
  float time;
  bool running;
};
