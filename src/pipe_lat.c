/*
    Measure latency of IPC using unix domain sockets


    Copyright (c) 2016 Erik Rigtorp <erik@rigtorp.se>

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use,
    copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following
    conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
    OTHER DEALINGS IN THE SOFTWARE.
*/

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0) &&                           \
    defined(_POSIX_MONOTONIC_CLOCK)
#define HAS_CLOCK_GETTIME_MONOTONIC
#endif

int main(int argc, char *argv[]) {
  int ofds[2];
  int ifds[2];

  int size;
  char *buf;
  uint64_t *RTT;
  int64_t count, i, delta;
#ifdef HAS_CLOCK_GETTIME_MONOTONIC
  struct timespec start, stop;
#else
  struct timeval start, stop;
#endif

  if (argc != 3) {
    printf("usage: pipe_lat <message-size> <roundtrip-count>\n");
    return 1;
  }

  size = atoi(argv[1]);
  count = atol(argv[2]);

  RTT = malloc(count * sizeof(uint64_t));
  if (RTT == NULL) {
    perror("malloc");
    return 1;
  }

  buf = malloc(size);
  if (buf == NULL) {
    perror("malloc");
    return 1;
  }

  // printf("message size: %i octets\n", size);
  // printf("roundtrip count: %" PRId64 "\n", count);

  if (pipe(ofds) == -1) {
    perror("pipe");
    return 1;
  }

  if (pipe(ifds) == -1) {
    perror("pipe");
    return 1;
  }

  if (!fork()) { /* child */



    for (i = 0; i < count; i++) {

      if (read(ifds[0], buf, size) != size) {
        perror("read");
        return 1;
      }

      if (write(ofds[1], buf, size) != size) {
        perror("write");
        return 1;
      }
    }
  } else { /* parent */

    for (i = 0; i < count; i++) {
      if (write(ifds[1], buf, size) != size) {
        perror("write");
        return 1;
      }

      if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
        perror("clock_gettime");
        return 1;
      }

      if (read(ofds[0], buf, size) != size) {
        perror("read");
        return 1;
      }


      if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
        perror("clock_gettime");
        return 1;
      }

      delta = ((stop.tv_sec - start.tv_sec) * 1000000000 +
              (stop.tv_nsec - start.tv_nsec));

      RTT[i] = delta;
    }


    // print avg of RTT skip first 10K numbers
    uint64_t sum = 0;
    for (i = 10000; i < count - 1000; i++) {
      printf("%" PRId64 "\n", RTT[i]);
      sum += RTT[i];
    }
    printf("avg: %" PRId64 "\n", sum / (count - 11000));
    printf("parent %u\n", getpid());

  }

  free(RTT);
  free(buf);
  
  return 0;
}