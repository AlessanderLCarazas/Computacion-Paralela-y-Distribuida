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

//   Clase para gestionar datos de la matriz  
class MatrixData {
private:
    double** matrix;
    double* inputVec;
    double* outputVec;
    int numRows;
    int numCols;
    
public:
    MatrixData(int r, int c) : numRows(r), numCols(c) {
        allocateMemory();
        fillWithRandomData();
    }
    
    ~MatrixData() {
        releaseMemory();
    }
    
    void allocateMemory() {
        random_device rd;
        mt19937 generator(rd());
        uniform_real_distribution<double> distribution(-1.0, 1.0);
        
        matrix = new double*[numRows];
        for (int i = 0; i < numRows; i++) {
            matrix[i] = new double[numCols];
        }
        
        inputVec = new double[numCols];
        outputVec = new double[numRows];
    }
    
    void fillWithRandomData() {
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<double> dist(-1.0, 1.0);
        
        for (int i = 0; i < numRows; i++) {
            for (int j = 0; j < numCols; j++) {
                matrix[i][j] = dist(gen);
            }
        }
        
        for (int j = 0; j < numCols; j++) {
            inputVec[j] = dist(gen);
        }
        
        resetOutput();
    }
    
    void releaseMemory() {
        for (int i = 0; i < numRows; i++) {
            delete[] matrix[i];
        }
        delete[] matrix;
        delete[] inputVec;
        delete[] outputVec;
    }
    
    void resetOutput() {
        for (int i = 0; i < numRows; i++) {
            outputVec[i] = 0.0;
        }
    }
    
    double** getMatrix() { return matrix; }
    double* getInput() { return inputVec; }
    double* getOutput() { return outputVec; }
    int getRows() { return numRows; }
    int getCols() { return numCols; }
};

//   Estructuras para configuración de threads  
struct ThreadConfig {
    int id;
    MatrixData* data;
    int totalThreads;
};

//   Estrategias de multiplicación  
class MultiplicationStrategy {
public:
    virtual void executeSerial(MatrixData* data) = 0;
    virtual void* executeParallel(void* config) = 0;
    virtual ~MultiplicationStrategy() {}
};

class BlockStrategy : public MultiplicationStrategy {
public:
    void executeSerial(MatrixData* data) override {
        double** mat = data->getMatrix();
        double* in = data->getInput();
        double* out = data->getOutput();
        int rows = data->getRows();
        int cols = data->getCols();
        
        for (int r = 0; r < rows; r++) {
            out[r] = 0.0;
            for (int c = 0; c < cols; c++) {
                out[r] += mat[r][c] * in[c];
            }
        }
    }
    
    void* executeParallel(void* config) override {
        ThreadConfig* cfg = (ThreadConfig*)config;
        MatrixData* data = cfg->data;
        
        double** mat = data->getMatrix();
        double* in = data->getInput();
        double* out = data->getOutput();
        int rows = data->getRows();
        int cols = data->getCols();
        
        int chunkSize = rows / cfg->totalThreads;
        int startRow = cfg->id * chunkSize;
        int endRow = (cfg->id == cfg->totalThreads - 1) ? rows : (cfg->id + 1) * chunkSize;
        
        for (int r = startRow; r < endRow; r++) {
            out[r] = 0.0;
            for (int c = 0; c < cols; c++) {
                out[r] += mat[r][c] * in[c];
            }
        }
        
        return nullptr;
    }
};

class InterleavedStrategy : public MultiplicationStrategy {
public:
    void executeSerial(MatrixData* data) override {
        double** mat = data->getMatrix();
        double* in = data->getInput();
        double* out = data->getOutput();
        int rows = data->getRows();
        int cols = data->getCols();
        
        for (int r = 0; r < rows; r++) {
            out[r] = 0.0;
            for (int c = 0; c < cols; c++) {
                out[r] += mat[r][c] * in[c];
            }
        }
    }
    
    void* executeParallel(void* config) override {
        ThreadConfig* cfg = (ThreadConfig*)config;
        MatrixData* data = cfg->data;
        
        double** mat = data->getMatrix();
        double* in = data->getInput();
        double* out = data->getOutput();
        int rows = data->getRows();
        int cols = data->getCols();
        
        for (int r = cfg->id; r < rows; r += cfg->totalThreads) {
            out[r] = 0.0;
            for (int c = 0; c < cols; c++) {
                out[r] += mat[r][c] * in[c];
            }
        }
        
        return nullptr;
    }
};

//   Gestor de experimentos  
class BenchmarkManager {
private:
    MultiplicationStrategy* strategy;
    
    static void* threadWrapper(void* arg) {
        ThreadConfig* cfg = (ThreadConfig*)arg;
        return nullptr;
    }
    
public:
    BenchmarkManager(MultiplicationStrategy* s) : strategy(s) {}
    
    double measureSerialTime(MatrixData* data) {
        auto t1 = high_resolution_clock::now();
        strategy->executeSerial(data);
        auto t2 = high_resolution_clock::now();
        
        auto elapsed = duration_cast<microseconds>(t2 - t1);
        return elapsed.count() / 1000000.0;
    }
    
    double measureParallelTime(MatrixData* data, int numThreads, 
                               void* (*threadFunc)(void*)) {
        data->resetOutput();
        
        pthread_t* threads = new pthread_t[numThreads];
        ThreadConfig* configs = new ThreadConfig[numThreads];
        
        auto t1 = high_resolution_clock::now();
        
        for (int i = 0; i < numThreads; i++) {
            configs[i].id = i;
            configs[i].data = data;
            configs[i].totalThreads = numThreads;
            pthread_create(&threads[i], nullptr, threadFunc, &configs[i]);
        }
        
        for (int i = 0; i < numThreads; i++) {
            pthread_join(threads[i], nullptr);
        }
        
        auto t2 = high_resolution_clock::now();
        auto elapsed = duration_cast<microseconds>(t2 - t1);
        
        delete[] threads;
        delete[] configs;
        
        return elapsed.count() / 1000000.0;
    }
    
    double computeEfficiency(double serialT, double parallelT, int threads) {
        return serialT / (parallelT * threads);
    }
};

//   Funciones auxiliares para threads  
static BlockStrategy blockStrat;
static InterleavedStrategy interleavedStrat;

void* blockThreadFunc(void* arg) {
    return blockStrat.executeParallel(arg);
}

void* interleavedThreadFunc(void* arg) {
    return interleavedStrat.executeParallel(arg);
}

//   Clase para presentar resultados  
class ResultPresenter {
private:
    struct TestCase {
        int rows, cols;
        string label;
    };
    
    TestCase testCases[3] = {
        {8000000, 8, "8,000,000 x 8"},
        {8000, 8000, "8000 x 8000"},
        {8, 8000000, "8 x 8,000,000"}
    };
    
    int threadOptions[3] = {1, 2, 4};
    
public:
    void displayHeader() {
        cout << "\n=== analisis de rendimiento - matriz-vector multiplication ===" << endl;
        cout << "implementaciones: division por filas, division ciclica" << endl;
        cout << "dimensiones como en el libro del capitulo 4" << endl;
        cout << "\n";
        
        cout << "    ======" << endl;
        cout << "|                                  |         Matrix Dimension        |" << endl;
        cout << "|----------------------------------|----------------------------------|" << endl;
        cout << "|             Threads              | 8,000,000 x 8 | 8000 x 8000 | 8 x 8,000,000 |" << endl;
        cout << "|----------------------------------|---------------|--------------|----------------|" << endl;
        cout << "|                                  | Time    Eff.  | Time   Eff.  | Time     Eff.  |" << endl;
        cout << "|----------------------------------|---------------|--------------|----------------|" << endl;
    }
    
    void runExperiments() {
        double blockResults[3][3];
        double interleavedResults[3][3];
        double baselineTimes[3];
        
        BlockStrategy blockStrat;
        InterleavedStrategy interleavedStrat;
        
        for (int tc = 0; tc < 3; tc++) {
            cout << "procesando dimension " << testCases[tc].label << "..." << endl;
            
            MatrixData* data = new MatrixData(testCases[tc].rows, testCases[tc].cols);
            BenchmarkManager blockBench(&blockStrat);
            BenchmarkManager interleavedBench(&interleavedStrat);
            
            baselineTimes[tc] = blockBench.measureSerialTime(data);
            
            for (int t = 0; t < 3; t++) {
                if (threadOptions[t] == 1) {
                    blockResults[tc][t] = baselineTimes[tc];
                    interleavedResults[tc][t] = baselineTimes[tc];
                } else {
                    blockResults[tc][t] = blockBench.measureParallelTime(
                        data, threadOptions[t], blockThreadFunc);
                    interleavedResults[tc][t] = interleavedBench.measureParallelTime(
                        data, threadOptions[t], interleavedThreadFunc);
                }
            }
            
            delete data;
        }
        
        displayResults(blockResults, interleavedResults, baselineTimes);
    }
    
    void displayResults(double block[3][3], double interleaved[3][3], double baseline[3]) {
        BenchmarkManager dummyBench(&blockStrat);
        
        cout << "| 1 (Division por Filas)           |";
        for (int i = 0; i < 3; i++) {
            cout << fixed << setprecision(3) << setw(6) << block[i][0] << " 1.000 |";
        }
        cout << endl;
        
        cout << "| 2                                |";
        for (int i = 0; i < 3; i++) {
            double eff = dummyBench.computeEfficiency(baseline[i], block[i][1], 2);
            cout << fixed << setprecision(3) << setw(6) << block[i][1] << " " << setw(5) << eff << " |";
        }
        cout << endl;
        
        cout << "| 4                                |";
        for (int i = 0; i < 3; i++) {
            double eff = dummyBench.computeEfficiency(baseline[i], block[i][2], 4);
            cout << fixed << setprecision(3) << setw(6) << block[i][2] << " " << setw(5) << eff << " |";
        }
        cout << endl;
        
        cout << "|----------------------------------|---------------|--------------|----------------|" << endl;
        
        cout << "| 1 (Division Ciclica)             |";
        for (int i = 0; i < 3; i++) {
            cout << fixed << setprecision(3) << setw(6) << interleaved[i][0] << " 1.000 |";
        }
        cout << endl;
        
        cout << "| 2                                |";
        for (int i = 0; i < 3; i++) {
            double eff = dummyBench.computeEfficiency(baseline[i], interleaved[i][1], 2);
            cout << fixed << setprecision(3) << setw(6) << interleaved[i][1] << " " << setw(5) << eff << " |";
        }
        cout << endl;
        
        cout << "| 4                                |";
        for (int i = 0; i < 3; i++) {
            double eff = dummyBench.computeEfficiency(baseline[i], interleaved[i][2], 4);
            cout << fixed << setprecision(3) << setw(6) << interleaved[i][2] << " " << setw(5) << eff << " |";
        }
        cout << endl;
        
        cout << "    ======" << endl;
    }
    
    void displayFooter() {
        cout << "\ntiempos en segundos" << endl;
        cout << "eficiencia = tiempo_serial / (tiempo_paralelo * num_threads)" << endl;
        cout << "dimensiones probadas: 8,000,000 x 8, 8000 x 8000, 8 x 8,000,000" << endl;
        
        cout << "\n=== analisis de rendimiento ===" << endl;
        cout << "- division por filas: cada thread procesa un bloque continuo de filas" << endl;
        cout << "- division ciclica: cada thread procesa filas alternadas (mejor balance de carga)" << endl;
        cout << "- eficiencia ideal = 1.0 (speedup lineal)" << endl;
        cout << "- matrices anchas (pocas filas) escalan peor" << endl;
        cout << "- matrices altas (muchas filas) escalan mejor" << endl;
    }
};

//   Función principal  
int main(int argc, char* argv[]) {
    if (argc != 1) {
        return 1;
    }
    
    ResultPresenter presenter;
    presenter.displayHeader();
    presenter.runExperiments();
    presenter.displayFooter();
    
    return 0;
}
