# to copy to the concole: export PATH=/usr/local/cuda/bin:$PATH
demo:
	nvcc demo.cu -o demo -arch=sm_60 -std=c++11 -I/usr/local/cuda/include

pde:
	nvcc difference_scheme.cu -o pde -arch=sm_60 -std=c++11 -I/usr/local/cuda/include

sum_distances:
	nvcc sum_distances.cu -o sum_distances -arch=sm_60 -std=c++11 -I/usr/local/cuda/include

clean:
	rm -f demo
	rm -f sum_distances
	rm -f pde
