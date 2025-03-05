This MP includes implementations of a parallel binary radix tree construction. At a high level, the algorithm consists of:

- given a list of sorted numbers (represent keys eg. from a Morton ordering)
- build a binary radix tree
- each internal node can compute its children separately

The algorithm follows the outline given in Ted Karras' publication "Maximizing Parallelism in the Construction of BVHs, Octrees, and k-d Trees" from 2012.

My approximation of this algorithm uses:

- unique keys only
- limited number of bits per key
