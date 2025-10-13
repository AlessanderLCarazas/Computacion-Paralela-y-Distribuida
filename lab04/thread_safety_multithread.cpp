#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <chrono>
#include <string>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <vector>

using namespace std;
using namespace std::chrono;

#define MAX_LINE 1000
#define MAX_TOKENS 100

// variables globales
int thread_count;
sem_t* sems;
FILE* input_file;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// estadisticas globales
int total_lines_processed = 0;
int total_tokens_found = 0;
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

// estructura para datos del thread
struct thread_data_s {
    int thread_id;
    int lines_processed;
    int tokens_found;
};

// ==================== implementacion 1: con semaforos (thread-safe) ====================

void* Tokenize_Semaphore(void* rank) {
    long my_rank = (long) rank;
    int count;
    int next = (my_rank + 1) % thread_count;
    char* fg_rv;
    char my_line[MAX_LINE];
    char* my_string;
    int local_lines = 0;
    int local_tokens = 0;
    
    sem_wait(&sems[my_rank]);
    fg_rv = fgets(my_line, MAX_LINE, input_file);
    sem_post(&sems[next]);
    
    while (fg_rv != nullptr) {
        local_lines++;
        
        count = 0;
        my_string = strtok(my_line, " \t\n");
        while (my_string != nullptr) {
            count++;
            local_tokens++;
            my_string = strtok(nullptr, " \t\n");
        }
        
        sem_wait(&sems[my_rank]);
        fg_rv = fgets(my_line, MAX_LINE, input_file);
        sem_post(&sems[next]);
    }
    
    // actualizar estadisticas globales
    pthread_mutex_lock(&stats_mutex);
    total_lines_processed += local_lines;
    total_tokens_found += local_tokens;
    pthread_mutex_unlock(&stats_mutex);
    
    // almacenar datos del thread
    struct thread_data_s* data = new thread_data_s;
    data->thread_id = my_rank;
    data->lines_processed = local_lines;
    data->tokens_found = local_tokens;
    
    return (void*)data;
}

// ==================== implementacion 2: con mutex (thread-safe alternativo) ====================

void* Tokenize_Mutex(void* rank) {
    long my_rank = (long) rank;
    int count;
    char* fg_rv;
    char my_line[MAX_LINE];
    char* my_string;
    int local_lines = 0;
    int local_tokens = 0;
    
    while (true) {
        // leer linea con proteccion mutex
        pthread_mutex_lock(&file_mutex);
        fg_rv = fgets(my_line, MAX_LINE, input_file);
        pthread_mutex_unlock(&file_mutex);
        
        if (fg_rv == nullptr) break;
        
        local_lines++;
        
        // tokenizar linea (no necesita sincronizacion)
        count = 0;
        my_string = strtok(my_line, " \t\n");
        while (my_string != nullptr) {
            count++;
            local_tokens++;
            my_string = strtok(nullptr, " \t\n");
        }
    }
    
    // actualizar estadisticas globales
    pthread_mutex_lock(&stats_mutex);
    total_lines_processed += local_lines;
    total_tokens_found += local_tokens;
    pthread_mutex_unlock(&stats_mutex);
    
    // almacenar datos del thread
    struct thread_data_s* data = new thread_data_s;
    data->thread_id = my_rank;
    data->lines_processed = local_lines;
    data->tokens_found = local_tokens;
    
    return (void*)data;
}

// ==================== implementacion 3: sin sincronizacion (race conditions) ====================

void* Tokenize_Unsafe(void* rank) {
    long my_rank = (long) rank;
    int count;
    char* fg_rv;
    char my_line[MAX_LINE];
    char* my_string;
    int local_lines = 0;
    int local_tokens = 0;
    
    while (true) {
        // leer linea SIN proteccion (race condition)
        fg_rv = fgets(my_line, MAX_LINE, input_file);
        
        if (fg_rv == nullptr) break;
        
        local_lines++;
        
        // tokenizar linea
        count = 0;
        my_string = strtok(my_line, " \t\n");
        while (my_string != nullptr) {
            count++;
            local_tokens++;
            my_string = strtok(nullptr, " \t\n");
        }
    }
    
    // actualizar estadisticas SIN proteccion (race condition)
    total_lines_processed += local_lines;
    total_tokens_found += local_tokens;
    
    // almacenar datos del thread
    struct thread_data_s* data = new thread_data_s;
    data->thread_id = my_rank;
    data->lines_processed = local_lines;
    data->tokens_found = local_tokens;
    
    return (void*)data;
}

// ==================== funciones auxiliares ====================

// crear archivo de prueba
void Create_test_file(const char* filename, int num_lines) {
    ofstream file(filename);
    string words[] = {"hello", "world", "thread", "safety", "parallel", "computing", 
                     "synchronization", "mutex", "semaphore", "race", "condition"};
    int num_words = 11;
    
    srand(time(nullptr));
    
    for (int i = 0; i < num_lines; i++) {
        int tokens_per_line = 3 + rand() % 5; // 3-7 tokens per line
        for (int j = 0; j < tokens_per_line; j++) {
            file << words[rand() % num_words];
            if (j < tokens_per_line - 1) file << " ";
        }
        file << endl;
    }
    
    file.close();
}

// inicializar semaforos
void Initialize_semaphores() {
    sems = new sem_t[thread_count];
    for (int i = 0; i < thread_count; i++) {
        sem_init(&sems[i], 0, 0);
    }
    sem_post(&sems[0]); // thread 0 comienza
}

// limpiar semaforos
void Cleanup_semaphores() {
    for (int i = 0; i < thread_count; i++) {
        sem_destroy(&sems[i]);
    }
    delete[] sems;
}

// ejecutar prueba
double Run_test(void* (*thread_func)(void*), const char* test_name) {
    pthread_t* thread_handles = new pthread_t[thread_count];
    struct thread_data_s* thread_results[thread_count];
    
    // resetear estadisticas
    total_lines_processed = 0;
    total_tokens_found = 0;
    
    // reabrir archivo
    if (input_file) fclose(input_file);
    input_file = fopen("test_input.txt", "r");
    
    auto start = high_resolution_clock::now();
    
    // crear threads
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], nullptr, thread_func, (void*)thread);
    }
    
    // esperar threads
    for (int thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], (void**)&thread_results[thread]);
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    
    cout << "| " << setw(20) << test_name << " |";
    cout << setw(8) << total_lines_processed << " |";
    cout << setw(9) << total_tokens_found << " |";
    cout << fixed << setprecision(3) << setw(8) << (duration.count() / 1000.0) << " |" << endl;
    
    // limpiar memoria
    for (int i = 0; i < thread_count; i++) {
        delete thread_results[i];
    }
    delete[] thread_handles;
    
    return duration.count() / 1000000.0;
}

// ==================== funcion principal ====================

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "uso: " << argv[0] << " <num_threads> <num_lineas>" << endl;
        cout << "ejemplo: " << argv[0] << " 4 1000" << endl;
        return 1;
    }
    
    thread_count = strtol(argv[1], nullptr, 10);
    int num_lines = strtol(argv[2], nullptr, 10);
    
    cout << "\n=== analisis de thread safety - tokenizacion de strings ===" << endl;
    cout << "threads: " << thread_count << ", lineas de entrada: " << num_lines << endl;
    cout << "comparacion de implementaciones thread-safe vs unsafe" << endl;
    cout << "\n";
    
    // crear archivo de prueba
    Create_test_file("test_input.txt", num_lines);
    
    // inicializar semaforos
    Initialize_semaphores();
    
    // crear tabla de resultados
    cout << "=============================================================================" << endl;
    cout << "|     Implementacion       | Lineas  | Tokens  | Tiempo  |" << endl;
    cout << "|                          |Proces.  |Encontr. |  (ms)   |" << endl;
    cout << "|--------------------------|---------|---------|---------|" << endl;
    
    // ejecutar pruebas
    Run_test(Tokenize_Semaphore, "Con Semaforos");
    Run_test(Tokenize_Mutex, "Con Mutex");
    Run_test(Tokenize_Unsafe, "Sin Sincronizacion");
    
    cout << "=============================================================================" << endl;
    cout << "\nanalisis de resultados:" << endl;
    cout << "- implementaciones thread-safe garantizan resultados correctos" << endl;
    cout << "- implementacion unsafe puede mostrar race conditions" << endl;
    cout << "- semaforos permiten acceso secuencial ordenado" << endl;
    cout << "- mutex permite acceso exclusivo pero sin orden garantizado" << endl;
    cout << "\nnotas sobre thread safety:" << endl;
    cout << "1. race conditions ocurren cuando threads acceden simultaneamente" << endl;
    cout << "2. sincronizacion agrega overhead pero garantiza correctness" << endl;
    cout << "3. semaforos vs mutex: diferentes estrategias de coordinacion" << endl;
    
    // limpiar recursos
    Cleanup_semaphores();
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&stats_mutex);
    if (input_file) fclose(input_file);
    
    return 0;
}