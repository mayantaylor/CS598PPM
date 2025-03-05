#include <iostream>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <ctime>
#include <assert.h>
#include <queue>

struct Node
{
    bool isLeaf;
    int data;
    Node *left;
    Node *right;
};

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

int computeCPL(std::vector<int> &randomInts, int i, int j)
{
    assert(i >= 0 && i < randomInts.size());

    if (j >= randomInts.size() || j < 0)
        return -1;

    return commonPrefixLength(randomInts[i], randomInts[j]);
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

void buildInternalNodes(std::vector<int> &randomInts, std::vector<Node> &internalNodes, std::vector<Node> &leaves)
{
    for (int i = 0; i < internalNodes.size(); i++)
    {

        // determine range direction
        int d = sign(computeCPL(randomInts, i, i + 1) - computeCPL(randomInts, i, i - 1));

        int minCPL = computeCPL(randomInts, i, i - d);

        // determine range extent
        int ub = i;
        while (computeCPL(randomInts, i, ub + d) > minCPL)
        {
            ub += d;
        }

        int dNode = computeCPL(randomInts, i, ub);

        // find split point
        int s = 0;
        while (computeCPL(randomInts, i, i + (s + 1) * d) > dNode)
        {
            s++;
        }

        int y = i + s * d + std::min(d, 0);

        // assign left and right children
        Node *left = &internalNodes[y];
        Node *right = &internalNodes[y + 1];
        if (std::min(i, ub) == y)
            left = &leaves[y];

        if (std::max(i, ub) == y + 1)
            right = &leaves[y + 1];

        Node internalNode = {false, i, left, right};
        internalNodes[i].data = i;
        internalNodes[i].left = left;
        internalNodes[i].right = right;
    }
}

void printTree(Node *root, int level, int maxLevel)
{
    if (!root)
    {
        return;
    }

    for (int i = 0; i < level; i++)
        std::cout << "   ";
    if (root->isLeaf)
        std::cout << std::bitset<5>(root->data) << " (leaf)" << std::endl;
    else
        std::cout << root->data << std::endl;

    if (level < maxLevel)
    {
        printTree(root->left, level + 1, maxLevel);
        printTree(root->right, level + 1, maxLevel);
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
        Node leaf = {true, num, nullptr, nullptr};
        leaves.push_back(leaf);
    }
}

void testTreeBuild()
{
    std::vector<int> randomInts = {0b00001, 0b00010, 0b00100, 0b00101, 0b10011, 0b11000, 0b11001, 0b11110};
    std::vector<Node> leaves;

    initializeLeaves(randomInts, leaves, 5);

    int nleaves = leaves.size();
    int ninternal = nleaves - 1;

    std::vector<Node> internalNodes;

    for (int i = 0; i < ninternal; i++)
    {
        Node internalNode = {false, i, nullptr, nullptr};
        internalNodes.push_back(internalNode);
    }

    buildInternalNodes(randomInts, internalNodes, leaves);

    assert(internalNodes[0].left->data == 3);
    assert(internalNodes[0].right->data == 4);
    assert(internalNodes[1].left->data == 0b00001);

    // printTree(&internalNodes[0], 0, ninternal);

    std::cout << "testTreeBuild passed!" << std::endl;
}

int main()
{
    int n = 8;     // number of random numbers (leaves)
    int nBits = 5; // number of bits in each random number

    std::srand(std::time(nullptr)); // use current time as seed for random generator
    // std::vector<int> randomInts = {0b00001, 0b00010, 0b00100, 0b00101, 0b10011, 0b11000, 0b11001, 0b11110};
    std::vector<int> randomInts(n);
    std::vector<Node> leaves;

    std::cout << "Testing support functionality..." << std::endl;
    testCommonPrefixLength();
    testSign();
    testTreeBuild();
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
        Node internalNode = {false, i, nullptr, nullptr};
        internalNodes.push_back(internalNode);
    }

    buildInternalNodes(randomInts, internalNodes, leaves);

    printTree(&internalNodes[0], 0, ninternal);

    return 0;
}
