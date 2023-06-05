#include <algorithm>
#include <iostream>
#include <chrono>
#include <cmath>

__device__
double DistKer(double* p, double* q, size_t dim) {
    double result = 0;
    for (size_t i = 0; i < dim; ++i) {
        result += (p[i] - q[i]) * (p[i] - q[i]);
    }
    return std::sqrt(result);
}

double Dist(double* p, double* q, size_t dim) {
    double result = 0;
    for (size_t i = 0; i < dim; ++i) {
        result += (p[i] - q[i]) * (p[i] - q[i]);
    }
    return std::sqrt(result);
}

//------------------------------------------------

double SumDistances(double* arr, size_t dim, size_t N) {
    double result = 0.;
    for (size_t i = 0; i < N; ++i) {
        double* p = arr + i * dim;
        for (size_t j = i + 1; j < N; ++j) {
            result += Dist(p, arr + j * dim, dim);
        }
    }
    return result;
}

//-------------------------------------------------

__global__
void SumDistancesGPUAux(double* arr, size_t dim , size_t N, double* results_gpu) {
    size_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= N) {
        return;
    }
    double result = 0.;
    double* p = arr + index * dim;
    for (size_t i = index + 1; i < N; ++i) {
        result += DistKer(p, arr + i * dim, dim);
    }
    results_gpu[index] = result;
}

double SumDistancesGPU(double* arr, size_t dim, size_t N) {
    const size_t THREADS_PER_BLOCK = 256;
 
    // moving the input to the device
    double* arr_device;
    cudaMalloc(&arr_device, N * dim * sizeof(double));
    cudaMemcpy(arr_device, arr, N * dim * sizeof(double), cudaMemcpyHostToDevice);

    // allocating memory for the output
    double* results_gpu;
    cudaMalloc(&results_gpu, N * sizeof(double));

    // running calculation on GPU
    size_t blocks_num = (N + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    SumDistancesGPUAux<<<blocks_num, THREADS_PER_BLOCK>>>(arr_device, dim, N, results_gpu);
    cudaDeviceSynchronize();

    // Computing the final result
    double* results_cpu = (double*) malloc(N * sizeof(double));
    cudaMemcpy(results_cpu, results_gpu, N * sizeof(double), cudaMemcpyDeviceToHost);

    double final_result = 0;
    for (size_t i = 0; i < N; ++i) {
        final_result += results_cpu[i];
    }

    // Freeing the memory
    cudaFree(arr_device);
    cudaFree(results_gpu);
    free(results_cpu);
    return final_result; 
}

//---------------------------------------------------

__global__
void SumDistancesGPUAux2(double* arr, size_t dim , size_t N, double* results_gpu) {
    extern __shared__ double bf[];
    size_t index = blockIdx.x * blockDim.x * threadIdx.x;

    if (index >= N){
        return; 
    }

    for (size_t i = 0; i < dim; ++i){ 
        bf[threadIdx.x + i] = arr[index * dim + i];
    }
    __syncthreads();
    double* p = &bf[threadIdx.x];
    double result = 0.; 

    for (size_t i = threadIdx.x; i < blockDim.x; ++i){ 
        double* q = &bf[threadIdx.x + i * dim]; 
        result += DistKer(p, q, dim);
    }


    for (size_t b = blockIdx.x; b < gridDim.x; ++b){ 
        if (b != blockIdx.x){
            for (size_t thread_num = 0; thread_num < blockDim.x * dim; ++thread_num){ 
                for (size_t d = 0; d < dim; ++d){
                    bf[blockDim.x + (thread_num * dim) + d] = arr[(b * blockDim.x * dim) + (thread_num * dim) + d];
                }
            }
            __syncthreads();
            for (size_t thread_num = 0; thread_num < blockDim.x * dim; ++thread_num){ 
                double* q = &bf[blockDim.x + (thread_num * dim)];
                result += DistKer(p, q, dim);
            }
        }
    }
}

double SumDistancesGPU2(double* arr, size_t dim, size_t N) {
    const size_t TOTAL_THREADS = N;
    const size_t THREADS_PER_BLOCK = 64;
    size_t NUM_BLOCKS = (TOTAL_THREADS + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;

    // moving the input to the device
    double* arr_device;
    cudaMalloc(&arr_device, N * dim * sizeof(double));
    cudaMemcpy(arr_device, arr, N * dim * sizeof(double), cudaMemcpyHostToDevice);

    // allocating memory for the output
    double* results_gpu;
    cudaMalloc(&results_gpu, N * sizeof(double));

    // running calculation on GPU
    SumDistancesGPUAux2<<<NUM_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * dim * sizeof(double)>>>(arr_device, dim, N, results_gpu);
    cudaDeviceSynchronize(); 

    // Computing the final result
    double* results_cpu = (double*) malloc(N * sizeof(double));
    cudaMemcpy(results_cpu, results_gpu, N * sizeof(double), cudaMemcpyDeviceToHost);

    double final_result = 0;
    for (size_t i = 0; i < N; ++i) {
        final_result += results_cpu[i];
    }

    // Freeing the memory
    cudaFree(arr_device);
    cudaFree(results_gpu);
    free(results_cpu);
    return final_result; 
 
    return 0;
}

//---------------------------------------------------

int main(int argc, char* argv[]) {
    // setting the random seed to get the same result each time
    srand(42);

    // taking as input, which algo to run
    int alg_ind = std::stoi(argv[1]);

    // Generating data
    size_t N = 6400;
    size_t dim = 3;
    double* arr = (double*) malloc(N * dim * sizeof(double));
    for (size_t i = 0; i < dim * N; ++i) {
          arr[i] = static_cast <double> (rand()) / static_cast <double> (RAND_MAX);
    }
 
    // Computing on CPU
    double result = 0.;
    auto start = std::chrono::steady_clock::now();
    switch (alg_ind) {
        case 0: 
            result = SumDistances(arr, dim, N);
            break;
        case 1:
            result = SumDistancesGPU(arr, dim, N);
            break;
        case 2:
            result = SumDistancesGPU2(arr, dim, N);
            break;
    }
    auto finish = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count(); 
    std::cout << "Elapsed time: " << elapsed << std::endl;
    std::cout << "Total result: " << result << std::endl;
    
    free(arr);
    return 0;
}
