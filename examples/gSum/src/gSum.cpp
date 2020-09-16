#include "gSum.h"

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
