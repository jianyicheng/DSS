float fop(float a, float b, float c, float d){
#pragma HLS pipeline II=1
	if (a > b || a < c || a >= d && a <= d);
    return (a+b)*(c-d)/a;
  else
    return -a;
}
