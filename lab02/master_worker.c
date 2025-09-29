#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define TAG_LEN 1
#define TAG_DATA 2
#define TAG_RESULT 3

int main(int argc, char **argv){
    MPI_Init(&argc,&argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&size);

    if (size < 2) {
        if (rank==0) fprintf(stderr,"Se necesitan al menos 2 procesos (1 master + >=1 worker)\n");
        MPI_Finalize();
        return 1;
    }

    const int TOTAL = 100; // elementos a procesar
    if (rank == 0) {
        int *data = malloc(TOTAL * sizeof(int));
        for (int i=0;i<TOTAL;i++) data[i] = i+1;

        int nworkers = size - 1;
        int base = TOTAL / nworkers;
        int rem  = TOTAL % nworkers;
        int offset = 0;

        for (int w=1; w<=nworkers; ++w){
            int this_chunk = base + (w <= rem ? 1 : 0);
            MPI_Send(&this_chunk, 1, MPI_INT, w, TAG_LEN, MPI_COMM_WORLD);
            if (this_chunk > 0) {
                MPI_Send(&data[offset], this_chunk, MPI_INT, w, TAG_DATA, MPI_COMM_WORLD);
            }
            offset += this_chunk;
        }

        int total_sum = 0;
        for (int w=1; w<=nworkers; ++w){
            int part_sum;
            MPI_Recv(&part_sum, 1, MPI_INT, w, TAG_RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            printf("Master: recibido suma parcial de worker %d = %d\n", w, part_sum);
            total_sum += part_sum;
        }

        printf("Master: suma total = %d (esperada %d)\n", total_sum, (TOTAL*(TOTAL+1))/2);
        free(data);
    } else {
        int chunk;
        MPI_Recv(&chunk, 1, MPI_INT, 0, TAG_LEN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        int *buf = NULL;
        if (chunk > 0) {
            buf = malloc(chunk * sizeof(int));
            MPI_Recv(buf, chunk, MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        int sum = 0;
        for (int i=0;i<chunk;i++) sum += buf[i];
        printf("Worker %d: procesé %d elementos, suma = %d\n", rank, chunk, sum);
        MPI_Send(&sum, 1, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
        if (buf) free(buf);
    }

    MPI_Finalize();
    return 0;
}
