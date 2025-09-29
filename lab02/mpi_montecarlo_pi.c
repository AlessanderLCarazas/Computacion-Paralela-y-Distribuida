#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Genera un número aleatorio en [-1, 1]
double rand_double() {
    return (2.0 * rand() / (double)RAND_MAX) - 1.0;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    long long int num_tosses = 0;
    if (rank == 0) {
        // Lee el número total de lanzamientos desde argumento o fijo
        if (argc >= 2) {
            num_tosses = atoll(argv[1]);
        } else {
            num_tosses = 1000000; // valor por defecto
        }
    }

    // Difundir número total de lanzamientos
    MPI_Bcast(&num_tosses, 1, MPI_LONG_LONG_INT, 0, MPI_COMM_WORLD);

    // Calcular lanzamientos locales (división equilibrada)
    long long int local_tosses = num_tosses / size;
    if (rank < num_tosses % size) {
        local_tosses++;
    }

    // Inicializar generador de números aleatorios con semilla distinta en cada proceso
    srand(time(NULL) + rank * 1337);

    // Contador local de dardos dentro del círculo
    long long int local_in_circle = 0;
    for (long long int toss = 0; toss < local_tosses; toss++) {
        double x = rand_double();
        double y = rand_double();
        double dist_sq = x * x + y * y;
        if (dist_sq <= 1.0) {
            local_in_circle++;
        }
    }

    // Reducir resultados al proceso 0
    long long int total_in_circle = 0;
    MPI_Reduce(&local_in_circle, &total_in_circle, 1, MPI_LONG_LONG_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Proceso 0 calcula e imprime la estimación de PI
    if (rank == 0) {
        double pi_estimate = 4.0 * (double)total_in_circle / (double)num_tosses;
        printf("\n=== Estimación de π con Monte Carlo ===\n");
        printf("Número total de lanzamientos = %lld\n", num_tosses);
        printf("Número dentro del círculo    = %lld\n", total_in_circle);
        printf("Estimación de π              = %.10f\n", pi_estimate);
    }

    MPI_Finalize();
    return 0;
}

