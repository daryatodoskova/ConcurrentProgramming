#include <algorithm>
#include <iostream>
#include <chrono>
#include <math.h>

__global__
void increment(int* counters, size_t N, int times) {
    size_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= N) {
        return;
    }
    for (size_t i = 0; i < times; ++i) {
        ++counters[index];
    }
}

__global__
void increment2(int* counters, size_t N, int times) {
    extern __shared__ int buffer[];
    size_t index = blockIdx.x * blockDim.x + threadIdx.x;
    if (index >= N) {
        return;
    }
    buffer[threadIdx.x] = counters[index];
    __syncthreads(); // not really necessary here but typically used

    for (size_t i = 0; i < times; ++i) {
        ++buffer[threadIdx.x];
    }
    __syncthreads();
    counters[index] = buffer[threadIdx.x];
}



//----------------------------------------------------

void RunIncrementer(size_t N, int times, int type) {
    const size_t THREADS_PER_BLOCK = 256;

    // allocating data on device
    int* counters; 
    cudaMalloc(&counters, N * sizeof(int));

    // computing on GPU
    size_t num_blocks = (N + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    if (type == 0) {
        increment<<<num_blocks, THREADS_PER_BLOCK>>>(counters, N, times);
    } else {
        increment2<<<num_blocks, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(int)>>>(counters, N, times);
    }
    cudaDeviceSynchronize();

    // Free memory
    cudaFree(counters);
}

//----------------------------------------------------

int main(int argc, char* argv[]) {
    // setting the random seed to get the same result each time
    srand(42);

    // taking as input, which algo to run
    int alg_ind = std::stoi(argv[1]);
    size_t N = std::stoi(argv[2]);
    int times = std::stoi(argv[3]);

    auto start = std::chrono::steady_clock::now();
    RunIncrementer(N, times, alg_ind);
    auto finish = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count(); 
    std::cout << "Elapsed time: " << elapsed << std::endl;
    
    return 0;
}
