#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Función para determinar en qué bin cae un valor
int find_bin(float value, float min_meas, float max_meas, int bin_count) {
    float bin_width = (max_meas - min_meas) / bin_count;
    int bin = (int)((value - min_meas) / bin_width);
    if (bin == bin_count) bin--; // caso borde
    return bin;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int data_count, bin_count;
    float min_meas, max_meas;
    float *data = NULL;

    if (rank == 0) {
        // ==== Datos de ejemplo del libro (20 valores) ====
        data_count = 20;
        float sample_data[20] = {
            1.3, 2.9, 0.4, 0.3, 1.3,
            4.4, 1.7, 0.4, 3.2, 0.3,
            4.9, 2.4, 3.1, 4.4, 3.9,
            0.4, 4.2, 4.5, 4.9, 0.9
        };
        min_meas = 0.0;
        max_meas = 5.0;
        bin_count = 5;

        // Copiamos datos en arreglo dinámico
        data = (float*)malloc(data_count * sizeof(float));
        for (int i = 0; i < data_count; i++) {
            data[i] = sample_data[i];
        }
    }

    // Broadcast de parámetros a todos los procesos
    MPI_Bcast(&data_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&bin_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&min_meas, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&max_meas, 1, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Distribuir datos entre procesos
    int local_count = data_count / size;
    int remainder = data_count % size;

    if (rank < remainder) {
        local_count++;
    }

    float* local_data = (float*)malloc(local_count * sizeof(float));

    // Enviar trozos de datos
    int* sendcounts = NULL;
    int* displs = NULL;

    if (rank == 0) {
        sendcounts = (int*)malloc(size * sizeof(int));
        displs = (int*)malloc(size * sizeof(int));

        int offset = 0;
        for (int i = 0; i < size; i++) {
            sendcounts[i] = data_count / size;
            if (i < remainder) sendcounts[i]++;
            displs[i] = offset;
            offset += sendcounts[i];
        }
    }

    MPI_Scatterv(data, sendcounts, displs, MPI_FLOAT,
                 local_data, local_count, MPI_FLOAT,
                 0, MPI_COMM_WORLD);

    // Cada proceso calcula su histograma local
    int* local_bins = (int*)calloc(bin_count, sizeof(int));
    for (int i = 0; i < local_count; i++) {
        int bin = find_bin(local_data[i], min_meas, max_meas, bin_count);
        local_bins[bin]++;
    }

    // Reducimos al histograma global en el proceso 0
    int* global_bins = NULL;
    if (rank == 0) {
        global_bins = (int*)calloc(bin_count, sizeof(int));
    }

    MPI_Reduce(local_bins, global_bins, bin_count, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Proceso 0 imprime el histograma
    if (rank == 0) {
        float bin_width = (max_meas - min_meas) / bin_count;
        printf("\n=== HISTOGRAMA GLOBAL ===\n");
        for (int i = 0; i < bin_count; i++) {
            float bin_start = min_meas + i * bin_width;
            float bin_end   = bin_start + bin_width;
            printf("Bin %d [%.2f - %.2f): %d\n", i, bin_start, bin_end, global_bins[i]);
        }
    }

    // Liberar memoria
    free(local_data);
    free(local_bins);
    if (rank == 0) {
        free(data);
        free(sendcounts);
        free(displs);
        free(global_bins);
    }

    MPI_Finalize();
    return 0;
}

