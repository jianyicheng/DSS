double dop(double a, double b, double c, double d){
#pragma HLS pipeline II=1
	if (a > b || a < c || a >= d && a <= d)
    return (a+b)*(c-d)/a;
  else
    return -a;
}
