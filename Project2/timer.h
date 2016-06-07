#include <ctime>
#include <cstdio>
#include <iostream>
#include <chrono>
class timer
{
 public:
  void start(double _time) { time=_time; startTime=std::chrono::system_clock::now();   running = true;}
  bool hasFired() { return (  running && (getTimeMicro() > time));}
  bool isRunning() {return running;}
  double getRunningTime() { std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - startTime; return elapsed_seconds.count();}
  double getTimeMicro() { return (getRunningTime() * 1000000.0); }
  void stop() {running = false;}
  void reset() { startTime=std::chrono::system_clock::now();}
 private:
  std::chrono::time_point<std::chrono::system_clock> startTime;
  double time;
  bool running;
};
