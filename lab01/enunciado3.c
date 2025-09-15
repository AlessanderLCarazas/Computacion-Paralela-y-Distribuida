#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 1000

double A[MAX][MAX], B[MAX][MAX], C[MAX][MAX];

void multiplicacion_bloques(int n, int block) {
    int i, j, k, ii, jj, kk;
    clock_t start, end;
    double cpu_time;

    // Inicializar
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++) {
            A[i][j] = (double)(i + j) / n;
            B[i][j] = (double)(i - j) / n;
            C[i][j] = 0.0;
        }

    start = clock();

    for (ii = 0; ii < n; ii += block)
        for (jj = 0; jj < n; jj += block)
            for (kk = 0; kk < n; kk += block)
                for (i = ii; i < ii + block && i < n; i++)
                    for (j = jj; j < jj + block && j < n; j++)
                        for (k = kk; k < kk + block && k < n; k++)
                            C[i][j] += A[i][k] * B[k][j];

    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tiempo bloques (%dx%d, block=%d): %f segundos\n", n, n, block, cpu_time);
}

int main() {
    int sizes[] = {200, 500, 800, 1000};
    int blocks[] = {16, 32, 64};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    int num_blocks = sizeof(blocks) / sizeof(blocks[0]);

    for (int bi = 0; bi < num_blocks; bi++) {
        int block = blocks[bi];
        for (int si = 0; si < num_sizes; si++) {
            int n = sizes[si];
            multiplicacion_bloques(n, block);
        }
    }
    return 0;
}
