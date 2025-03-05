#include <cuda_runtime.h>
#include <iostream>
#include <vector>

struct Node
{
    bool isLeaf;
    int data;

    int left;
    bool leftIsLeaf;

    int right;
    bool rightIsLeaf;
};

__device__ int signDevice(int x)
{
    return (x >= 0) - (x < 0);
}

__device__ int commonPrefixLengthDevice(int a, int b)
{
    if (a == b)
        return sizeof(int) * 8; // If numbers are equal, full bit length match

    int xorVal = a ^ b;   // Find differing bits
    return __clz(xorVal); // Count leading zeros
}

__device__ int computeCPLDevice(int *randomInts, int size, int i, int j)
{
    if (j >= size || j < 0)
        return -1;

    return commonPrefixLengthDevice(randomInts[i], randomInts[j]);
}

__global__ void buildInternalKernel(int *randomInts, Node *leaves, Node *internalNodes, int size)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= size)
        return;

    // determine range direction
    int d = signDevice(computeCPLDevice(randomInts, size + 1, i, i + 1) - computeCPLDevice(randomInts, size + 1, i, i - 1));

    int minCPL = computeCPLDevice(randomInts, size + 1, i, i - d);

    // determine range extent
    int ub = i;
    while (computeCPLDevice(randomInts, size + 1, i, ub + d) > minCPL)
    {
        ub += d;
    }

    int dNode = computeCPLDevice(randomInts, size + 1, i, ub);

    // find split point
    int s = 0;
    while (computeCPLDevice(randomInts, size + 1, i, i + (s + 1) * d) > dNode)
    {
        s++;
    }

    int y = i + s * d + min(d, 0);

    int left = y;
    bool leftIsLeaf = false;

    int right = y + 1;
    bool rightIsLeaf = false;
    if (min(i, ub) == y)
        leftIsLeaf = true;

    if (max(i, ub) == y + 1)
        rightIsLeaf = true;

    internalNodes[i].data = i;
    internalNodes[i].left = left;
    internalNodes[i].leftIsLeaf = leftIsLeaf;

    internalNodes[i].right = right;
    internalNodes[i].rightIsLeaf = rightIsLeaf;
}

void buildInternalNodes(std::vector<int> &randomInts, std::vector<Node> &internalNodes, std::vector<Node> &leaves)
{
    int size = internalNodes.size();

    // Allocate memory on GPU
    int *d_randomInts;
    Node *d_internalNodes, *d_leaves;

    cudaMalloc(&d_randomInts, randomInts.size() * sizeof(int));
    cudaMalloc(&d_internalNodes, internalNodes.size() * sizeof(Node));
    cudaMalloc(&d_leaves, leaves.size() * sizeof(Node));

    // Copy data from host to device
    cudaMemcpy(d_randomInts, randomInts.data(), randomInts.size() * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_internalNodes, internalNodes.data(), internalNodes.size() * sizeof(Node), cudaMemcpyHostToDevice);
    cudaMemcpy(d_leaves, leaves.data(), leaves.size() * sizeof(Node), cudaMemcpyHostToDevice);

    // Configure kernel launch
    double threadsPerBlock = 256;
    int blocksPerGrid = ceil(size / threadsPerBlock);

    // Launch kernel
    buildInternalKernel<<<blocksPerGrid, threadsPerBlock>>>(d_randomInts, d_leaves, d_internalNodes, size);
    cudaDeviceSynchronize(); // Wait for GPU execution to complete

    // Copy results back to host
    cudaMemcpy(internalNodes.data(), d_internalNodes, internalNodes.size() * sizeof(Node), cudaMemcpyDeviceToHost);

    // Free GPU memory
    cudaFree(d_randomInts);
    cudaFree(d_internalNodes);
    cudaFree(d_leaves);
}