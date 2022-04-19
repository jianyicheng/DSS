
#include <hls_stream.h>

// Function name: xload
// A minimal AXI load component with handshake address and data interfaces.
// addr: address of the operation
// out: loaded data value
void xload(hls::stream<int> &addr, int A[1024], hls::stream<int> &out) {
#pragma HLS INTERFACE m_axi port = A
#pragma HLS PIPELINE
  int in;
  if (addr.empty())
    return;

  addr.read_nb(in);
  out.write_nb(A[in]);
}

// Function name: xstore
// A minimal AXI store component with handshake address and data interfaces.
// addr: address of the operation
// out: data to store
void xstore(hls::stream<int> &addr, hls::stream<int> &data, int A[1024]) {
#pragma HLS INTERFACE m_axi port = A
#pragma HLS PIPELINE
  int addrin, datain;
  if (addr.empty() || data.empty())
    return;
  addr.read_nb(addrin);
  data.read_nb(datain);
  A[addrin] = datain;
}

int main() {
  hls::stream<int> a0, a1, b;
  int A[1024];

  a0.write_nb(10);
  a1.write_nb(10);
  load(a0, A, b);
  store(a1, b, A);
  return 0;
}
