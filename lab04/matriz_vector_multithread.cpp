#include <iostream>
#include <pthread.h>
#include <chrono>
#include <random>
#include <vector>
#include <cstdlib>
#include <iomanip>
#include <string>

using namespace std;
using namespace std::chrono;

// variables globales para la matriz-vector
double** A;  // matriz
double* x;   // vector de entrada
double* y;   // vector de salida
int m, n;    // dimensiones de la matriz (m x n)
int thread_count;

// estructura para pasar parametros a los threads
struct thread_data {
    int thread_id;
    int local_m;
    int my_first_row;
    int my_last_row;
};

// ==================== implementaciones de matriz-vector ====================

// implementacion serial para comparacion
void Serial_mat_vect() {
    for (int i = 0; i < m; i++) {
        y[i] = 0.0;
        for (int j = 0; j < n; j++) {
            y[i] += A[i][j] * x[j];
        }
    }
}

// implementacion paralela - division por filas
void* Pth_mat_vect_rows(void* rank) {
    long my_rank = (long) rank;
    int local_m = m / thread_count;
    int my_first_row = my_rank * local_m;
    int my_last_row = (my_rank + 1) * local_m - 1;
    
    // ajustar la ultima fila para el ultimo thread
    if (my_rank == thread_count - 1) {
        my_last_row = m - 1;
    }
    
    for (int i = my_first_row; i <= my_last_row; i++) {
        y[i] = 0.0;
        for (int j = 0; j < n; j++) {
            y[i] += A[i][j] * x[j];
        }
    }
    
    return nullptr;
}

// implementacion paralela - division por bloques ciclicos
void* Pth_mat_vect_cyclic(void* rank) {
    long my_rank = (long) rank;
    
    for (int i = my_rank; i < m; i += thread_count) {
        y[i] = 0.0;
        for (int j = 0; j < n; j++) {
            y[i] += A[i][j] * x[j];
        }
    }
    
    return nullptr;
}

// ==================== funciones auxiliares ====================

// inicializar matriz con valores aleatorios
void Initialize_matrix(int rows, int cols) {
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<double> dis(-1.0, 1.0);
    
    // alocar memoria para la matriz
    A = new double*[rows];
    for (int i = 0; i < rows; i++) {
        A[i] = new double[cols];
        for (int j = 0; j < cols; j++) {
            A[i][j] = dis(gen);
        }
    }
    
    // alocar e inicializar vectores
    x = new double[cols];
    y = new double[rows];
    
    for (int i = 0; i < cols; i++) {
        x[i] = dis(gen);
    }
    
    for (int i = 0; i < rows; i++) {
        y[i] = 0.0;
    }
}

// limpiar memoria
void Cleanup_matrix() {
    for (int i = 0; i < m; i++) {
        delete[] A[i];
    }
    delete[] A;
    delete[] x;
    delete[] y;
}

// verificar si dos vectores son iguales (para validacion)
bool Verify_result(double* v1, double* v2, int size) {
    const double EPSILON = 1e-10;
    for (int i = 0; i < size; i++) {
        if (abs(v1[i] - v2[i]) > EPSILON) {
            return false;
        }
    }
    return true;
}

// ==================== funciones de prueba ====================

double Run_serial_test() {
    auto start = high_resolution_clock::now();
    Serial_mat_vect();
    auto end = high_resolution_clock::now();
    
    auto duration = duration_cast<microseconds>(end - start);
    return duration.count() / 1000000.0; // convertir a segundos
}

double Run_parallel_test(void* (*thread_func)(void*), int num_threads) {
    thread_count = num_threads;
    pthread_t* thread_handles = new pthread_t[thread_count];
    
    // reinicializar vector resultado
    for (int i = 0; i < m; i++) {
        y[i] = 0.0;
    }
    
    auto start = high_resolution_clock::now();
    
    // crear threads
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], nullptr, thread_func, (void*) thread);
    }
    
    // esperar que terminen todos los threads
    for (int thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], nullptr);
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    
    delete[] thread_handles;
    return duration.count() / 1000000.0; // convertir a segundos
}

// calcular eficiencia
double Calculate_efficiency(double serial_time, double parallel_time, int num_threads) {
    return serial_time / (parallel_time * num_threads);
}

// ==================== funcion principal ====================

int main(int argc, char* argv[]) {
    if (argc != 1) {
        cout << "uso: " << argv[0] << endl;
        cout << "el programa ejecuta automaticamente con las dimensiones del libro" << endl;
        return 1;
    }
    
    // dimensiones como en el libro
    struct MatrixDim {
        int rows;
        int cols;
        string name;
    };
    
    MatrixDim dimensions[] = {
        {8000000, 8, "8,000,000 x 8"},
        {8000, 8000, "8000 x 8000"},  
        {8, 8000000, "8 x 8,000,000"}
    };
    int num_dimensions = 3;
    
    int thread_counts[] = {1, 2, 4};
    int num_thread_counts = 3;
    
    cout << "\n=== analisis de rendimiento - matriz-vector multiplication ===" << endl;
    cout << "implementaciones: division por filas, division ciclica" << endl;
    cout << "dimensiones como en el libro del capitulo 4" << endl;
    cout << "\n";
    
    // crear tabla de resultados
    cout << "======================================================================================" << endl;
    cout << "|                                  |         Matrix Dimension        |" << endl;
    cout << "|----------------------------------|----------------------------------|" << endl;
    cout << "|             Threads              | 8,000,000 x 8 | 8000 x 8000 | 8 x 8,000,000 |" << endl;
    cout << "|----------------------------------|---------------|--------------|----------------|" << endl;
    cout << "|                                  | Time    Eff.  | Time   Eff.  | Time     Eff.  |" << endl;
    cout << "|----------------------------------|---------------|--------------|----------------|" << endl;
    
    // almacenar resultados para cada dimension
    double results_rows[3][3];    // [dimension][thread]
    double results_cyclic[3][3];  // [dimension][thread]
    double serial_times[3];       // tiempo serial para cada dimension
    
    // ejecutar pruebas para cada dimension
    for (int dim = 0; dim < num_dimensions; dim++) {
        m = dimensions[dim].rows;
        n = dimensions[dim].cols;
        
        cout << "procesando dimension " << dimensions[dim].name << "..." << endl;
        
        // inicializar matriz y vectores
        Initialize_matrix(m, n);
        
        // ejecutar prueba serial
        serial_times[dim] = Run_serial_test();
        
        // guardar resultado serial para verificacion
        double* serial_result = new double[m];
        for (int i = 0; i < m; i++) {
            serial_result[i] = y[i];
        }
        
        // ejecutar pruebas paralelas
        for (int t = 0; t < num_thread_counts; t++) {
            if (thread_counts[t] == 1) {
                results_rows[dim][t] = serial_times[dim];
                results_cyclic[dim][t] = serial_times[dim];
            } else {
                // division por filas
                results_rows[dim][t] = Run_parallel_test(Pth_mat_vect_rows, thread_counts[t]);
                
                // division ciclica
                results_cyclic[dim][t] = Run_parallel_test(Pth_mat_vect_cyclic, thread_counts[t]);
            }
        }
        
        delete[] serial_result;
        Cleanup_matrix();
    }
    
    // mostrar resultados para division por filas
    cout << "| 1 (Division por Filas)           |";
    for (int dim = 0; dim < num_dimensions; dim++) {
        cout << fixed << setprecision(3) << setw(6) << results_rows[dim][0] << " 1.000 |";
    }
    cout << endl;
    
    cout << "| 2                                |";
    for (int dim = 0; dim < num_dimensions; dim++) {
        double eff = Calculate_efficiency(serial_times[dim], results_rows[dim][1], 2);
        cout << fixed << setprecision(3) << setw(6) << results_rows[dim][1] << " " << setw(5) << eff << " |";
    }
    cout << endl;
    
    cout << "| 4                                |";
    for (int dim = 0; dim < num_dimensions; dim++) {
        double eff = Calculate_efficiency(serial_times[dim], results_rows[dim][2], 4);
        cout << fixed << setprecision(3) << setw(6) << results_rows[dim][2] << " " << setw(5) << eff << " |";
    }
    cout << endl;
    
    cout << "|----------------------------------|---------------|--------------|----------------|" << endl;
    
    // mostrar resultados para division ciclica
    cout << "| 1 (Division Ciclica)             |";
    for (int dim = 0; dim < num_dimensions; dim++) {
        cout << fixed << setprecision(3) << setw(6) << results_cyclic[dim][0] << " 1.000 |";
    }
    cout << endl;
    
    cout << "| 2                                |";
    for (int dim = 0; dim < num_dimensions; dim++) {
        double eff = Calculate_efficiency(serial_times[dim], results_cyclic[dim][1], 2);
        cout << fixed << setprecision(3) << setw(6) << results_cyclic[dim][1] << " " << setw(5) << eff << " |";
    }
    cout << endl;
    
    cout << "| 4                                |";
    for (int dim = 0; dim < num_dimensions; dim++) {
        double eff = Calculate_efficiency(serial_times[dim], results_cyclic[dim][2], 4);
        cout << fixed << setprecision(3) << setw(6) << results_cyclic[dim][2] << " " << setw(5) << eff << " |";
    }
    cout << endl;
    
    cout << "======================================================================================" << endl;
    cout << "\ntiempos en segundos" << endl;
    cout << "eficiencia = tiempo_serial / (tiempo_paralelo * num_threads)" << endl;
    cout << "dimensiones probadas: 8,000,000 x 8, 8000 x 8000, 8 x 8,000,000" << endl;
    
    // analisis de resultados
    cout << "\n=== analisis de rendimiento ===" << endl;
    cout << "- division por filas: cada thread procesa un bloque continuo de filas" << endl;
    cout << "- division ciclica: cada thread procesa filas alternadas (mejor balance de carga)" << endl;
    cout << "- eficiencia ideal = 1.0 (speedup lineal)" << endl;
    cout << "- matrices anchas (pocas filas) escalan peor" << endl;
    cout << "- matrices altas (muchas filas) escalan mejor" << endl;
    
    return 0;
}