#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int is_power_of_two(int n) {
    return (n > 0) && ((n & (n - 1)) == 0);
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Cada proceso empieza con un valor local (ejemplo: su rank+1)
    int local_val = rank + 1;
    int sum = local_val;

    if (is_power_of_two(size)) {
        // Caso 1: número de procesos es potencia de 2
        for (int step = 1; step < size; step *= 2) {
            if (rank % (2 * step) == 0) {
                int recv_val;
                MPI_Recv(&recv_val, 1, MPI_INT, rank + step, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                sum += recv_val;
            } else if (rank % (2 * step) == step) {
                MPI_Send(&sum, 1, MPI_INT, rank - step, 0, MPI_COMM_WORLD);
                break; // sale del bucle después de enviar
            }
        }
    } else {
        // Caso 2: número de procesos NO es potencia de 2
        // Encontrar la mayor potencia de 2 menor que size
        int pow2 = 1;
        while (pow2 * 2 <= size) pow2 *= 2;

        if (rank >= pow2) {
            // Procesos "extra" envían su valor a (rank - pow2)
            MPI_Send(&sum, 1, MPI_INT, rank - pow2, 0, MPI_COMM_WORLD);
        } else {
            if (rank + pow2 < size) {
                int recv_val;
                MPI_Recv(&recv_val, 1, MPI_INT, rank + pow2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                sum += recv_val;
            }

            // Luego ejecutamos el algoritmo normal de árbol con pow2 procesos
            for (int step = 1; step < pow2; step *= 2) {
                if (rank % (2 * step) == 0) {
                    int recv_val;
                    MPI_Recv(&recv_val, 1, MPI_INT, rank + step, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    sum += recv_val;
                } else if (rank % (2 * step) == step) {
                    MPI_Send(&sum, 1, MPI_INT, rank - step, 0, MPI_COMM_WORLD);
                    break;
                }
            }
        }
    }

    // Solo el proceso 0 imprime el resultado final
    if (rank == 0) {
        printf("\n=== Tree-Structured Global Sum ===\n");
        printf("Número de procesos = %d\n", size);
        printf("Suma global        = %d\n", sum);
    }

    MPI_Finalize();
    return 0;
}

