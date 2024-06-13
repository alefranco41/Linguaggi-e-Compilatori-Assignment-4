void f(int *a, int *b, int *c, int *d, int n, int m) {
  
  for (int i=0; i<n; i++) {
    a[i] = b[i] + c[i];
  }

  n = 10;

  for (int i=0; i<n; i++) {
    d[i] = a[i] + d[i];
  }
}