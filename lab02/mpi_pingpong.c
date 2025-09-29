#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PINGPONG_LIMIT 100000  // número de iteraciones para medir tiempos

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2) {
        if (rank == 0) printf("Este programa necesita al menos 2 procesos.\n");
        MPI_Finalize();
        return 0;
    }

    int partner = (rank == 0) ? 1 : 0; // procesos que se comunican
    int msg = 1;

    // ==== Medición con clock() ====
    clock_t start_clock, end_clock;
    start_clock = clock();

    for (int i = 0; i < PINGPONG_LIMIT; i++) {
        if (rank == 0) {
            MPI_Send(&msg, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
            MPI_Recv(&msg, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if (rank == 1) {
            MPI_Recv(&msg, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(&msg, 1, MPI_INT, partner, 0, MPI_COMM_WORLD);
        }
    }

    end_clock = clock();
    double elapsed_clock = ((double)(end_clock - start_clock)) / CLOCKS_PER_SEC;

    // ==== Medición con MPI_Wtime() ====
    MPI_Barrier(MPI_COMM_WORLD);
    double start_wtime = MPI_Wtime();

    for (int i = 0; i < PINGPONG_LIMIT; i++) {
        if (rank == 0) {
            MPI_Send(&msg, 1, MPI_INT, partner, 1, MPI_COMM_WORLD);
            MPI_Recv(&msg, 1, MPI_INT, partner, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if (rank == 1) {
            MPI_Recv(&msg, 1, MPI_INT, partner, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(&msg, 1, MPI_INT, partner, 1, MPI_COMM_WORLD);
        }
    }

    double end_wtime = MPI_Wtime();
    double elapsed_wtime = end_wtime - start_wtime;

    if (rank == 0) {
        printf("Ping-Pong con %d iteraciones\n", PINGPONG_LIMIT);
        printf("Tiempo medido con clock():   %f segundos\n", elapsed_clock);
        printf("Tiempo medido con MPI_Wtime(): %f segundos\n", elapsed_wtime);
        printf("Tiempo promedio por ping-pong (MPI_Wtime): %e segundos\n",
               elapsed_wtime / PINGPONG_LIMIT);
    }

    MPI_Finalize();
    return 0;
}

