int intop(int a, int b, int c, int d) {
#pragma HLS pipeline II = 1
  return (a + b) * (c - d) / a % c;
}

int main() {
  int a = intop(1, 2, 3, 4);

  return 0;
}
