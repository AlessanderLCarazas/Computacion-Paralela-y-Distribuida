// EJERCICIO 1: SUMA DE MATRICES EN CUDA
#include <stdio.h>
#include <stdlib.h>
#include <cuda_runtime.h>

// parte b: kernel donde cada thread produce un elemento de la matriz
__global__ void matrixAddKernel_OneElementPerThread(float* C, float* A, float* B, int N) {
    // calcular la fila y columna del elemento que procesara este thread
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    // verificar que estamos dentro de los limites de la matriz
    if (row < N && col < N) {
        int index = row * N + col;
        C[index] = A[index] + B[index];
    }
}

// parte c: kernel donde cada thread produce una fila completa
__global__ void matrixAddKernel_OneRowPerThread(float* C, float* A, float* B, int N) {
    // cada thread procesa una fila completa
    int row = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (row < N) {
        // iterar sobre todas las columnas de esta fila
        for (int col = 0; col < N; col++) {
            int index = row * N + col;
            C[index] = A[index] + B[index];
        }
    }
}

// parte d: kernel donde cada thread produce una columna completa
__global__ void matrixAddKernel_OneColumnPerThread(float* C, float* A, float* B, int N) {
    // cada thread procesa una columna completa
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (col < N) {
        // iterar sobre todas las filas de esta columna
        for (int row = 0; row < N; row++) {
            int index = row * N + col;
            C[index] = A[index] + B[index];
        }
    }
}

// parte a: funcion host stub para ejecutar la suma de matrices
void matrixAddition(float* h_C, float* h_A, float* h_B, int N, int kernelType) {
    // calcular el tamano en bytes de las matrices
    size_t size = N * N * sizeof(float);
    
    // 1. allocar memoria en el device (gpu)
    float *d_A, *d_B, *d_C;
    cudaMalloc((void**)&d_A, size);
    cudaMalloc((void**)&d_B, size);
    cudaMalloc((void**)&d_C, size);
    
    // 2. transferir datos de entrada del host al device
    cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);
    cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice);
    
    // 3. configurar y lanzar el kernel segun el tipo
    
    if (kernelType == 1) {
        // parte b: un elemento por thread - configuracion 2d
        printf("\n--- parte b: un elemento por thread ---\n");
        dim3 blockDim(16, 16);
        dim3 gridDim((N + blockDim.x - 1) / blockDim.x, 
                     (N + blockDim.y - 1) / blockDim.y);
        
        printf("calculo del grid:\n");
        printf("  grid.x = (%d + %d - 1) / %d = %d\n", N, blockDim.x, blockDim.x, gridDim.x);
        printf("  grid.y = (%d + %d - 1) / %d = %d\n", N, blockDim.y, blockDim.y, gridDim.y);
        printf("configuracion:\n");
        printf("  grid: (%d, %d) bloques\n", gridDim.x, gridDim.y);
        printf("  block: (%d, %d) threads\n", blockDim.x, blockDim.y);
        printf("calculo total threads:\n");
        printf("  total threads = %d * %d * %d * %d = %d\n", 
               gridDim.x, gridDim.y, blockDim.x, blockDim.y,
               gridDim.x * gridDim.y * blockDim.x * blockDim.y);
        
        matrixAddKernel_OneElementPerThread<<<gridDim, blockDim>>>(d_C, d_A, d_B, N);
        
    } else if (kernelType == 2) {
        // parte c: una fila por thread - configuracion 1d
        printf("\n--- parte c: una fila por thread ---\n");
        int threadsPerBlock = 256;
        int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
        
        printf("calculo del grid:\n");
        printf("  bloques necesarios = (%d + %d - 1) / %d = %d\n", 
               N, threadsPerBlock, threadsPerBlock, blocksPerGrid);
        printf("configuracion:\n");
        printf("  grid: %d bloques\n", blocksPerGrid);
        printf("  block: %d threads\n", threadsPerBlock);
        printf("calculo total threads:\n");
        printf("  total threads = %d * %d = %d\n", 
               blocksPerGrid, threadsPerBlock, blocksPerGrid * threadsPerBlock);
        
        matrixAddKernel_OneRowPerThread<<<blocksPerGrid, threadsPerBlock>>>(d_C, d_A, d_B, N);
        
    } else if (kernelType == 3) {
        // parte d: una columna por thread - configuracion 1d
        printf("\n--- parte d: una columna por thread ---\n");
        int threadsPerBlock = 256;
        int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;
        
        printf("calculo del grid:\n");
        printf("  bloques necesarios = (%d + %d - 1) / %d = %d\n", 
               N, threadsPerBlock, threadsPerBlock, blocksPerGrid);
        printf("configuracion:\n");
        printf("  grid: %d bloques\n", blocksPerGrid);
        printf("  block: %d threads\n", threadsPerBlock);
        printf("calculo total threads:\n");
        printf("  total threads = %d * %d = %d\n", 
               blocksPerGrid, threadsPerBlock, blocksPerGrid * threadsPerBlock);
        
        matrixAddKernel_OneColumnPerThread<<<blocksPerGrid, threadsPerBlock>>>(d_C, d_A, d_B, N);
    }
    
    // esperar a que el kernel termine
    cudaDeviceSynchronize();
    
    // verificar errores de cuda
    cudaError_t error = cudaGetLastError();
    if (error != cudaSuccess) {
        printf("error cuda: %s\n", cudaGetErrorString(error));
    }
    
    // 4. transferir resultado del device al host
    cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost);
    
    // 5. liberar memoria del device
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

bool verifyResult(float* A, float* B, float* C, int N) {
    float tolerance = 1e-5;
    for (int i = 0; i < N * N; i++) {
        float expected = A[i] + B[i];
        if (fabs(C[i] - expected) > tolerance) {
            printf("error en posicion %d: esperado %.2f, obtenido %.2f\n", 
                   i, expected, C[i]);
            return false;
        }
    }
    return true;
}

// funcion main
int main(int argc, char* argv[]) {
    printf("=============================================================\n");
    printf("ejercicio 1: suma de matrices en cuda\n");
    printf("=============================================================\n\n");
    
    // tamano de la matriz (nxn)
    int N = 1024;
    
    if (argc > 1) {
        N = atoi(argv[1]);
    }
    
    printf("tamano de matriz: %d x %d\n", N, N);
    printf("total de elementos: %d\n", N * N);
    printf("memoria por matriz: %.2f mb\n", (N * N * sizeof(float)) / (1024.0 * 1024.0));
    
    // allocar memoria en el host
    size_t size = N * N * sizeof(float);
    float* h_A = (float*)malloc(size);
    float* h_B = (float*)malloc(size);
    float* h_C = (float*)malloc(size);
    
    // inicializar matrices a y b con valores aleatorios
    srand(2025);
    initializeMatrix(h_A, N);
    initializeMatrix(h_B, N);
    
    // imprimir muestra de las matrices de entrada
    printMatrix(h_A, N, "matriz a");
    printMatrix(h_B, N, "matriz b");
    
    printf("\n=============================================================\n");
    
    // probar parte b: un elemento por thread
    matrixAddition(h_C, h_A, h_B, N, 1);
    if (verifyResult(h_A, h_B, h_C, N)) {
        printf("resultado correcto\n");
    } else {
        printf("error en resultado\n");
    }
    printMatrix(h_C, N, "resultado c (parte b)");
    
    // probar parte c: una fila por thread
    matrixAddition(h_C, h_A, h_B, N, 2);
    if (verifyResult(h_A, h_B, h_C, N)) {
        printf("resultado correcto\n");
    } else {
        printf("error en resultado\n");
    }
    printMatrix(h_C, N, "resultado c (parte c)");
    
    // probar parte d: una columna por thread
    matrixAddition(h_C, h_A, h_B, N, 3);
    if (verifyResult(h_A, h_B, h_C, N)) {
        printf("resultado correcto\n");
    } else {
        printf("error en resultado\n");
    }
    printMatrix(h_C, N, "resultado c (parte d)");

    // liberar memoria del host
    free(h_A);
    free(h_B);
    free(h_C);

    return 0;
}
