#include "vecNormTrans.h"

//------------------------------------------------------------------------
// vectorProcess
//------------------------------------------------------------------------

// SEPARATOR_FOR_MAIN

#include "vecNormTrans.h"
#include <stdlib.h>

#define N 1024

void vecNormTrans(in_float_t a[1024], inout_float_t r[1024]) {
  float weight = 0.0f;
  for (int i = 0; i < N; i++) {
    float d = a[i];
    if (d < 1.0f)
      weight = ((d * d + 19.52381f) * d + 3.704762f) * d + 0.73f * weight;
    else
      weight = weight;
  }

  for (int i = 0; i < N - 4; i++) {
    float d = a[i] / weight;
    r[i + 4] = r[i] + d;
  }
}

int main() {
  float array[N], result[N], gold[N];
  for (int i = 0; i < N; i++) {
    array[i] = (i % 2 == 0) ? 2.0f : 0.1f;
    result[i] = 0.0f;
    gold[i] = 0.0f;
  }

  float weight = 0.0f;
  for (int i = 0; i < N; i++) {
    float d = array[i];
    float s;
    if (d < 1.0f)
      weight = ((d * d + 19.52381f) * d + 3.704762f) * d + 0.73f * weight;
    else
      weight = weight;
  }

  for (int i = 0; i < N - 4; i++) {
    float d = array[i] / weight;
    gold[i + 4] = gold[i] + d;
  }

  vecNormTrans(array, result);

  int check = 0;
  for (int i = 0; i < N; i++)
    check += (result[i] == gold[i]);

  if (check == N)
    return 0;
  else
    return -1;
}
