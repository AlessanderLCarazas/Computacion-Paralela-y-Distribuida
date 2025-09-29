#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    int rank, size, n = 16;  // Tamaño del vector global (ejemplo n=16)
    int *global_vector = NULL;
    double start, end;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Solo el proceso 0 inicializa el vector global
    if (rank == 0) {
        global_vector = (int*) malloc(n * sizeof(int));
        for (int i = 0; i < n; i++) {
            global_vector[i] = i + 1;  // Vector 1..n
        }
        printf("Vector inicial (1..n):\n");
        for (int i = 0; i < n; i++) printf("%d ", global_vector[i]);
        printf("\n\n");
    }

    // -------------------------------
    // BLOCK DISTRIBUTION
    // -------------------------------
    int block_size = n / size;
    int *block_part = (int*) malloc(block_size * sizeof(int));

    MPI_Scatter(global_vector, block_size, MPI_INT,
                block_part, block_size, MPI_INT,
                0, MPI_COMM_WORLD);

    // -------------------------------
    // BLOCK -> CYCLIC
    // -------------------------------
    int cyclic_size = (n + size - 1) / size; // en caso no divisible exacto
    int *cyclic_part = (int*) malloc(cyclic_size * sizeof(int));

    start = MPI_Wtime();

    // Enviar desde block a cada proceso en orden cíclico
    for (int i = 0; i < block_size; i++) {
        int global_index = rank * block_size + i;
        int target_proc = global_index % size;
        int pos_in_target = global_index / size;
        MPI_Send(&block_part[i], 1, MPI_INT, target_proc, pos_in_target, MPI_COMM_WORLD);
    }

    // Recibir los datos correspondientes a la distribución cíclica
    int recv_count = 0;
    for (int i = 0; i < cyclic_size; i++) {
        MPI_Status status;
        if (rank + i * size < n) {
            MPI_Recv(&cyclic_part[i], 1, MPI_INT, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &status);
            recv_count++;
        }
    }

    end = MPI_Wtime();
    if (rank == 0) {
        printf("Tiempo BLOCK -> CYCLIC: %f segundos\n", end - start);
    }

    // -------------------------------
    // CYCLIC -> BLOCK
    // -------------------------------
    int *block_part_back = (int*) malloc(block_size * sizeof(int));

    start = MPI_Wtime();

    for (int i = 0; i < recv_count; i++) {
        int global_index = i * size + rank;
        int target_proc = global_index / block_size;
        int pos_in_target = global_index % block_size;
        MPI_Send(&cyclic_part[i], 1, MPI_INT, target_proc, pos_in_target, MPI_COMM_WORLD);
    }

    for (int i = 0; i < block_size; i++) {
        MPI_Status status;
        MPI_Recv(&block_part_back[i], 1, MPI_INT, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &status);
    }

    end = MPI_Wtime();
    if (rank == 0) {
        printf("Tiempo CYCLIC -> BLOCK: %f segundos\n\n", end - start);
    }

    // -------------------------------
    // PROCESO 0: Recolectar y mostrar
    // -------------------------------
    if (rank == 0) {
        int* final_vector = (int*) malloc(n * sizeof(int));
        MPI_Gather(block_part_back, block_size, MPI_INT,
                   final_vector, block_size, MPI_INT,
                   0, MPI_COMM_WORLD);

        printf("Vector final (después de redistribuciones):\n");
        for (int i = 0; i < n; i++) printf("%d ", final_vector[i]);
        printf("\n");

        free(final_vector);
        free(global_vector);
    } else {
        MPI_Gather(block_part_back, block_size, MPI_INT,
                   NULL, block_size, MPI_INT,
                   0, MPI_COMM_WORLD);
    }

    // Liberar memoria
    free(block_part);
    free(cyclic_part);
    free(block_part_back);

    MPI_Finalize();
    return 0;
}

