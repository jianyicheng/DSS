#include "offchip.h"

//------------------------------------------------------------------------
// offchip
//------------------------------------------------------------------------

// SEPARATOR_FOR_MAIN

#include "offchip.h"
#include <stdlib.h>

void offchip(in_int_t a[1024], in_int_t c[1024], out_int_t b[1024]) {
#pragma HLS INTERFACE m_axi port=a depth=100
#pragma HLS INTERFACE m_axi port=b depth=100
  for (int i = 0; i < 1024; i++)
    b[i] = a[i] + c[i];
}

int main() {
  int a[1024], b[1024], c[1024];

  srand(9);

  for (int i = 0; i < 1024; i++) {
    a[i] = rand();
    c[i] = rand();
  }

  offchip(a, b, c);

  return 0;
}
