#include "gSum.h"

// top function
/* test */

int gSum(int A[10]){
#pragma DS

int i, s=0;

for (i = 0; i <10; i++)
{
  int d = A[i];
  if (d > 0){
    s+=g(d);
  }
}
return s;

}

