#include <iostream>
#include <pthread.h>
#include <chrono>
#include <random>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <iomanip>

using namespace std;
using namespace std::chrono;

// estructuras para los diferentes tipos de nodos
struct list_node_s {
    int data;
    struct list_node_s* next;
};

struct list_node_with_mutex_s {
    int data;
    struct list_node_with_mutex_s* next;
    pthread_mutex_t mutex;
    
    list_node_with_mutex_s(int value) : data(value), next(nullptr) {
        pthread_mutex_init(&mutex, nullptr);
    }
    
    ~list_node_with_mutex_s() {
        pthread_mutex_destroy(&mutex);
    }
};

// variables globales
struct list_node_s* head_p = nullptr;
struct list_node_with_mutex_s* head_per_node = nullptr;

pthread_rwlock_t list_rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t head_per_node_mutex = PTHREAD_MUTEX_INITIALIZER;

int thread_count;
int num_ops_per_thread;
int implementation_type; // 1=rwlock, 2=single_mutex, 3=per_node_mutex

//  implementacion 1: read-write locks 

int Member_RWLock(int value) {
    struct list_node_s* temp_p;
    
    pthread_rwlock_rdlock(&list_rwlock);
    temp_p = head_p;
    while (temp_p != nullptr && temp_p->data < value) {
        temp_p = temp_p->next;
    }
    
    int result = 0;
    if (temp_p != nullptr && temp_p->data == value) {
        result = 1;
    }
    
    pthread_rwlock_unlock(&list_rwlock);
    return result;
}

int Insert_RWLock(int value) {
    struct list_node_s* curr_p = head_p;
    struct list_node_s* pred_p = nullptr;
    struct list_node_s* temp_p;
    
    pthread_rwlock_wrlock(&list_rwlock);
    
    while (curr_p != nullptr && curr_p->data < value) {
        pred_p = curr_p;
        curr_p = curr_p->next;
    }
    
    if (curr_p == nullptr || curr_p->data > value) {
        temp_p = new list_node_s;
        temp_p->data = value;
        temp_p->next = curr_p;
        if (pred_p == nullptr) {
            head_p = temp_p;
        } else {
            pred_p->next = temp_p;
        }
        pthread_rwlock_unlock(&list_rwlock);
        return 1;
    } else {
        pthread_rwlock_unlock(&list_rwlock);
        return 0;
    }
}

int Delete_RWLock(int value) {
    struct list_node_s* curr_p = head_p;
    struct list_node_s* pred_p = nullptr;
    
    pthread_rwlock_wrlock(&list_rwlock);
    
    while (curr_p != nullptr && curr_p->data < value) {
        pred_p = curr_p;
        curr_p = curr_p->next;
    }
    
    if (curr_p != nullptr && curr_p->data == value) {
        if (pred_p == nullptr) {
            head_p = curr_p->next;
        } else {
            pred_p->next = curr_p->next;
        }
        delete curr_p;
        pthread_rwlock_unlock(&list_rwlock);
        return 1;
    } else {
        pthread_rwlock_unlock(&list_rwlock);
        return 0;
    }
}

//  implementacion 2: un mutex para toda la lista 

int Member_SingleMutex(int value) {
    struct list_node_s* temp_p;
    
    pthread_mutex_lock(&list_mutex);
    temp_p = head_p;
    while (temp_p != nullptr && temp_p->data < value) {
        temp_p = temp_p->next;
    }
    
    int result = 0;
    if (temp_p != nullptr && temp_p->data == value) {
        result = 1;
    }
    
    pthread_mutex_unlock(&list_mutex);
    return result;
}

int Insert_SingleMutex(int value) {
    struct list_node_s* curr_p = head_p;
    struct list_node_s* pred_p = nullptr;
    struct list_node_s* temp_p;
    
    pthread_mutex_lock(&list_mutex);
    
    while (curr_p != nullptr && curr_p->data < value) {
        pred_p = curr_p;
        curr_p = curr_p->next;
    }
    
    if (curr_p == nullptr || curr_p->data > value) {
        temp_p = new list_node_s;
        temp_p->data = value;
        temp_p->next = curr_p;
        if (pred_p == nullptr) {
            head_p = temp_p;
        } else {
            pred_p->next = temp_p;
        }
        pthread_mutex_unlock(&list_mutex);
        return 1;
    } else {
        pthread_mutex_unlock(&list_mutex);
        return 0;
    }
}

int Delete_SingleMutex(int value) {
    struct list_node_s* curr_p = head_p;
    struct list_node_s* pred_p = nullptr;
    
    pthread_mutex_lock(&list_mutex);
    
    while (curr_p != nullptr && curr_p->data < value) {
        pred_p = curr_p;
        curr_p = curr_p->next;
    }
    
    if (curr_p != nullptr && curr_p->data == value) {
        if (pred_p == nullptr) {
            head_p = curr_p->next;
        } else {
            pred_p->next = curr_p->next;
        }
        delete curr_p;
        pthread_mutex_unlock(&list_mutex);
        return 1;
    } else {
        pthread_mutex_unlock(&list_mutex);
        return 0;
    }
}

// ==================== implementacion 3: un mutex por nodo ====================

int Member_PerNodeMutex(int value) {
    struct list_node_with_mutex_s* temp_p;
    
    pthread_mutex_lock(&head_per_node_mutex);
    temp_p = head_per_node;
    if (temp_p != nullptr) {
        pthread_mutex_lock(&(temp_p->mutex));
    }
    pthread_mutex_unlock(&head_per_node_mutex);
    
    while (temp_p != nullptr && temp_p->data < value) {
        if (temp_p->next != nullptr) {
            pthread_mutex_lock(&(temp_p->next->mutex));
        }
        struct list_node_with_mutex_s* old_temp = temp_p;
        temp_p = temp_p->next;
        pthread_mutex_unlock(&(old_temp->mutex));
    }
    
    if (temp_p == nullptr || temp_p->data > value) {
        if (temp_p != nullptr) {
            pthread_mutex_unlock(&(temp_p->mutex));
        }
        return 0;
    } else {
        pthread_mutex_unlock(&(temp_p->mutex));
        return 1;
    }
}

int Insert_PerNodeMutex(int value) {
    struct list_node_with_mutex_s* curr_p;
    struct list_node_with_mutex_s* pred_p = nullptr;
    struct list_node_with_mutex_s* temp_p;
    
    pthread_mutex_lock(&head_per_node_mutex);
    curr_p = head_per_node;
    if (curr_p != nullptr) {
        pthread_mutex_lock(&(curr_p->mutex));
    }
    
    while (curr_p != nullptr && curr_p->data < value) {
        if (curr_p->next != nullptr) {
            pthread_mutex_lock(&(curr_p->next->mutex));
        }
        if (pred_p != nullptr) {
            pthread_mutex_unlock(&(pred_p->mutex));
        } else {
            pthread_mutex_unlock(&head_per_node_mutex);
        }
        pred_p = curr_p;
        curr_p = curr_p->next;
    }
    
    if (curr_p == nullptr || curr_p->data > value) {
        temp_p = new list_node_with_mutex_s(value);
        temp_p->next = curr_p;
        
        if (pred_p == nullptr) {
            head_per_node = temp_p;
            pthread_mutex_unlock(&head_per_node_mutex);
        } else {
            pred_p->next = temp_p;
            pthread_mutex_unlock(&(pred_p->mutex));
        }
        
        if (curr_p != nullptr) {
            pthread_mutex_unlock(&(curr_p->mutex));
        }
        return 1;
    } else {
        if (pred_p != nullptr) {
            pthread_mutex_unlock(&(pred_p->mutex));
        } else {
            pthread_mutex_unlock(&head_per_node_mutex);
        }
        pthread_mutex_unlock(&(curr_p->mutex));
        return 0;
    }
}

int Delete_PerNodeMutex(int value) {
    struct list_node_with_mutex_s* curr_p;
    struct list_node_with_mutex_s* pred_p = nullptr;
    
    pthread_mutex_lock(&head_per_node_mutex);
    curr_p = head_per_node;
    if (curr_p != nullptr) {
        pthread_mutex_lock(&(curr_p->mutex));
    }
    
    while (curr_p != nullptr && curr_p->data < value) {
        if (curr_p->next != nullptr) {
            pthread_mutex_lock(&(curr_p->next->mutex));
        }
        if (pred_p != nullptr) {
            pthread_mutex_unlock(&(pred_p->mutex));
        } else {
            pthread_mutex_unlock(&head_per_node_mutex);
        }
        pred_p = curr_p;
        curr_p = curr_p->next;
    }
    
    if (curr_p != nullptr && curr_p->data == value) {
        if (pred_p == nullptr) {
            head_per_node = curr_p->next;
            pthread_mutex_unlock(&head_per_node_mutex);
        } else {
            pred_p->next = curr_p->next;
            pthread_mutex_unlock(&(pred_p->mutex));
        }
        
        delete curr_p;
        return 1;
    } else {
        if (pred_p != nullptr) {
            pthread_mutex_unlock(&(pred_p->mutex));
        } else {
            pthread_mutex_unlock(&head_per_node_mutex);
        }
        if (curr_p != nullptr) {
            pthread_mutex_unlock(&(curr_p->mutex));
        }
        return 0;
    }
}

//  funciones de inicializacion 

void Initialize_RWLock_List() {
    // limpiar lista existente
    struct list_node_s* temp;
    while (head_p != nullptr) {
        temp = head_p;
        head_p = head_p->next;
        delete temp;
    }
    
    // Insertar algunos valores iniciales
    for (int i = 0; i < 1000; i++) {
        Insert_RWLock(i * 2);
    }
}

void Initialize_SingleMutex_List() {
    // limpiar lista existente
    struct list_node_s* temp;
    while (head_p != nullptr) {
        temp = head_p;
        head_p = head_p->next;
        delete temp;
    }
    
    // insertar algunos valores iniciales
    for (int i = 0; i < 1000; i++) {
        Insert_SingleMutex(i * 2);
    }
}

void Initialize_PerNodeMutex_List() {
    // limpiar lista existente
    struct list_node_with_mutex_s* temp;
    while (head_per_node != nullptr) {
        temp = head_per_node;
        head_per_node = head_per_node->next;
        delete temp;
    }
    
    // insertar algunos valores iniciales
    for (int i = 0; i < 1000; i++) {
        Insert_PerNodeMutex(i * 2);
    }
}

//  funcion de trabajo de los threads 

void* Thread_work(void* rank) {
    long my_rank = (long) rank;
    int i, val;
    double which_op;
    
    // generador de numeros aleatorios por thread
    random_device rd;
    mt19937 gen(rd() + my_rank);
    uniform_real_distribution<double> op_dist(0.0, 1.0);
    uniform_int_distribution<int> val_dist(0, 99999);
    
    for (i = 0; i < num_ops_per_thread; i++) {
        which_op = op_dist(gen);
        val = val_dist(gen);
        
        if (which_op < 0.999) { // 99.9% member operations
            if (implementation_type == 1) {
                Member_RWLock(val);
            } else if (implementation_type == 2) {
                Member_SingleMutex(val);
            } else {
                Member_PerNodeMutex(val);
            }
        } else if (which_op < 0.9995) { // 0.05% insert operations
            if (implementation_type == 1) {
                Insert_RWLock(val);
            } else if (implementation_type == 2) {
                Insert_SingleMutex(val);
            } else {
                Insert_PerNodeMutex(val);
            }
        } else { // 0.05% delete operations
            if (implementation_type == 1) {
                Delete_RWLock(val);
            } else if (implementation_type == 2) {
                Delete_SingleMutex(val);
            } else {
                Delete_PerNodeMutex(val);
            }
        }
    }
    
    return nullptr;
}

//  funcion principal 

double RunTest(int impl_type, int threads, int ops) {
    implementation_type = impl_type;
    thread_count = threads;
    num_ops_per_thread = ops;
    
    pthread_t* thread_handles = new pthread_t[thread_count];
    
    // inicializar la lista apropiada
    if (impl_type == 1) {
        Initialize_RWLock_List();
    } else if (impl_type == 2) {
        Initialize_SingleMutex_List();
    } else {
        Initialize_PerNodeMutex_List();
    }
    
    auto start_time = high_resolution_clock::now();
    
    // crear threads
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_create(&thread_handles[thread], nullptr, Thread_work, (void*) thread);
    }
    
    // esperar que terminen todos los threads
    for (long thread = 0; thread < thread_count; thread++) {
        pthread_join(thread_handles[thread], nullptr);
    }
    
    auto end_time = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end_time - start_time);
    
    delete[] thread_handles;
    
    return duration.count() / 1000.0; // retornar en segundos
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "uso: " << argv[0] << " <operaciones_por_thread>" << endl;
        cout << "ejemplo: " << argv[0] << " 100000" << endl;
        return 1;
    }
    
    int ops_per_thread = strtol(argv[1], nullptr, 10);
    int thread_counts[] = {1, 2, 4, 8};
    int num_thread_counts = 4;
    
    cout << "\n=== analisis de rendimiento - lista enlazada multi-thread ===" << endl;
    cout << "operaciones por thread: " << ops_per_thread << endl;
    cout << "distribucion: 99.9% member, 0.05% insert, 0.05% delete" << endl;
    cout << "\n";
    
    // crear tabla de resultados
    cout << "=====================================================================" << endl;
    cout << "|                             |      Number of Threads       |" << endl;
    cout << "|-----------------------------|-------------------------------|" << endl;
    cout << "|        Implementation       |    1    |    2    |    4    |    8    |" << endl;
    cout << "|-----------------------------|---------|---------|---------|---------|" << endl;
    
    // ejecutar pruebas para read-write locks
    cout << "| Read-Write Locks            |";
    for (int i = 0; i < num_thread_counts; i++) {
        double time = RunTest(1, thread_counts[i], ops_per_thread);
        cout << fixed << setprecision(3) << setw(7) << time << " |";
    }
    cout << endl;
    
    // ejecutar pruebas para un mutex para toda la lista
    cout << "| One Mutex for Entire List   |";
    for (int i = 0; i < num_thread_counts; i++) {
        double time = RunTest(2, thread_counts[i], ops_per_thread);
        cout << fixed << setprecision(3) << setw(7) << time << " |";
    }
    cout << endl;
    
    // ejecutar pruebas para un mutex por nodo
    cout << "| One Mutex per Node          |";
    for (int i = 0; i < num_thread_counts; i++) {
        double time = RunTest(3, thread_counts[i], ops_per_thread);
        cout << fixed << setprecision(3) << setw(7) << time << " |";
    }
    cout << endl;
    
    cout << "=====================================================================" << endl;
    cout << "\ntiempos en segundos" << endl;
    cout << ops_per_thread << " ops/thread" << endl;
    cout << "99.9% member" << endl;
    cout << "0.05% insert" << endl;
    cout << "0.05% delete" << endl;
    
    // limpiar recursos
    pthread_rwlock_destroy(&list_rwlock);
    pthread_mutex_destroy(&list_mutex);
    pthread_mutex_destroy(&head_per_node_mutex);
    
    // limpiar listas
    struct list_node_s* temp1;
    while (head_p != nullptr) {
        temp1 = head_p;
        head_p = head_p->next;
        delete temp1;
    }
    
    struct list_node_with_mutex_s* temp2;
    while (head_per_node != nullptr) {
        temp2 = head_per_node;
        head_per_node = head_per_node->next;
        delete temp2;
    }
    
    return 0;
}