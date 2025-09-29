#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv){
    MPI_Init(&argc,&argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    if (size < 2) {
        if (rank==0) fprintf(stderr, "Ejecutar con al menos 2 procesos\n");
        MPI_Finalize();
        return 1;
    }

    const int NLOOPS = 1000;
    int msg;
    double t0=0.0, t1=0.0;
    if (rank == 0) {
        t0 = MPI_Wtime();
        for (int i=0;i<NLOOPS;i++){
            msg = i;
            MPI_Send(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(&msg, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        t1 = MPI_Wtime();
        double secs = t1 - t0;
        printf("Ping-pong %d iteraciones: tiempo total = %f s, latencia aprox (ida+vuelta) = %g us\n",
               NLOOPS, secs, (secs / NLOOPS) * 1e6);
    } else if (rank == 1) {
        for (int i=0;i<NLOOPS;i++){
            MPI_Recv(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            msg += 1;
            MPI_Send(&msg, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
