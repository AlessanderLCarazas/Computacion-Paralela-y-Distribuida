#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = 4;  // tamaño de la matriz (ejemplo: 4x4)
    if (rank == 0) {
        if (argc >= 2) {
            n = atoi(argv[1]); // permitir pasar n por argumento
        }
    }

    // Difundir tamaño de la matriz a todos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Verificar divisibilidad
    if (n % size != 0) {
        if (rank == 0) {
            printf("Error: n=%d no es divisible entre el número de procesos=%d\n", n, size);
        }
        MPI_Finalize();
        return 0;
    }

    int local_cols = n / size;   // columnas por proceso
    double* local_A = (double*)malloc(n * local_cols * sizeof(double));
    double* local_x = (double*)malloc(local_cols * sizeof(double));

    double* A = NULL;
    double* x = NULL;
    if (rank == 0) {
        // Crear matriz y vector de ejemplo
        A = (double*)malloc(n * n * sizeof(double));
        x = (double*)malloc(n * sizeof(double));

        for (int i = 0; i < n; i++) {
            x[i] = 1.0; // vector [1,1,1,...]
            for (int j = 0; j < n; j++) {
                A[i*n + j] = i + j + 1; // solo para prueba
            }
        }
        printf("\nMatriz A:\n");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                printf("%5.1f ", A[i*n + j]);
            }
            printf("\n");
        }
        printf("Vector x:\n");
        for (int i = 0; i < n; i++) printf("%5.1f ", x[i]);
        printf("\n");
    }

    // Scatter columnas de la matriz
    if (rank == 0) {
        for (int p = 0; p < size; p++) {
            if (p == 0) {
                for (int j = 0; j < local_cols; j++) {
                    for (int i = 0; i < n; i++) {
                        local_A[i*local_cols + j] = A[i*n + j];
                    }
                }
                for (int j = 0; j < local_cols; j++) {
                    local_x[j] = x[j];
                }
            } else {
                double* tempA = (double*)malloc(n * local_cols * sizeof(double));
                double* tempx = (double*)malloc(local_cols * sizeof(double));
                for (int j = 0; j < local_cols; j++) {
                    for (int i = 0; i < n; i++) {
                        tempA[i*local_cols + j] = A[i*n + (p*local_cols + j)];
                    }
                    tempx[j] = x[p*local_cols + j];
                }
                MPI_Send(tempA, n*local_cols, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
                MPI_Send(tempx, local_cols, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
                free(tempA);
                free(tempx);
            }
        }
    } else {
        MPI_Recv(local_A, n*local_cols, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(local_x, local_cols, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Calcular contribución local
    double* local_y = (double*)calloc(n, sizeof(double));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < local_cols; j++) {
            local_y[i] += local_A[i*local_cols + j] * local_x[j];
        }
    }

    // Reducir contribuciones en proceso 0
    double* y = NULL;
    if (rank == 0) {
        y = (double*)malloc(n * sizeof(double));
    }

    MPI_Reduce(local_y, y, n, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\nResultado y = A*x:\n");
        for (int i = 0; i < n; i++) {
            printf("%5.1f ", y[i]);
        }
        printf("\n");
    }

    free(local_A);
    free(local_x);
    free(local_y);
    if (rank == 0) {
        free(A);
        free(x);
        free(y);
    }

    MPI_Finalize();
    return 0;
}

