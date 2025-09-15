#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 1000

double A[MAX][MAX], B[MAX][MAX], C[MAX][MAX];

void inicializar(int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            A[i][j] = (double)(i + j) / n;
            B[i][j] = (double)(i - j) / n;
            C[i][j] = 0.0;
        }
}

void multiplicacion_clasica(int n) {
    inicializar(n);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            for (int k = 0; k < n; k++)
                C[i][j] += A[i][k] * B[k][j];
}

void multiplicacion_bloques(int n, int block) {
    inicializar(n);
    for (int ii = 0; ii < n; ii += block)
        for (int jj = 0; jj < n; jj += block)
            for (int kk = 0; kk < n; kk += block)
                for (int i = ii; i < ii + block && i < n; i++)
                    for (int j = jj; j < jj + block && j < n; j++)
                        for (int k = kk; k < kk + block && k < n; k++)
                            C[i][j] += A[i][k] * B[k][j];
}

int main() {
    int n = 500;
    int block = 32;

    clock_t start, end;
    double t;

    start = clock();
    multiplicacion_clasica(n);
    end = clock();
    t = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tiempo clasica (%dx%d): %f segundos\n", n, n, t);

    start = clock();
    multiplicacion_bloques(n, block);
    end = clock();
    t = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tiempo bloques (%dx%d, block=%d): %f segundos\n", n, n, block, t);

    return 0;
}
