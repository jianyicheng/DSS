#include "doitgenTriple.h"

//------------------------------------------------------------------------
// doitgenTriple
//------------------------------------------------------------------------

// SEPARATOR_FOR_MAIN

#include "doitgenTriple.h"
#include <stdlib.h>

#define N 256

// void doitgenTriple(inout_float_t (&A)[256], in_float_t (&w)[65536],
//                    inout_float_t (&sum)[256]) {
void doitgenTriple(inout_float_t A[256], in_float_t w[65536],
                   inout_float_t sum[256]) {
  int p = 0;

loop_0:
  for (int i = 0; i < N; i++) {
    float s = 0;
  loop_1:
    for (int j = 0; j < N; j++) {
      float a = A[j];
      float wt = w[p + j];
      if (a > 0.0) {
        // s = ss_func_0(s, A[j], w[i][j]);
        // s += (a * wt + wt) * a;
        float b = a * wt;
        float c = b + wt;
        float d = c * a;
        s = s + d;
      }
    }
    p += N;
    sum[i] = s;
  }
// ss_func_1(A, sum);
loop_2:
  for (int i = 0; i < N - 1; i++) {
    float q = sum[i];
    // A[i + 1] = A[i] + (q * q + 0.5f) * q;
    float b = A[i];
    float c = q * q;
    float d = c + 0.5f;
    float e = d * q;
    float f = e + b;
    A[i + 1] = f;
  }
}

int main() {
  float A[N], B[N], sum[N], sum_[N], w[N * N];

  for (int i = 0; i < N; i++) {
    A[i] = (i % 2) ? 1 : -1;
    B[i] = A[i];
    for (int j = 0; j < N; j++)
      w[i * N + j] = 0;
    sum[i] = 0;
    sum_[i] = 0;
  }

  int p = 0;
loop_0:
  for (int i = 0; i < N; i++) {
    float s = 0;
  loop_1:
    for (int j = 0; j < N; j++) {
      float a = B[j];
      float wt = w[p + j];
      if (a > 0.0) {
        s += (a * wt + wt) * a;
      }
    }
    p += N;
    sum_[i] = s;
  }
loop_2:
  for (int i = 0; i < N - 1; i++) {
    float q = sum_[i];
    B[i + 1] = B[i] + q * q * q;
  }

  doitgenTriple(A, w, sum);

  int f = 0;
  for (int i = 0; i < N; i++)
    f += (A[i] == B[i]);

  if (f == N)
    return 0;
  else
    return -1;
}