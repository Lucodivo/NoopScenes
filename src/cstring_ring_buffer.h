#pragma once

struct CStringRingBuffer {
  u32 first;
  u32 count;
  u32 cStringSize;
  u32 cStringMaxCount;
  char* buffer;
};

CStringRingBuffer createCStringRingBuffer(u32 stringSize, u32 stringsMaxCount) {
  CStringRingBuffer ringBuffer;
  ringBuffer.first = 0;
  ringBuffer.count = 0;
  ringBuffer.cStringSize = stringSize;
  ringBuffer.cStringMaxCount = stringsMaxCount;
  ringBuffer.buffer = new char[stringSize * stringsMaxCount];
  return ringBuffer;
}

// adds a string to the last available spot, if no spot is available, it replaces the first
// cstring and moves the first index forward
void addCString(CStringRingBuffer* ringBuffer, const char* cStr, u32 strLength) {
  u32 bufferOffset;

  if(ringBuffer->count == ringBuffer->cStringMaxCount) {
    bufferOffset = ringBuffer->first;
    ++ringBuffer->first;
    ringBuffer->first %= ringBuffer->cStringMaxCount;
  } else {
    bufferOffset = (ringBuffer->first + ringBuffer->count);
    ++ringBuffer->count;
  }

  char* cStrBuffer = ringBuffer->buffer + (bufferOffset * ringBuffer->cStringSize);

  if(strLength < ringBuffer->cStringSize) {
    strcpy(cStrBuffer, cStr);
  } else { // copy as much as we can into the buffer
    memcpy(cStrBuffer, cStr, ringBuffer->cStringSize - 1);
    cStrBuffer[ringBuffer->cStringSize - 1] = '\0';
  }
}

char* getCString(CStringRingBuffer* ringBuffer, const u32 index) {
  u32 ringOffset = (ringBuffer->first + index) % ringBuffer->cStringMaxCount;
  return ringBuffer->buffer + (ringOffset * ringBuffer->cStringSize);
}

void deleteCStringRingBuffer(CStringRingBuffer* ringBuffer) {
  delete[] ringBuffer->buffer;
  *ringBuffer = {}; // zero out buffer
}