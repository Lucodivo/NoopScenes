#pragma once

#include <time.h>

f32 getTime() {
  clock_t time = clock();
  return (f32)time / CLOCKS_PER_SEC;
}

struct StopWatch {
  f32 lastFrame;
  f32 delta;
};

StopWatch createStopWatch() {
  StopWatch stopWatch;
  stopWatch.lastFrame = getTime();
  stopWatch.delta = 0.0f;
  return stopWatch;
}

void updateStopWatch(StopWatch* stopWatch) {
  f32 t = getTime();
  stopWatch->delta = t - stopWatch->lastFrame;
  stopWatch->lastFrame = t;
}