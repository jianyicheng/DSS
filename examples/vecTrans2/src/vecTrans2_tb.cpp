#include "vecTrans2.h"
#include <stdlib.h>

int main(void){
      int A[1024], b[1024];

      for(int j = 0; j < 1024; ++j){
        A[j] = j % 50-25;
        b[j] = rand()%1024;
      }

      vecTrans2(A, b);
}
