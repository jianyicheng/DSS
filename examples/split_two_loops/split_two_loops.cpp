//------------------------------------------------------------------------
// split_two_loops
//------------------------------------------------------------------------

#include "split_two_loops.h"
#include <stdlib.h>

#define NUM 128

// Cannot use the name "buffer" as Vitis HLS renames "buffer" to "buffer_r" and
// causes DASS to fail.
void loop1(int din[NUM], int buff[NUM]) {
#pragma DASS SS
  for (int i = 0; i < NUM; i++) {
    buff[i] = din[i] * 3;
  }
}

void loop2(int dout[NUM], int buff[NUM]) {
#pragma DASS SS
  for (int i = 0; i < NUM; i++) {
    dout[i] = buff[i] * 2;
  }
}

void split_two_loops(in_int_t din[128], out_int_t dout[128],
                     inout_int_t buff[128]) {

  loop1(din, buff);
  loop2(dout, buff);
}

int main() {
  srand(9);

  int din[128], dout[128], dgold[128], buff[128], bgold[128];
  for (int i = 0; i < 128; i++) {
    din[i] = rand();
    dout[i] = 0;
    dgold[i] = 0;
    buff[i] = 0;
    bgold[i] = 0;
  }

  {
    for (int i = 0; i < NUM; i++) {
      buff[i] = din[i] * 3;
    }
    for (int i = 0; i < NUM; i++) {
      dout[i] = buff[i] * 2;
    }
  }

  split_two_loops(din, dgold, bgold);

  int check = 0;
  for (int i = 0; i < NUM; i++)
    check += (dout[i] == dgold[i]);

  if (check == NUM)
    return 0;
  else
    return check;
}
