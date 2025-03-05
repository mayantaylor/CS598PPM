#include <iostream>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <assert.h>
#include <queue>
#include <bitset>

#include "cuda_runtime.h"

struct Node
{
    bool isLeaf;
    int data;

    int left;
    bool leftIsLeaf;

    int right;
    bool rightIsLeaf;
};

void buildInternalNodes(std::vector<int> &randomInts, std::vector<Node> &internalNodes, std::vector<Node> &leaves);

int sign(int x)
{
    return (x >= 0) - (x < 0);
}

// assume x and y have same bitlength and start with 1
int commonPrefixLength(int x, int y)
{
    int xLength = std::bitset<5>(x).to_string().length();
    int yLength = std::bitset<5>(y).to_string().length();

    int maxLen = std::max(xLength, yLength);

    int cpl = 0;
    while (x != y)
    {

        x >>= 1;
        y >>= 1;
        cpl++;
    }

    return maxLen - cpl;
}

void testCommonPrefixLength()
{
    assert(commonPrefixLength(0b11100, 0b11110) == 3);
    assert(commonPrefixLength(0b11010, 0b11011) == 4);
    assert(commonPrefixLength(0b11000, 0b11000) == 5);
    assert(commonPrefixLength(0b10000, 0b11000) == 1);
    std::cout << "commonPrefixLength tests passed!" << std::endl;
}
void testSign()
{
    assert(sign(0) == 1);
    assert(sign(1) == 1);
    assert(sign(-1) == -1);
    assert(sign(100) == 1);
    assert(sign(-100) == -1);
    std::cout << "sign tests passed!" << std::endl;
}

void printTree(std::vector<Node> &internalNodes)
{
    std::queue<int> q;
    q.push(0);

    while (!q.empty())
    {
        int nodeIndex = q.front();
        q.pop();

        Node &node = internalNodes[nodeIndex];

        std::cout << "Node " << nodeIndex << ": ";
        std::cout << "data = " << node.data << ", ";

        if (node.leftIsLeaf)
        {
            std::cout << "left = Leaf(" << node.left << "), ";
        }
        else
        {
            std::cout << "left = Internal(" << node.left << "), ";
            q.push(node.left);
        }

        if (node.rightIsLeaf)
        {
            std::cout << "right = Leaf(" << node.right << ")";
        }
        else
        {
            std::cout << "right = Internal(" << node.right << ")";
            q.push(node.right);
        }

        std::cout << std::endl;
    }
}

void initializeRandints(std::vector<int> &randomInts, int nBits)
{
    for (int &num : randomInts)
    {
        num = std::rand() % (2 ^ nBits); // generate random number between 0 and 99
    }

    std::sort(randomInts.begin(), randomInts.end());
    randomInts.erase(std::unique(randomInts.begin(), randomInts.end()), randomInts.end());

    printf("Sorted random numbers (in binary): \n");
    for (const int &num : randomInts)
    {
        std::cout << std::bitset<5>(num) << " ";
    }
    std::cout << std::endl;
}

void initializeLeaves(std::vector<int> &randomInts, std::vector<Node> &leaves, int nBits)
{

    for (const int &num : randomInts)
    {
        Node leaf = {true, num, -1, NULL, -1, NULL};
        leaves.push_back(leaf);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: ./tree <number of random numbers>" << std::endl;
        return 1;
    }

    int n = atoi(argv[1]); // number of random numbers (leaves)

    if (n < 2)
    {
        std::cerr << "Number of random numbers must be greater than 1" << std::endl;
        return 1;
    }

    int nBits = 5; // number of bits in each random number

    std::srand(std::time(nullptr)); // use current time as seed for random generator
    // std::vector<int> randomInts = {0b00001, 0b00010, 0b00100, 0b00101, 0b10011, 0b11000, 0b11001, 0b11110};
    std::vector<int> randomInts(n);
    std::vector<Node> leaves;

    std::cout << "Testing support functionality..." << std::endl;
    testCommonPrefixLength();
    testSign();
    std::cout << "---------------" << std::endl;

    std::cout << "Generating random numbers with " << nBits << " bits" << std::endl;
    initializeRandints(randomInts, nBits);
    initializeLeaves(randomInts, leaves, nBits);

    std::cout << "---------------" << std::endl;

    int nleaves = leaves.size();
    int ninternal = nleaves - 1;
    std::vector<Node> internalNodes;

    for (int i = 0; i < ninternal; i++)
    {
        Node internalNode = {false, i, -1, NULL, -1, NULL};
        internalNodes.push_back(internalNode);
    }

    buildInternalNodes(randomInts, internalNodes, leaves);

    printTree(internalNodes);

    return 0;
}
