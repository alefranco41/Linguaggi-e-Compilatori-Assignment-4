void f(int *a, int *b, int *c, int *d, int n, int m) {

  int i = 0;
  do{
    a[i] = 0;
    i++;
  }while(i<n);


  i = 0;
  do{
    b[i] = a[i];
    i++;
  }while(i<n);

  i = 0;
  do{
    c[i] = b[i];
    i++;
  }while(i<n);
}