#include "foo.h"

//------------------------------------------------------------------------
// Foo
//------------------------------------------------------------------------

// SEPARATOR_FOR_MAIN

#include "foo.h"
#include <stdlib.h>

#define N 1024
#define M N / 2

out_int_t foo(in_int_t A[N]) {
  int s = 0;
  for (int i = 0; i < N; i++) {
    if (A[i] < M)
      s += A[i] * A[i] + A[i];
  }
  return s;
}

int main() {
  int A[N], gold = 0;
  for (int i = 0; i < N; i++) {
    A[i] = (i % 2) ? M + 1 : M - 1;
    if (A[i] < M)
      gold += A[i] * A[i] + A[i];
  }

  int s = foo(A);

  if (s == gold)
    return 0;
  else
    return -1;
}
