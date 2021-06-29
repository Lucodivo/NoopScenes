#pragma once

#include <time.h>

f32 getTime() {
  clock_t time = clock();
  return (f32)time / CLOCKS_PER_SEC;
}

// stopwatch
struct StopWatch {
  f32 totalElapsed;
  f32 lastFrame;
  f32 delta;
};

StopWatch createStopWatch() {
  StopWatch stopWatch;
  stopWatch.totalElapsed = 0.0f;
  stopWatch.lastFrame = getTime();
  stopWatch.delta = 0.0f;
  return stopWatch;
}

void updateStopWatch(StopWatch* stopWatch) {
  f32 t = getTime();
  stopWatch->delta = t - stopWatch->lastFrame;
  stopWatch->lastFrame = t;
  stopWatch->totalElapsed += stopWatch->delta;
}

// 32 bit boolean flags
void clearFlags(b32* flags) {
  *flags = 0;
}

void setFlags(b32* flags, b32 desiredFlags){
  *flags |= desiredFlags;
}

void overrideFlags(b32* flags, b32 desiredFlags) {
  *flags = desiredFlags;
}

b32 flagIsSet(b32 flags, b32 checkFlag) {
  return flags & checkFlag;
}

b32 flagsAreSet(b32 flags, b32 checkFlags) {
  return (flags & checkFlags) == checkFlags;
}

char* cStrAllocateAndCopy(const char* cStr) {
  char* returnCStr = new char[strlen(cStr) + 1];
  strcpy(returnCStr, cStr);
  return returnCStr;
}