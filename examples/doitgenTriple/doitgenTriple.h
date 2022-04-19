typedef float in_float_t;
typedef float out_float_t;
typedef float inout_float_t;

// void doitgenTriple(inout_float_t A[64], in_float_t w[4096],
//                    inout_float_t sum[64]);

void doitgenTriple(inout_float_t A[256], in_float_t w[65536],
                   inout_float_t sum[256]);
// void doitgenTriple(inout_float_t (&A)[256], in_float_t (&w)[65536],
//                    inout_float_t (&sum)[256]);