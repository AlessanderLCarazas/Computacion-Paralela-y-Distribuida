#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Función de comparación para qsort
int cmpfunc(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

// Función merge
int* merge_arrays(int *arr1, int n1, int *arr2, int n2) {
    int *result = malloc((n1+n2) * sizeof(int));
    int i=0, j=0, k=0;

    while (i < n1 && j < n2) {
        if (arr1[i] <= arr2[j]) result[k++] = arr1[i++];
        else result[k++] = arr2[j++];
    }
    while (i < n1) result[k++] = arr1[i++];
    while (j < n2) result[k++] = arr2[j++];
    return result;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, comm_sz;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);

    int n;
    if (rank == 0) {
        if (argc < 2) {
            printf("Uso: %s <número total de elementos>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        n = atoi(argv[1]);
    }

    // Broadcast de n a todos
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (n % comm_sz != 0) {
        if (rank == 0) printf("n debe ser divisible entre comm_sz.\n");
        MPI_Finalize();
        return 0;
    }

    int local_n = n / comm_sz;
    int *local_arr = malloc(local_n * sizeof(int));

    // Semilla distinta en cada proceso
    srand(time(NULL) + rank*100);

    for (int i = 0; i < local_n; i++) {
        local_arr[i] = rand() % 100; // valores entre 0-99
    }

    // Ordenar el arreglo local
    qsort(local_arr, local_n, sizeof(int), cmpfunc);

    // Mostrar los arreglos locales antes del merge (solo debug)
    int *gathered = NULL;
    if (rank == 0) gathered = malloc(n * sizeof(int));

    MPI_Gather(local_arr, local_n, MPI_INT,
               gathered, local_n, MPI_INT,
               0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Listas locales ordenadas (antes del merge tree):\n");
        for (int p = 0; p < comm_sz; p++) {
            printf("Proceso %d: ", p);
            for (int j = 0; j < local_n; j++) {
                printf("%d ", gathered[p*local_n + j]);
            }
            printf("\n");
        }
    }

    if (gathered) free(gathered);

    // === Etapa de tree-structured merge ===
    int step = 1;
    int *current_arr = local_arr;
    int current_size = local_n;

    while (step < comm_sz) {
        if (rank % (2*step) == 0) {
            // Recibe de rank + step si existe
            int src = rank + step;
            if (src < comm_sz) {
                int recv_size;
                MPI_Recv(&recv_size, 1, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                int *recv_arr = malloc(recv_size * sizeof(int));
                MPI_Recv(recv_arr, recv_size, MPI_INT, src, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                int *merged = merge_arrays(current_arr, current_size, recv_arr, recv_size);
                free(current_arr);
                free(recv_arr);
                current_arr = merged;
                current_size += recv_size;
            }
        } else {
            // Envía al rank - step
            int dest = rank - step;
            MPI_Send(&current_size, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            MPI_Send(current_arr, current_size, MPI_INT, dest, 0, MPI_COMM_WORLD);
            free(current_arr);
            break; // Sale del bucle, ya no participa
        }
        step *= 2;
    }

    // Proceso 0 imprime el arreglo global ordenado
    if (rank == 0) {
        printf("\nArreglo global ordenado:\n");
        for (int i = 0; i < n; i++) {
            printf("%d ", current_arr[i]);
        }
        printf("\n");
        free(current_arr);
    }

    MPI_Finalize();
    return 0;
}

