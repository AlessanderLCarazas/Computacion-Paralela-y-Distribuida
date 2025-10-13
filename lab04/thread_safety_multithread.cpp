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

const int BUFFER_SIZE = 1000;
const int TOKEN_LIMIT = 100;

//    Clase para gestionar recursos de archivo   
class FileManager {
private:
    FILE* fileHandle;
    pthread_mutex_t accessLock;
    
public:
    FileManager() : fileHandle(nullptr) {
        pthread_mutex_init(&accessLock, nullptr);
    }
    
    ~FileManager() {
        if (fileHandle) fclose(fileHandle);
        pthread_mutex_destroy(&accessLock);
    }
    
    bool openFile(const char* path) {
        if (fileHandle) fclose(fileHandle);
        fileHandle = fopen(path, "r");
        return fileHandle != nullptr;
    }
    
    char* readLineWithLock(char* buffer, int size) {
        pthread_mutex_lock(&accessLock);
        char* result = fgets(buffer, size, fileHandle);
        pthread_mutex_unlock(&accessLock);
        return result;
    }
    
    char* readLineUnsafe(char* buffer, int size) {
        return fgets(buffer, size, fileHandle);
    }
    
    void closeFile() {
        if (fileHandle) {
            fclose(fileHandle);
            fileHandle = nullptr;
        }
    }
    
    FILE* getHandle() { return fileHandle; }
};

//    Clase para estadísticas   
class Statistics {
private:
    int linesCount;
    int tokensCount;
    pthread_mutex_t statLock;
    
public:
    Statistics() : linesCount(0), tokensCount(0) {
        pthread_mutex_init(&statLock, nullptr);
    }
    
    ~Statistics() {
        pthread_mutex_destroy(&statLock);
    }
    
    void addCountsSafe(int lines, int tokens) {
        pthread_mutex_lock(&statLock);
        linesCount += lines;
        tokensCount += tokens;
        pthread_mutex_unlock(&statLock);
    }
    
    void addCountsUnsafe(int lines, int tokens) {
        linesCount += lines;
        tokensCount += tokens;
    }
    
    void reset() {
        pthread_mutex_lock(&statLock);
        linesCount = 0;
        tokensCount = 0;
        pthread_mutex_unlock(&statLock);
    }
    
    int getLines() { return linesCount; }
    int getTokens() { return tokensCount; }
};

//    Clase para coordinar con semáforos   
class SemaphoreCoordinator {
private:
    sem_t* semaphores;
    int numSemaphores;
    
public:
    SemaphoreCoordinator(int count) : numSemaphores(count) {
        semaphores = new sem_t[numSemaphores];
        for (int i = 0; i < numSemaphores; i++) {
            sem_init(&semaphores[i], 0, 0);
        }
        sem_post(&semaphores[0]);
    }
    
    ~SemaphoreCoordinator() {
        for (int i = 0; i < numSemaphores; i++) {
            sem_destroy(&semaphores[i]);
        }
        delete[] semaphores;
    }
    
    void waitTurn(int index) {
        sem_wait(&semaphores[index]);
    }
    
    void signalNext(int index) {
        int nextIndex = (index + 1) % numSemaphores;
        sem_post(&semaphores[nextIndex]);
    }
};

//    Estructura para resultados de thread   
struct WorkerResult {
    int workerId;
    int processedLines;
    int discoveredTokens;
};

//    Clase base para estrategias de tokenización   
class TokenizationStrategy {
protected:
    FileManager* fileMgr;
    Statistics* stats;
    int workerCount;
    
    int parseTokens(char* line) {
        int count = 0;
        char* token = strtok(line, " \t\n");
        while (token != nullptr) {
            count++;
            token = strtok(nullptr, " \t\n");
        }
        return count;
    }
    
public:
    TokenizationStrategy(FileManager* fm, Statistics* st, int wc) 
        : fileMgr(fm), stats(st), workerCount(wc) {}
    
    virtual void* execute(int workerId) = 0;
    virtual ~TokenizationStrategy() {}
};

//    Estrategia con semáforos   
class SemaphoreStrategy : public TokenizationStrategy {
private:
    SemaphoreCoordinator* coordinator;
    
public:
    SemaphoreStrategy(FileManager* fm, Statistics* st, int wc, SemaphoreCoordinator* coord)
        : TokenizationStrategy(fm, st, wc), coordinator(coord) {}
    
    void* execute(int workerId) override {
        char lineBuffer[BUFFER_SIZE];
        int linesProcessed = 0;
        int tokensFound = 0;
        
        coordinator->waitTurn(workerId);
        char* readResult = fgets(lineBuffer, BUFFER_SIZE, fileMgr->getHandle());
        coordinator->signalNext(workerId);
        
        while (readResult != nullptr) {
            linesProcessed++;
            tokensFound += parseTokens(lineBuffer);
            
            coordinator->waitTurn(workerId);
            readResult = fgets(lineBuffer, BUFFER_SIZE, fileMgr->getHandle());
            coordinator->signalNext(workerId);
        }
        
        stats->addCountsSafe(linesProcessed, tokensFound);
        
        WorkerResult* result = new WorkerResult;
        result->workerId = workerId;
        result->processedLines = linesProcessed;
        result->discoveredTokens = tokensFound;
        
        return (void*)result;
    }
};

//    Estrategia con mutex   
class MutexStrategy : public TokenizationStrategy {
public:
    MutexStrategy(FileManager* fm, Statistics* st, int wc)
        : TokenizationStrategy(fm, st, wc) {}
    
    void* execute(int workerId) override {
        char lineBuffer[BUFFER_SIZE];
        int linesProcessed = 0;
        int tokensFound = 0;
        
        while (true) {
            char* readResult = fileMgr->readLineWithLock(lineBuffer, BUFFER_SIZE);
            
            if (readResult == nullptr) break;
            
            linesProcessed++;
            tokensFound += parseTokens(lineBuffer);
        }
        
        stats->addCountsSafe(linesProcessed, tokensFound);
        
        WorkerResult* result = new WorkerResult;
        result->workerId = workerId;
        result->processedLines = linesProcessed;
        result->discoveredTokens = tokensFound;
        
        return (void*)result;
    }
};

//    Estrategia sin sincronización   
class UnsafeStrategy : public TokenizationStrategy {
public:
    UnsafeStrategy(FileManager* fm, Statistics* st, int wc)
        : TokenizationStrategy(fm, st, wc) {}
    
    void* execute(int workerId) override {
        char lineBuffer[BUFFER_SIZE];
        int linesProcessed = 0;
        int tokensFound = 0;
        
        while (true) {
            char* readResult = fileMgr->readLineUnsafe(lineBuffer, BUFFER_SIZE);
            
            if (readResult == nullptr) break;
            
            linesProcessed++;
            tokensFound += parseTokens(lineBuffer);
        }
        
        stats->addCountsUnsafe(linesProcessed, tokensFound);
        
        WorkerResult* result = new WorkerResult;
        result->workerId = workerId;
        result->processedLines = linesProcessed;
        result->discoveredTokens = tokensFound;
        
        return (void*)result;
    }
};

//    Contexto para threads   
struct WorkerContext {
    int id;
    TokenizationStrategy* strategy;
};

void* workerThreadFunction(void* arg) {
    WorkerContext* ctx = (WorkerContext*)arg;
    return ctx->strategy->execute(ctx->id);
}

//    Generador de datos de prueba   
class TestDataGenerator {
public:
    void generateFile(const char* filename, int numLines) {
        ofstream output(filename);
        string vocabulary[] = {"hello", "world", "thread", "safety", "parallel", 
                              "computing", "synchronization", "mutex", "semaphore", 
                              "race", "condition"};
        int vocabSize = 11;
        
        srand(time(nullptr));
        
        for (int line = 0; line < numLines; line++) {
            int wordsInLine = 3 + rand() % 5;
            for (int word = 0; word < wordsInLine; word++) {
                output << vocabulary[rand() % vocabSize];
                if (word < wordsInLine - 1) output << " ";
            }
            output << endl;
        }
        
        output.close();
    }
};

//    Executor de pruebas   
class BenchmarkExecutor {
private:
    FileManager* fileMgr;
    Statistics* stats;
    int workerCount;
    
public:
    BenchmarkExecutor(FileManager* fm, Statistics* st, int wc)
        : fileMgr(fm), stats(st), workerCount(wc) {}
    
    double runBenchmark(TokenizationStrategy* strategy, const char* strategyName) {
        pthread_t* workers = new pthread_t[workerCount];
        WorkerResult* results[workerCount];
        WorkerContext* contexts = new WorkerContext[workerCount];
        
        stats->reset();
        fileMgr->openFile("test_input.txt");
        
        for (int i = 0; i < workerCount; i++) {
            contexts[i].id = i;
            contexts[i].strategy = strategy;
        }
        
        auto startTime = high_resolution_clock::now();
        
        for (int i = 0; i < workerCount; i++) {
            pthread_create(&workers[i], nullptr, workerThreadFunction, &contexts[i]);
        }
        
        for (int i = 0; i < workerCount; i++) {
            pthread_join(workers[i], (void**)&results[i]);
        }
        
        auto endTime = high_resolution_clock::now();
        auto elapsed = duration_cast<microseconds>(endTime - startTime);
        
        displayResults(strategyName, elapsed.count() / 1000.0);
        
        for (int i = 0; i < workerCount; i++) {
            delete results[i];
        }
        delete[] workers;
        delete[] contexts;
        
        return elapsed.count() / 1000000.0;
    }
    
    void displayResults(const char* name, double timeMs) {
        cout << "| " << setw(20) << name << " |";
        cout << setw(8) << stats->getLines() << " |";
        cout << setw(9) << stats->getTokens() << " |";
        cout << fixed << setprecision(3) << setw(8) << timeMs << " |" << endl;
    }
};

//    Presentador de resultados   
class ResultPresenter {
public:
    void showHeader(int workers, int lines) {
        cout << "\n=== analisis de thread safety - tokenizacion de strings ===" << endl;
        cout << "threads: " << workers << ", lineas de entrada: " << lines << endl;
        cout << "comparacion de implementaciones thread-safe vs unsafe" << endl;
        cout << "\n";
        
        cout << "      =================" << endl;
        cout << "|     Implementacion       | Lineas  | Tokens  | Tiempo  |" << endl;
        cout << "|                          |Proces.  |Encontr. |  (ms)   |" << endl;
        cout << "|--------------------------|---------|---------|---------|" << endl;
    }
    
    void showFooter() {
        cout << "      =================" << endl;
        cout << "\nanalisis de resultados:" << endl;
        cout << "- implementaciones thread-safe garantizan resultados correctos" << endl;
        cout << "- implementacion unsafe puede mostrar race conditions" << endl;
        cout << "- semaforos permiten acceso secuencial ordenado" << endl;
        cout << "- mutex permite acceso exclusivo pero sin orden garantizado" << endl;
        cout << "\nnotas sobre thread safety:" << endl;
        cout << "1. race conditions ocurren cuando threads acceden simultaneamente" << endl;
        cout << "2. sincronizacion agrega overhead pero garantiza correctness" << endl;
        cout << "3. semaforos vs mutex: diferentes estrategias de coordinacion" << endl;
    }
};

//    Función principal   
int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "uso: " << argv[0] << " <num_threads> <num_lineas>" << endl;
        cout << "ejemplo: " << argv[0] << " 4 1000" << endl;
        return 1;
    }
    
    int workerCount = strtol(argv[1], nullptr, 10);
    int lineCount = strtol(argv[2], nullptr, 10);
    
    ResultPresenter presenter;
    presenter.showHeader(workerCount, lineCount);
    
    TestDataGenerator generator;
    generator.generateFile("test_input.txt", lineCount);
    
    FileManager fileMgr;
    Statistics stats;
    SemaphoreCoordinator coordinator(workerCount);
    BenchmarkExecutor executor(&fileMgr, &stats, workerCount);
    
    SemaphoreStrategy semStrategy(&fileMgr, &stats, workerCount, &coordinator);
    MutexStrategy mutexStrategy(&fileMgr, &stats, workerCount);
    UnsafeStrategy unsafeStrategy(&fileMgr, &stats, workerCount);
    
    executor.runBenchmark(&semStrategy, "Con Semaforos");
    executor.runBenchmark(&mutexStrategy, "Con Mutex");
    executor.runBenchmark(&unsafeStrategy, "Sin Sincronizacion");
    
    presenter.showFooter();
    
    return 0;
}
