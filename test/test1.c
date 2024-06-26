void f(int *a, int *b, int *c, int *d, int n, int m) {

  int i = 0;
  int j = 0;
  int k = 0;

  do{
    a[i] = 0;
    i++;
  }while(i<n);


  do{
    b[j] = a[j];
    j++;
  }while(j<n);

  /*do{
    c[k] = 0;
    k++;
  }while(k<n);*/
}