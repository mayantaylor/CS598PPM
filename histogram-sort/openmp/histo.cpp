#include <iostream>
#include <vector>
#include <omp.h>

// Function to perform histogram sort using OpenMP
void histogramSort(std::vector<int> &arr, int num_bins)
{
    int n = arr.size();
    std::vector<int> histogram(num_bins, 0);

// Step 1: Compute histogram counts in parallel
#pragma omp parallel for shared(arr, histogram)
    for (int i = 0; i < n; ++i)
    {
#pragma omp atomic
        histogram[arr[i]]++;
    }

    // Step 2: Reconstruct sorted array using histogram
    int index = 0;
#pragma omp parallel for shared(arr, histogram) private(index)
    for (int bin = 0; bin < num_bins; ++bin)
    {
        for (int count = 0; count < histogram[bin]; ++count)
        {
            arr[index++] = bin;
        }
    }
}

int main()
{
    std::vector<int> arr = {2, 5, 1, 3, 2, 4, 1, 0};
    int num_bins = 6; // Adjust this based on the range of values in your array

    std::cout << "Original array:";
    for (int num : arr)
    {
        std::cout << " " << num;
    }
    std::cout << std::endl;

    // Perform histogram sort
    histogramSort(arr, num_bins);

    std::cout << "Sorted array:";
    for (int num : arr)
    {
        std::cout << " " << num;
    }
    std::cout << std::endl;

    return 0;
}
