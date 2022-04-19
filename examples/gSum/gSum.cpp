/*
  Benchmark:    gSum
  FileName:     gSum.cpp
  Author:       Jianyi Cheng
  Date:         11 Apr 2022
*/

#define K 2

#include "gSum.h"

void g(float d, float s, float *s_new) {
#pragma DASS SS II = 1
  *s_new =
      s +
      (((((d + 0.64f) * d + 0.7f) * d + 0.21f) * d + 0.33f) * d + 0.25f) * d +
      0.125f;
}

out_float_t gSum(in_float_t A[1000], in_float_t B[1000]) {
  float s = 0.0f;
  int i;

  for (i = 0; i < 1000; i++) {
    float d = A[i] + B[i];
    if (d > 0.0f) {
      float s_new;
      g(d, s, &s_new);
      s = s_new;
    }
  }
  return s;
}

int main() {
  float A[1000], B[1000], accum = 0.0f, res, k, i;

  for (int j = 0; j < 1000; j++) {
    A[j] = j + 1.0f;
    if (j % K == 0)
      B[j] = -A[j] + 2.0f;
    else
      B[j] = -A[j] - 2.0f;

    i = A[j] + B[j];
    if (i > 0.0f) {
      accum +=
          (((((i + 0.64f) * i + 0.7f) * i + 0.21f) * i + 0.33f) * i + 0.25f) *
              i +
          0.125f;
    }
  }

  res = gSum(A, B);

  return 0;
}
