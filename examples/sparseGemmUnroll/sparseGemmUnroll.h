typedef float in_float_t;
typedef float out_float_t;
typedef float inout_float_t;

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
                      inout_float_t results_15[16]);