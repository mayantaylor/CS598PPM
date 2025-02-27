#include <iostream>
#include <vector>
#include <omp.h>
#include <cmath>
#include <math.h>
#include <chrono>
#include <cassert>

void histogramSort(std::vector<int> &arr, int num_bins)
{

  auto start_sort = std::chrono::high_resolution_clock::now();
  int n = arr.size();
  std::vector<int> histogram(num_bins, 0);
  
  // build histogram (in parallel)
#pragma omp parallel for 
  for (int i = 0; i < n; ++i)
    {    
      int bin = arr[i];      
#pragma omp atomic
      histogram[bin]++;
    }
       
  // build histogram scan buffer
  std::vector<int> scan(num_bins, 0);
  std::copy(histogram.begin() ,histogram.end(),scan.begin()); 
  

  // do prefix sum on the histogram (each stride is parallelized)
  for (int j = 0; j < log2(num_bins); j++) {
    int stride = pow(2,j);
    
    std::vector<int> newscan(num_bins, 0);
    std::copy(scan.begin(), scan.end(), newscan.begin());
    
#pragma omp parallel for shared(newscan, scan)
    for (int i = 0; i < num_bins - stride; i++) {
      newscan[i+stride] = scan[i+stride] + scan[i];
    }
    scan = newscan;   // double buffering
  }    
  
  // sanity check for scan
  assert(scan[num_bins - 1] == n);
  
  // do sort based on scan
  std::vector<int> sorted(n);
  std::vector<int> bincount(num_bins, 0);
  

  // compute global index for every data element, in parallel
  // this parallelism is only a benefit when the number of bins is large
#pragma omp parallel for 
  for (int i = 0; i < n; i++) {
    // compute global index
    
    int bin = arr[i];
    int pastbins = scan[bin] - histogram[bin];
    int globalId;
    
#pragma omp atomic capture
    globalId = bincount[bin]++;
    
    globalId += pastbins;
    sorted[globalId] = arr[i];
    
  }


  auto end_sort = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_sort = end_sort - start_sort;


    // assert the array is sorted, serially
    for (int i = 1; i < n; i++) {
      assert(sorted[i] >= sorted[i-1]);
    }

      std::cout << "Histogram sort verified in time: " << elapsed_sort.count() << " seconds" << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <vector_size> <nbins>\n";
    return 1;
  }

  int size = std::atoi(argv[1]); 
  
  std::vector<int> arr(size, 0);
  int num_bins = std::atoi(argv[2]); 
  
  srand(time(NULL));
  for (int i = 0; i < size; i++) {
    arr[i] = std::rand() % num_bins;
  }
  
  histogramSort(arr, num_bins);

  return 0;
}
