#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 5000

double A[MAX][MAX], x[MAX], y[MAX];

int main() {
    int i, j;
    clock_t start, end;
    double cpu_time;

    for (i = 0; i < MAX; i++) {
        x[i] = 1.0;
        y[i] = 0.0;
        for (j = 0; j < MAX; j++) {
            A[i][j] = (double)(i + j) / MAX;
        }
    }

    start = clock();
    for (i = 0; i < MAX; i++)
        for (j = 0; j < MAX; j++)
            y[i] += A[i][j] * x[j];
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tiempo primer par de bucles: %f segundos\n", cpu_time);

    for (i = 0; i < MAX; i++) y[i] = 0.0;

    start = clock();
    for (j = 0; j < MAX; j++)
        for (i = 0; i < MAX; i++)
            y[i] += A[i][j] * x[j];
    end = clock();
    cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Tiempo segundo par de bucles: %f segundos\n", cpu_time);

    return 0;
}
