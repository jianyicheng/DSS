#include "gSum.h"

// top function
/* test */

int gSum(int A[1000]){
#pragma DS

int i, s=0;

for (i = 0; i <1000; i++)
{
  int d = A[i];
  if (d > 0){
    s+=g(d);
  }
}
return s;

}

int main(){
   int A[1000];
   for (int i = 0; i < 1000; i++)
        A[i] = (i%2)?i:-i;
   int s = gSum(A);
   return 0;
}
