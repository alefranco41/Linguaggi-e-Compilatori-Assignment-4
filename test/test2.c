void f(int **a, int **b, int **c, int **d, int n) {
    for(int i=0; i<n; i++){
        for(int j=0; j<n; j++){
            a[i][j] = 1/b[i][j]*c[i][j];
        }
    }

    for(int i=0; i<n; i++){
        for(int j=0; j<n; j++){
            d[i][j] = a[i][j]+c[i][j];
        }
    }
}   