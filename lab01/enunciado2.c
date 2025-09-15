#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 1000

double A[MAX][MAX], B[MAX][MAX], C[MAX][MAX];

void multiplicacion(int n) {
    int i, j, k;
    clock_t start, end;
    double cpu_time;

    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++) {
            A[i][j] = (double)(i + j) / n;
            B[i][j] = (double)(i - j) / n;
            C[i][j] = 0.0;
        }

    start = clock();
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            for (k = 0; k < n; k++)
                C[i][j] += A[i][k] * B[k][j];
    end = clock();

    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tiempo multiplicacion clasica (%dx%d): %f segundos\n", n, n, cpu_time);
}

int main() {
    int sizes[] = {100, 500, 1000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int i = 0; i < num_sizes; i++)
        multiplicacion(sizes[i]);

    return 0;
}
