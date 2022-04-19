//------------------------------------------------------------------------
// sparseGemmUnroll
//------------------------------------------------------------------------

#include "sparseGemmUnroll.h"
#include <stdio.h>
#include <stdlib.h>

void sparseGemmUnroll(in_float_t data[4096], in_float_t weight_0[256],
                      in_float_t weight_1[256], in_float_t weight_2[256],
                      in_float_t weight_3[256], in_float_t weight_4[256],
                      in_float_t weight_5[256], in_float_t weight_6[256],
                      in_float_t weight_7[256], in_float_t weight_8[256],
                      in_float_t weight_9[256], in_float_t weight_10[256],
                      in_float_t weight_11[256], in_float_t weight_12[256],
                      in_float_t weight_13[256], in_float_t weight_14[256],
                      in_float_t weight_15[256], inout_float_t results_0[16],
                      inout_float_t results_1[16], inout_float_t results_2[16],
                      inout_float_t results_3[16], inout_float_t results_4[16],
                      inout_float_t results_5[16], inout_float_t results_6[16],
                      inout_float_t results_7[16], inout_float_t results_8[16],
                      inout_float_t results_9[16], inout_float_t results_10[16],
                      inout_float_t results_11[16],
                      inout_float_t results_12[16],
                      inout_float_t results_13[16],
                      inout_float_t results_14[16],
                      inout_float_t results_15[16]) {
#pragma HLS interface ap_memory storage_type = RAM_S2P port = data
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_0
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_1
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_2
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_3
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_4
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_5
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_6
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_7
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_8
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_9
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_10
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_11
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_12
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_13
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_14
#pragma HLS interface ap_memory storage_type = RAM_S2P port = weight_15
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_0
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_1
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_2
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_3
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_4
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_5
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_6
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_7
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_8
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_9
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_10
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_11
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_12
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_13
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_14
#pragma HLS interface ap_memory storage_type = RAM_S2P port = results_15

  int ii = 0;
  for (int i = 0; i < 256; i++) {
    for (int j = 0; j < 16; j++) {
      float d = data[ii + j];

	// data = 0, 0, 0, 1, 0, 0, 0, 1
	// result_0[j] = (d != 0.0f) ? 0 : ...

      if (d != 0.0f) {
        results_0[j] += d * weight_0[i];
        results_1[j] += d * weight_1[i];
        results_2[j] += d * weight_2[i];
        results_3[j] += d * weight_3[i];
        results_4[j] += d * weight_4[i];
        results_5[j] += d * weight_5[i];
        results_6[j] += d * weight_6[i];
        results_7[j] += d * weight_7[i];
        results_8[j] += d * weight_8[i];
        results_9[j] += d * weight_9[i];
        results_10[j] += d * weight_10[i];
        results_11[j] += d * weight_11[i];
        results_12[j] += d * weight_12[i];
        results_13[j] += d * weight_13[i];
        results_14[j] += d * weight_14[i];
        results_15[j] += d * weight_15[i];
      }
    }
    ii += 16;
  }
}

int main() {
  in_float_t data[4096];
  in_float_t weight_0[256];
  in_float_t weight_1[256];
  in_float_t weight_2[256];
  in_float_t weight_3[256];
  in_float_t weight_4[256];
  in_float_t weight_5[256];
  in_float_t weight_6[256];
  in_float_t weight_7[256];
  in_float_t weight_8[256];
  in_float_t weight_9[256];
  in_float_t weight_10[256];
  in_float_t weight_11[256];
  in_float_t weight_12[256];
  in_float_t weight_13[256];
  in_float_t weight_14[256];
  in_float_t weight_15[256];
  inout_float_t results_0[16];
  inout_float_t results_1[16];
  inout_float_t results_2[16];
  inout_float_t results_3[16];
  inout_float_t results_4[16];
  inout_float_t results_5[16];
  inout_float_t results_6[16];
  inout_float_t results_7[16];
  inout_float_t results_8[16];
  inout_float_t results_9[16];
  inout_float_t results_10[16];
  inout_float_t results_11[16];
  inout_float_t results_12[16];
  inout_float_t results_13[16];
  inout_float_t results_14[16];
  inout_float_t results_15[16];
  float results_0_[16];
  float results_1_[16];
  float results_2_[16];
  float results_3_[16];
  float results_4_[16];
  float results_5_[16];
  float results_6_[16];
  float results_7_[16];
  float results_8_[16];
  float results_9_[16];
  float results_10_[16];
  float results_11_[16];
  float results_12_[16];
  float results_13_[16];
  float results_14_[16];
  float results_15_[16];

  srand(9);

  for (int i = 0; i < 4096; i++)
    data[i] = (i % 4 == 0) ? 0.1f : 0.0f;

  for (int i = 0; i < 16; i++) {
    weight_0[i] = rand() / 1000.0f;
    weight_1[i] = rand() / 1000.0f;
    weight_2[i] = rand() / 1000.0f;
    weight_3[i] = rand() / 1000.0f;
    weight_4[i] = rand() / 1000.0f;
    weight_5[i] = rand() / 1000.0f;
    weight_6[i] = rand() / 1000.0f;
    weight_7[i] = rand() / 1000.0f;
    weight_8[i] = rand() / 1000.0f;
    weight_9[i] = rand() / 1000.0f;
    weight_10[i] = rand() / 1000.0f;
    weight_11[i] = rand() / 1000.0f;
    weight_12[i] = rand() / 1000.0f;
    weight_13[i] = rand() / 1000.0f;
    weight_14[i] = rand() / 1000.0f;
    weight_15[i] = rand() / 1000.0f;
    results_0[i] = 0.0f;
    results_1[i] = 0.0f;
    results_2[i] = 0.0f;
    results_3[i] = 0.0f;
    results_4[i] = 0.0f;
    results_5[i] = 0.0f;
    results_6[i] = 0.0f;
    results_7[i] = 0.0f;
    results_8[i] = 0.0f;
    results_9[i] = 0.0f;
    results_10[i] = 0.0f;
    results_11[i] = 0.0f;
    results_12[i] = 0.0f;
    results_13[i] = 0.0f;
    results_14[i] = 0.0f;
    results_15[i] = 0.0f;
    results_0_[i] = 0.0f;
    results_1_[i] = 0.0f;
    results_2_[i] = 0.0f;
    results_3_[i] = 0.0f;
    results_4_[i] = 0.0f;
    results_5_[i] = 0.0f;
    results_6_[i] = 0.0f;
    results_7_[i] = 0.0f;
    results_8_[i] = 0.0f;
    results_9_[i] = 0.0f;
    results_10_[i] = 0.0f;
    results_11_[i] = 0.0f;
    results_12_[i] = 0.0f;
    results_13_[i] = 0.0f;
    results_14_[i] = 0.0f;
    results_15_[i] = 0.0f;
  }

  // {
  //   int ii = 0;
  //   for (int i = 0; i < 256; i++) {
  //     for (int j = 0; j < 16; j++) {
  //       float d = data[ii + j];
  //       if (d != 0.0f) {
  //         results_0_[j] += d * weight_0[i];
  //         results_1_[j] += d * weight_1[i];
  //         results_2_[j] += d * weight_2[i];
  //         results_3_[j] += d * weight_3[i];
  //         results_4_[j] += d * weight_4[i];
  //         results_5_[j] += d * weight_5[i];
  //         results_6_[j] += d * weight_6[i];
  //         results_7_[j] += d * weight_7[i];
  //         results_8_[j] += d * weight_8[i];
  //         results_9_[j] += d * weight_9[i];
  //         results_10_[j] += d * weight_10[i];
  //         results_11_[j] += d * weight_11[i];
  //         results_12_[j] += d * weight_12[i];
  //         results_13_[j] += d * weight_13[i];
  //         results_14_[j] += d * weight_14[i];
  //         results_15_[j] += d * weight_15[i];
  //       }
  //     }
  //     ii += 16;
  //   }
  // }

  sparseGemmUnroll(data, weight_0, weight_1, weight_2, weight_3, weight_4,
                   weight_5, weight_6, weight_7, weight_8, weight_9, weight_10,
                   weight_11, weight_12, weight_13, weight_14, weight_15,
                   results_0, results_1, results_2, results_3, results_4,
                   results_5, results_6, results_7, results_8, results_9,
                   results_10, results_11, results_12, results_13, results_14,
                   results_15);

  // float sumCheck = 0.0f;
  // for (int j = 0; j < 16; j++) {
  //   sumCheck += results_0[j];
  //   sumCheck += results_1[j];
  //   sumCheck += results_2[j];
  //   sumCheck += results_3[j];
  //   sumCheck += results_4[j];
  //   sumCheck += results_5[j];
  //   sumCheck += results_6[j];
  //   sumCheck += results_7[j];
  //   sumCheck += results_8[j];
  //   sumCheck += results_9[j];
  //   sumCheck += results_10[j];
  //   sumCheck += results_11[j];
  //   sumCheck += results_12[j];
  //   sumCheck += results_13[j];
  //   sumCheck += results_14[j];
  //   sumCheck += results_15[j];
  //   sumCheck -= results_0_[j];
  //   sumCheck -= results_1_[j];
  //   sumCheck -= results_2_[j];
  //   sumCheck -= results_3_[j];
  //   sumCheck -= results_4_[j];
  //   sumCheck -= results_5_[j];
  //   sumCheck -= results_6_[j];
  //   sumCheck -= results_7_[j];
  //   sumCheck -= results_8_[j];
  //   sumCheck -= results_9_[j];
  //   sumCheck -= results_10_[j];
  //   sumCheck -= results_11_[j];
  //   sumCheck -= results_12_[j];
  //   sumCheck -= results_13_[j];
  //   sumCheck -= results_14_[j];
  //   sumCheck -= results_15_[j];
  // }
  // printf("%f\n", sumCheck);

  return 0;
}
