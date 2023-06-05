#include <chrono>
#include <climits>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "CoarseSetList.cpp"
#include "SetList.cpp"

template <typename T>
void insert_into_set(T& set, int count) {
    for (int i = 0; i < count; ++i) {
        set.add(std::to_string(rand()));
    }
}

template <typename T>
int benchmark_single_thread(T& set, int count) {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < count; ++i) {
        set.add(std::to_string(rand()));
    }    
    auto finish = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    return elapsed;
}

template <typename T>
int benchmark_multithread(T& set, int count, int num_threads) {
    std::vector<std::thread> threads(num_threads);
    auto start = std::chrono::steady_clock::now();
    size_t num_per_thread = count / num_threads;
    for (int i = 0; i < num_threads; ++i) {
        threads[i] = std::thread(insert_into_set<T>, std::ref(set), num_per_thread);
    }
    for (int i = 0; i < num_threads; ++i) {
        threads[i].join();
    }
    auto finish = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    return elapsed;
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./set_benchmarker num_thread num_insertions" << std::endl;
        return 0;
    }

    int num_threads = std::stoi(argv[1]);
    int num_insertions = std::stoi(argv[2]);

    if (num_threads == 1) {
        // Timing for coarse-grained
        CoarseSetList CL;
        int elapsed = benchmark_single_thread(CL, num_insertions);
        std::cout << "Time for coare-grained version is " << elapsed << " microseconds" << std::endl;
    
        // Timing for fine-grained
        SetList L;
        elapsed = benchmark_single_thread(L, num_insertions);        
        std::cout << "Time for fine-grained version is " << elapsed << " microseconds" << std::endl;
    } else {
        // Timing for coarse-grained
        CoarseSetList CL;
        int elapsed = benchmark_multithread(CL, num_insertions, num_threads);
        std::cout << "Time for coare-grained version is " << elapsed << " microseconds" << std::endl;
            
        // Timing for fine-grained
        SetList L;
        elapsed = benchmark_multithread(L, num_insertions, num_threads);
        std::cout << "Time for fine-grained version is " << elapsed << " microseconds" << std::endl;
    }
}

/*

SPACE TO REPORT AND ANALYZE THE RUNTIMES

1st column : number of insertions; 
2nd column : time taken by the coarse-grained version; 
3rd column : time taken by the fine-grained version.

Running analysis for 1 thread
1000 4948 1332
2000 20259 5397
3000 44629 13782
4000 81691 19040
5000 108898 31373
6000 150817 49256
7000 196944 71503
8000 263330 103770
9000 326477 131277
10000 401174 173792
20000 1596599 976681
30000 3637425 2489680
40000 6595531 4656615
50000 10351708 7609578

Running analysis for 2 threads
1000 4779 1515
2000 19325 7181
3000 44432 16866
4000 73820 22774
5000 97838 38312
6000 140820 58739
7000 188191 75106
8000 244039 119699
9000 298169 151389
10000 378104 185707
20000 1447891 984034
30000 3203471 2704843
40000 5832902 4607614
50000 8895201 7397618

Running analysis for 3 threads
1000 5664 3324
2000 14858 9552
3000 30649 17667
4000 57384 29662
5000 74486 42091
6000 103353 66473
7000 143083 88010
8000 186419 120310
9000 237349 162553
10000 281628 201380
20000 1087065 1027247
30000 2430964 2475930
40000 4325742 4635811
50000 6736674 7400736

Running analysis for 4 threads
1000 6267 3432
2000 12479 9883
3000 25249 19279
4000 46117 28802
5000 60485 42007
6000 85459 64894
7000 115837 91638
8000 152183 126744
9000 193616 160291
10000 237624 203424
20000 881929 1022311
30000 1970945 2481310
40000 3405457 4942941
50000 5428176 8438209

Running analysis for 5 threads
1000 5458 4420
2000 14338 10585
3000 24943 17583
4000 34380 27532
5000 54335 44196
6000 77092 63291
7000 98221 94473
8000 124527 124002
9000 158678 160768
10000 199534 201876
20000 757542 1027841
30000 1642389 2557692
40000 2885451 4679306
50000 4427435 7946775

Running analysis for 6 threads
1000 4474 3795
2000 11815 9947
3000 21738 16921
4000 32823 28424
5000 55595 48838
6000 85655 63374
7000 93021 90205
8000 115462 121302
9000 145219 162142
10000 184015 204431
20000 682286 1024055
30000 1506424 2571068
40000 2608603 4968075
50000 4766774 7913712

We can observe that the fine-grained version seems faster for a small number of insertions, 
and seems slower for a larger number of insertions. The same goes with the number of threads.

*/
