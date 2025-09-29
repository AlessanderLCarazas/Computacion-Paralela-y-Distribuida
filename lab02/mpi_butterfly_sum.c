#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Verifica si un número es potencia de 2
int is_power_of_two(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Valor inicial de cada proceso (ejemplo: rank+1)
    int local_val = rank + 1;
    int sum = local_val;

    if (is_power_of_two(size)) {
        // Caso 1: número de procesos es potencia de 2
        int steps = (int)log2(size);
        for (int s = 0; s < steps; s++) {
            int partner = rank ^ (1 << s);
            int recv_val;
            MPI_Sendrecv(&sum, 1, MPI_INT, partner, 0,
                         &recv_val, 1, MPI_INT, partner, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            sum += recv_val;
        }
    } else {
        // Caso 2: número de procesos no es potencia de 2
        // Adaptación: reducimos al mayor potencia de 2 < size
        int pow2 = 1;
        while (pow2 * 2 <= size) pow2 *= 2;

        // Procesos extra envían su valor a un "socio" dentro de la potencia de 2
        if (rank >= pow2) {
            MPI_Send(&sum, 1, MPI_INT, rank - pow2, 0, MPI_COMM_WORLD);
            // Reciben luego la suma global de su socio
            MPI_Recv(&sum, 1, MPI_INT, rank - pow2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            if (rank + pow2 < size) {
                int recv_val;
                MPI_Recv(&recv_val, 1, MPI_INT, rank + pow2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                sum += recv_val;
            }

            // Ahora ejecutamos butterfly normal con pow2 procesos
            int steps = (int)log2(pow2);
            for (int s = 0; s < steps; s++) {
                int partner = rank ^ (1 << s);
                int recv_val;
                MPI_Sendrecv(&sum, 1, MPI_INT, partner, 0,
                             &recv_val, 1, MPI_INT, partner, 0,
                             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                sum += recv_val;
            }

            // Finalmente enviamos suma global de vuelta a los procesos extra
            if (rank + pow2 < size) {
                MPI_Send(&sum, 1, MPI_INT, rank + pow2, 0, MPI_COMM_WORLD);
            }
        }
    }

    // Ahora TODOS los procesos tienen la suma global
    printf("Proceso %d: suma global = %d\n", rank, sum);

    MPI_Finalize();
    return 0;
}

