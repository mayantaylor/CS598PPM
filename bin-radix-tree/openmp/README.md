OpenMP implementation of histogram sort.

Implementation details:

- exactly like the serial implementation, with added parallel for loops

Usage:

- compile based on platform: `g++ -fopenmp tree.cpp -o tree`
- run: `./tree <nelements> `
