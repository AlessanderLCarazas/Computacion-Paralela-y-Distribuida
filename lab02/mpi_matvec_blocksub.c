#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Genera matriz y vector de ejemplo
void generate_matrix_vector(double *A, double *x, int n) {
    for (int i = 0; i < n*n; i++) {
        A[i] = 1.0; // Para debug más fácil (matriz llena de 1s)
    }
    for (int i = 0; i < n; i++) {
        x[i] = 1.0; // Vector lleno de 1s
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, comm_sz;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    // Asegurar que comm_sz es un cuadrado perfecto
    int q = (int)sqrt(comm_sz);
    if (q * q != comm_sz) {
        if (rank == 0) printf("El número de procesos debe ser un cuadrado perfecto.\n");
        MPI_Finalize();
        return 0;
    }

    int n = 4; // Orden de la matriz (ejemplo)
    if (argc > 1) n = atoi(argv[1]);

    if (n % q != 0) {
        if (rank == 0) printf("n debe ser divisible por sqrt(comm_sz).\n");
        MPI_Finalize();
        return 0;
    }

    int local_n = n / q; // tamaño del bloque
    double *local_A = (double*)malloc(local_n * local_n * sizeof(double));
    double *local_x = (double*)malloc(local_n * sizeof(double));
    double *local_y = (double*)calloc(local_n, sizeof(double));

    double *A = NULL;
    double *x = NULL;

    if (rank == 0) {
        A = (double*)malloc(n * n * sizeof(double));
        x = (double*)malloc(n * sizeof(double));
        generate_matrix_vector(A, x, n);
    }

    // Crear comunicador cartesiano 2D
    int dims[2] = {q, q};
    int periods[2] = {0, 0};
    MPI_Comm grid_comm;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &grid_comm);

    int coords[2];
    MPI_Cart_coords(grid_comm, rank, 2, coords);
    int row = coords[0];
    int col = coords[1];

    // Scatter de la matriz en sub-bloques
    if (rank == 0) {
        // Armar bloques y mandarlos
        for (int pr = 0; pr < comm_sz; pr++) {
            int coords_pr[2];
            MPI_Cart_coords(grid_comm, pr, 2, coords_pr);
            int r_off = coords_pr[0] * local_n;
            int c_off = coords_pr[1] * local_n;

            double *subblock = (double*)malloc(local_n * local_n * sizeof(double));
            for (int i = 0; i < local_n; i++) {
                for (int j = 0; j < local_n; j++) {
                    subblock[i*local_n + j] = A[(r_off+i)*n + (c_off+j)];
                }
            }

            if (pr == 0) {
                for (int i = 0; i < local_n*local_n; i++) local_A[i] = subblock[i];
            } else {
                MPI_Send(subblock, local_n*local_n, MPI_DOUBLE, pr, 0, MPI_COMM_WORLD);
            }
            free(subblock);
        }
    } else {
        MPI_Recv(local_A, local_n*local_n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Distribuir vector x: cada diagonal recibe su bloque
    if (row == col) {
        if (rank == 0) {
            for (int pr = 0; pr < comm_sz; pr++) {
                int coords_pr[2];
                MPI_Cart_coords(grid_comm, pr, 2, coords_pr);
                if (coords_pr[0] == coords_pr[1]) {
                    int start = coords_pr[0] * local_n;
                    if (pr == 0) {
                        for (int i = 0; i < local_n; i++) local_x[i] = x[start+i];
                    } else {
                        MPI_Send(&x[start], local_n, MPI_DOUBLE, pr, 0, MPI_COMM_WORLD);
                    }
                }
            }
        } else {
            MPI_Recv(local_x, local_n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    // Broadcast del vector en la fila de la cuadrícula
    MPI_Comm row_comm;
    MPI_Comm_split(grid_comm, row, col, &row_comm);

    MPI_Bcast(local_x, local_n, MPI_DOUBLE, row, row_comm);

    // Multiplicación local: local_y = local_A * local_x
    for (int i = 0; i < local_n; i++) {
        for (int j = 0; j < local_n; j++) {
            local_y[i] += local_A[i*local_n + j] * local_x[j];
        }
    }

    // Reducción por fila para formar la parte del vector resultante
    double *row_result = (double*)calloc(local_n, sizeof(double));
    MPI_Reduce(local_y, row_result, local_n, MPI_DOUBLE, MPI_SUM, row, row_comm);

    // Recolectar resultados en proceso 0
    if (col == row) {
        if (rank == 0) {
            double *y = (double*)malloc(n * sizeof(double));
            for (int i = 0; i < local_n; i++) y[i] = row_result[i];

            for (int pr = 1; pr < comm_sz; pr++) {
                int coords_pr[2];
                MPI_Cart_coords(grid_comm, pr, 2, coords_pr);
                if (coords_pr[0] == coords_pr[1]) {
                    MPI_Recv(&y[coords_pr[0]*local_n], local_n, MPI_DOUBLE, pr, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }

            printf("Resultado y = [");
            for (int i = 0; i < n; i++) printf(" %.1f", y[i]);
            printf(" ]\n");

            free(y);
        } else {
            MPI_Send(row_result, local_n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
    }

    free(local_A); free(local_x); free(local_y); free(row_result);
    if (rank == 0) { free(A); free(x); }

    MPI_Finalize();
    return 0;
}

