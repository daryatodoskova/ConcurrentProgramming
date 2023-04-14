#pragma once
#include <cfloat>
#include <climits>
#include <thread>
#include <numeric>
#include <iterator>
#include <vector>
#include <mutex>
#include <chrono>
#include <iostream>

/**
 * @brief Finds the maximum in the array in parallel
 * @param start - pointer to the beginning of the array
 * @param N - length of the array
 * @param num_threads - the number of threads to be used
 */
double MaxParallel(double* start, size_t N, size_t num_threads) {
    if (N == 0) {
        return 0.;
    }
    // Divide the array into chunks for parallel processing
    size_t block_size = N / num_threads;
    // Create a vector to store the maximum values found in each chunk
    std::vector<double> max_values(num_threads, -DBL_MAX);
    // Create a vector of threads to execute the function in parallel
    std::vector<std::thread> workers(num_threads);
    // Start a thread for each chunk of the array
    for (size_t i = 0; i < num_threads; ++i) {
        workers[i] = std::thread([=, &max_values] {
            // Compute the start and end indices of the chunk
            size_t start_index = i * block_size;
            size_t end_index = (i + 1) * block_size;
            if (i == num_threads - 1) {
                end_index = N;
            }
            // Find the maximum value in the chunk
            for (size_t j = start_index; j < end_index; ++j) {
                if (start[j] > max_values[i]) {
                    max_values[i] = start[j];
                }
            }
        });  
    }
    // Wait for all threads to finish executing
    for (size_t i = 0; i < num_threads; ++i) {
        workers[i].join();
    }
    // Find the maximum value in the vector of maximum values
    return *std::max_element(max_values.begin(), max_values.end());
}



//-----------------------------------------------------------------------------

// This function computes the maximum value of the array elements up to the current position, 
// taking into account an offset value
void PartialMaxSeq(double* start, size_t N, double* res_start, double offset) {
    if (start[0] > offset){
        res_start[0] = start[0];
    }
    else{
        res_start[0] = offset;
    }
    for (size_t i = 1; i < N; ++i) {
        if (res_start[i-1] < start[i]){
            res_start[i] = start[i];
        }
        else{
            res_start[i] = res_start[i-1];
        }
    }
}

// This function replaces all array elements smaller than the given offset value with the offset value
void MaxOffset(double* start, size_t N, double offset) {
    for (size_t i = 0; i < N; ++i) {
        if (start[i] < offset){
            start[i] = offset;
        }
    }
}

/**
 * @brief Computes the maximums of all the prefixes of the array start
 * @param start - pointer to the beginning of the array
 * @param N - number of elements
 * @param num_threads - number of threads to be used
 * @param res_start - pointer to the beginning of the result array
 */
//computes the maximums of the prefixes of the array start of length N 
//using num_threads and stores them in the results array.

void PrefixMaximums(double* start, size_t N, size_t num_threads, double* res_start) {
    size_t chunk_length = N / (num_threads + 1);
    std::thread workers[num_threads-1];
    size_t offset = 0;

    // Compute the maximums of prefixes for each chunk in a separate thread
    for (size_t i = 0; i < num_threads - 1; ++i) {
        workers[i] = std::thread(&PartialMaxSeq, start + offset, chunk_length, res_start + offset, 0.);
        offset += chunk_length;
    }
    // Compute the maximums of prefixes for the last chunk in the main thread
    PartialMaxSeq(start + offset, chunk_length, res_start + offset, 0.);

    // Wait for all the worker threads to finish
    for (size_t i = 0; i < num_threads - 1; ++i) {
        workers[i].join();
    }

    // Compute the offset for each thread
    double offsets[num_threads];
    offsets[0] = res_start[chunk_length - 1];
    for (size_t i = 1; i < num_threads; ++i) {
        if (offsets[i-1] <= res_start[chunk_length * (i + 1) - 1]){
            offsets[i] = res_start[chunk_length * (i + 1) - 1];
        }
        else{
            offsets[i] = offsets[i - 1];
        }
    }
    // Apply the offset to each chunk
    offset = chunk_length;
    for (size_t i = 0; i < num_threads - 1; ++i) {
        workers[i] = std::thread(&MaxOffset, res_start + offset, chunk_length, offsets[i]);
        offset += chunk_length;
    }
    // Compute the maximums of prefixes for the remaining elements in the main thread
    PartialMaxSeq(start + chunk_length * num_threads, N - chunk_length * num_threads, res_start + chunk_length * num_threads, offsets[num_threads - 1]);
    for (size_t i = 0; i < num_threads - 1; ++i) {
        workers[i].join();
    }
}


//-----------------------------------------------------------------------------
