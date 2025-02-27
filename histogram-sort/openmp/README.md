OpenMP implementation of histogram sort.

Implementation details:
- individual bucket for each integer value
- all values generated randomly
- histogram built with parallel for
- histogram scan done via prefix sum
- global index for each element computed and elements inserted

Usage:
- compile based on platform: `g++ -fopenmp histo.cpp -o histo`
- run: `./histo <nelements> <nbins>`