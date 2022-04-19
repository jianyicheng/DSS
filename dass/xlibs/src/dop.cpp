double dop(double a, double b, double c, double d) {
#pragma HLS pipeline II = 1
  if ((a > b || a < c) || (a >= d && a <= d))
    return (a + b) * (c - d) / a;
  else
    return -a;
}

int main() {
  double s = dop(1.0, 0, 0, 0);
  if (s == 0.0)
    return 0;
  else
    return -1;
}
