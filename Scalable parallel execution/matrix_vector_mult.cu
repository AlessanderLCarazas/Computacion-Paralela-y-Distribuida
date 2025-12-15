// EJERCICIO 2: MULTIPLICACION MATRIZ-VECTOR EN CUDA
#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>
#include <math.h>

// kernel: cada thread calcula un elemento del vector de salida
__global__ void matrixVectorMultKernel(float* A, float* B, float* C, int N) {
    // cada thread procesa una fila de la matriz b
    int row = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row < N) {
        float sum = 0.0f;
        // producto punto de la fila con el vector c
        for (int j = 0; j < N; j++) {
            sum += B[row * N + j] * C[j];
        }
        A[row] = sum;
    }
}

// funcion host stub
void matrixVectorMultiplication(float* h_A, float* h_B, float* h_C, int N) {
    // tamaño de memoria
    size_t sizeMatrix = N * N * sizeof(float);
    size_t sizeVector = N * sizeof(float);
    
    // allocar memoria en device
    float *d_A, *d_B, *d_C;
    cudaMalloc((void**)&d_A, sizeVector);
    cudaMalloc((void**)&d_B, sizeMatrix);
    cudaMalloc((void**)&d_C, sizeVector);
    
    // transferir datos del host al device
    cudaMemcpy(d_B, h_B, sizeMatrix, cudaMemcpyHostToDevice);
    cudaMemcpy(d_C, h_C, sizeVector, cudaMemcpyHostToDevice);
    
    // configuracion de ejecucion
    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
    
    printf("\nconfiguracion del kernel:\n");
    printf("  calculo de bloques: (%d + %d - 1) / %d = %d\n", 
           N, threadsPerBlock, threadsPerBlock, blocksPerGrid);
    printf("  grid: %d bloques\n", blocksPerGrid);
    printf("  block: %d threads\n", threadsPerBlock);
    printf("  total threads: %d * %d = %d\n", 
           blocksPerGrid, threadsPerBlock, blocksPerGrid * threadsPerBlock);
    printf("  cada thread calcula: 1 elemento del vector (producto punto de 1 fila)\n");
    
    // lanzar kernel
    matrixVectorMultKernel<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, N);
    
    // sincronizar
    cudaDeviceSynchronize();
    
    // verificar errores
    cudaError_t error = cudaGetLastError();
    if (error != cudaSuccess) {
        printf("error cuda: %s\n", cudaGetErrorString(error));
    }
    
    // transferir resultado del device al host
    cudaMemcpy(h_A, d_A, sizeVector, cudaMemcpyDeviceToHost);
    
    // liberar memoria del device
    cudaFree(d_A);
    cudaFree(d_B);
    cudaFree(d_C);
}

// funciones auxiliares
void initializeMatrix(float* matrix, int N) {
    for (int i = 0; i < N * N; i++) {
        matrix[i] = (float)(rand() % 100) / 10.0f;
    }
}

void initializeVector(float* vector, int N) {
    for (int i = 0; i < N; i++) {
        vector[i] = (float)(rand() % 100) / 10.0f;
    }
}

void printMatrix(float* matrix, int N, const char* name) {
    printf("\n%s:\n", name);
    int limit = (N < 8) ? N : 8;
    for (int i = 0; i < limit; i++) {
        printf("  ");
        for (int j = 0; j < limit; j++) {
            printf("%6.2f ", matrix[i * N + j]);
        }
        if (N > 8) printf("...");
        printf("\n");
    }
    if (N > 8) printf("  ...\n");
}

void printVector(float* vector, int N, const char* name) {
    printf("\n%s:\n  ", name);
    int limit = (N < 8) ? N : 8;
    for (int i = 0; i < limit; i++) {
        printf("%6.2f ", vector[i]);
    }
    if (N > 8) printf("...");
    printf("\n");
}

bool verifyResult(float* B, float* C, float* A, int N) {
    float tolerance = 1e-2;
    for (int i = 0; i < N; i++) {
        float expected = 0.0f;
        for (int j = 0; j < N; j++) {
            expected += B[i * N + j] * C[j];
        }
        if (fabs(A[i] - expected) > tolerance) {
            printf("error en posicion %d: esperado %.4f, obtenido %.4f\n", 
                   i, expected, A[i]);
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    printf("=============================================================\n");
    printf("ejercicio 2: multiplicacion matriz-vector en cuda\n");
    printf("=============================================================\n\n");
    
    // tamaño de la matriz (nxn) y vector (n)
    int N = 1024;
    
    if (argc > 1) {
        N = atoi(argv[1]);
    }
    
    printf("tamano de matriz: %d x %d\n", N, N);
    printf("tamano de vector: %d\n", N);
    printf("memoria matriz: %.2f mb\n", (N * N * sizeof(float)) / (1024.0 * 1024.0));
    printf("memoria vector: %.2f kb\n", (N * sizeof(float)) / 1024.0);
    
    // allocar memoria en el host
    size_t sizeMatrix = N * N * sizeof(float);
    size_t sizeVector = N * sizeof(float);
    float* h_A = (float*)malloc(sizeVector);  // vector salida
    float* h_B = (float*)malloc(sizeMatrix);  // matriz entrada
    float* h_C = (float*)malloc(sizeVector);  // vector entrada
    
    // inicializar matriz b y vector c con valores aleatorios
    srand(2025);
    initializeMatrix(h_B, N);
    initializeVector(h_C, N);
    
    // imprimir muestra de los datos de entrada
    printMatrix(h_B, N, "matriz b");
    printVector(h_C, N, "vector c");
    
    printf("\n=============================================================\n");
    
    // ejecutar multiplicacion matriz-vector
    matrixVectorMultiplication(h_A, h_B, h_C, N);
    
    // verificar resultado
    printf("\nverificando resultado...\n");
    if (verifyResult(h_B, h_C, h_A, N)) {
        printf("resultado correcto: todos los %d elementos verificados\n", N);
    } else {
        printf("error en resultado\n");
    }
    
    // imprimir muestra del resultado
    printVector(h_A, N, "vector resultado a");
    
    // liberar memoria del host
    free(h_A);
    free(h_B);
    free(h_C);
    
    return 0;
}
