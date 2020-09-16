#include "vecTrans2.h"

void vecTrans2(int A[1024], int b[1024]) { 
  for (int i = 0; i < 1000; i++) { 
    int d = A[i];
    if (d > 0)
      // A[b[i]] = (((((((d+112)*d+23)*d+36)*d+82)*d+127)*d+2)*d+20)*d+100;
      A[b[i]] = g(d);
  }
}
