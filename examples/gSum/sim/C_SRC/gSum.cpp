#include "gSum.h"
// -------------------------------------------
// gSum
// -------------------------------------------
#include <stdlib.h>
#include "gSum.h"

out_int_t gSum(in_int_t A[1000]){
int i, s=0;

for (i = 0; i <1000; i++)
{
  int d = A[i];
  if (d > 0){
    s+=(((((d+2)*d+3)*d+6)*d+2)*d+7)*d+2;
  }
}
return s;

}

int main(void){
   in_int_t A[1000];
   for (int i = 0; i < 1000; i++)
        A[i] = (i%2)?i:-i;
   out_int_t s = gSum(A);
   return 0;
}
