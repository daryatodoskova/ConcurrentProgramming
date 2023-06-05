#include <algorithm>
#include <iostream>
#include <chrono>
#include <math.h>

const double C = 0.5;

//------------------------------------------------

void SolvePDE(double* boundary_values, size_t N, double dx, double dt, size_t timesteps, double* result) {
    double* curr = (double*) malloc(N * sizeof(double));
    double* next = (double*) malloc(N * sizeof(double));
    memcpy(curr, boundary_values, N * sizeof(double));
    for (size_t i = 0; i < timesteps; ++i) {
        for (size_t j = 0; j < N; ++j) {
            if (j < N - 1) {
                next[j] = curr[j] + C * (dt / dx) * (curr[j + 1] - curr[j]);
            } else {
                next[j] = curr[j] + C * (dt / dx) * (curr[0] - curr[j]);
            }
        }
        std::swap(curr, next);
    }
    memcpy(result, curr, N * sizeof(double));
    free(curr);
    free(next);
}

//-------------------------------------------------

//CUDA kernel to perform the PDE computation on the GPU
__global__
void PDEAux(double* curr, double* next, size_t N, double dx, double dt){
    size_t index = blockIdx.x * blockDim.x + threadIdx.x;  //index of thread
    if (index >= N) {
        return;
    }
    //if not last element
    if (index < N - 1) {
        next[index] = curr[index] + C * (dt / dx) * (curr[index+1] - curr[index]);
    }
    else {
        next[index] = curr[index] + C * (dt / dx) * (curr[0] - curr[index]); 
    }
}

// __global__
// void SolvePDEGPUAux(double* curr, double* next, size_t N, double dx, double dt){
//     size_t index = blockIdx.x * blockDim.x + threadIdx.x;  
//     if (index >= N) {
//         return;
//     }
//     if (index < N - 1) {
//     
//     }
// }

/**
 * @brief Solves a PDE u_t = C * u_x using a simple difference scheme
 * @param boundary_values - the pointer to the beginning of an array of values at t = 0
 * @param N - the length of the array
 * @param dx - step size for x coordinate
 * @param dt - step size for t coordinate
 * @param timesteps - number of steps in time to preform
 * @param result - pointer to yhe array for the value at the last time step
 */

void SolvePDEGPU(double* boundary_values, size_t N, double dx, double dt, size_t timesteps, double* result) {
    const size_t THREADS_PER_BLOCK = 64; 
    const size_t TOTAL_THREADS = N; 

    double *currd; //to the current array of values
    double *nextd; //to the next array of values
    cudaMalloc(&currd, N * sizeof(double));
    cudaMalloc(&nextd, N * sizeof(double));
    cudaMemcpy(currd, boundary_values, N * sizeof(double), cudaMemcpyHostToDevice);

    //number of blocks needed to run all threads
    const size_t NUM_BLOCKS = (TOTAL_THREADS + THREADS_PER_BLOCK - 1)/THREADS_PER_BLOCK; 
    //const size_t NUM_BLOCKS = (TOTAL_THREADS - 1)/THREADS_PER_BLOCK; 

    for (size_t t = 0; t < timesteps; ++t){
        //calling the kernel function 
        PDEAux<<<NUM_BLOCKS, THREADS_PER_BLOCK>>>(currd, nextd, N, dx, dt);
        cudaDeviceSynchronize();
        std::swap(currd, nextd);
    }

    cudaMemcpy(result, currd, N * sizeof(double), cudaMemcpyDeviceToHost);

    cudaFree(currd);
    cudaFree(nextd);
}

//---------------------------------------------------

__global__
void PDEAux2(double* curr, double* next, size_t N, double dx, double dt){
    extern __shared__ double bf[]; //buffer
    size_t index = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (index >= N) {
        return;
    }
    bf[threadIdx.x] = curr[index];
    __syncthreads();

    if (index < N - 1){
        if (threadIdx.x < blockDim.x - 1) {
            next[index] = bf[threadIdx.x] + C * (dt / dx) * (bf[threadIdx.x + 1] - bf[threadIdx.x]);  //compute next element value
        }
        else {
            //in block - last thread
            next[index] = bf[threadIdx.x] + C * (dt / dx) * (curr[index+1] - bf[threadIdx.x]); 
        }
    }
    else{
        //in array - last thread
        next[index] = bf[threadIdx.x] + C * (dt / dx) * (curr[0] - bf[threadIdx.x]);
    }
}

// __global__
// void PDEAux2(double* curr, double* next, size_t N, double dx, double dt){
//     extern __shared__ double bf[]; 
//     size_t index = blockIdx.x * blockDim.x + threadIdx.x;
//     if (index >= N) {
//         return;
//     }
//     bf[threadIdx.x] = curr[index];
//     __syncthreads();

//     if (threadIdx.x < blockDim.x - 1) {
//         next[index] = bf[threadIdx.x] + C * (dt / dx) * (bf[threadIdx.x + 1] - bf[threadIdx.x]); 
//     }
//     else {
//         next[index] = bf[threadIdx.x] + C * (dt / dx) * (curr[index+1] - bf[threadIdx.x]); 
//     }
// }

/**
 * @brief Solves a PDE u_t = C * u_x using a simple difference scheme
 * @param boundary_values - the pointer to the beginning of an array of values at t = 0
 * @param N - the length of the array
 * @param dx - step size for x coordinate
 * @param dt - step size for t coordinate
 * @param timesteps - number of steps in time to preform
 * @param result - pointer to yhe array for the value at the last time step
 */

void SolvePDEGPU2(double* boundary_values, size_t N, double dx, double dt, size_t timesteps, double* result) {
    const size_t THREADS_PER_BLOCK = 64;
    const size_t TOTAL_THREADS = N;
    const size_t NUM_BLOCKS = (TOTAL_THREADS + THREADS_PER_BLOCK - 1)/THREADS_PER_BLOCK; 

    double *currd; 
    double *nextd; 
    cudaMalloc(&currd, N * sizeof(double));
    cudaMalloc(&nextd, N * sizeof(double));
    cudaMemcpy(currd, boundary_values, N * sizeof(double), cudaMemcpyHostToDevice);

    for (size_t t = 0; t < timesteps; ++t){
        PDEAux2<<<NUM_BLOCKS, THREADS_PER_BLOCK, THREADS_PER_BLOCK * sizeof(double)>>>(currd, nextd, N, dx, dt);
        cudaDeviceSynchronize();
        std::swap(currd, nextd);  //swap the current and next arrays for the next time step
    }

    cudaMemcpy(result, currd, N * sizeof(double), cudaMemcpyDeviceToHost);

    cudaFree(currd);
    cudaFree(nextd);
}

//---------------------------------------------------

int main(int argc, char* argv[]) {
    // setting the random seed to get the same result each time
    srand(42);

    // taking as input, which algo to run
    int alg_ind = std::stoi(argv[1]);

    // Generating data
    double length = 8 * atan(1.0); // 2 pi
    double dx = 0.0001;
    double dt = 0.0001;
    size_t N = int(length / dx);

    double* boundary = (double*) malloc(N * sizeof(double));
    double* result = (double*) malloc(N * sizeof(double));
    for (size_t i = 0; i < N; ++i) {
        boundary[i] = sin(i * dx);
    }

    size_t timesteps = 10000;
    auto start = std::chrono::steady_clock::now();
    switch (alg_ind) {
        case 0: 
            SolvePDE(boundary, N, dx, dt, timesteps, result);
            break;
        case 1:
            SolvePDEGPU(boundary, N, dx, dt, timesteps, result);
            break;
        case 2:
            SolvePDEGPU2(boundary, N, dx, dt, timesteps, result);
            break;
    }
    auto finish = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count(); 
   
    for (size_t i = 0; i <  N; ++i) {
        std::cout << result[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Elapsed time: " << elapsed << std::endl;
 
    free(boundary);
    free(result);
    return 0;
}
