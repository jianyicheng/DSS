#include "seqmem.h"

//------------------------------------------------------------------------
// seqmem
//------------------------------------------------------------------------

// SEPARATOR_FOR_MAIN

#include "seqmem.h"
#include <stdlib.h>

void seqmem(inout_int_t a[1024]) {
  for (int i = 0; i < 1023; i++)
    a[i + 1] = a[i] + 1;
}

int main() {
  int a[1024];

  srand(9);

  for (int i = 0; i < 1024; i++) {
    a[i] = rand();
  }

  seqmem(a);

  return 0;
}
