#ifndef DECISION_TREE_H
#define DECISION_TREE_H

#define MAX_DEPTH 10

typedef struct {
    double entropy;
    int splitAttribute;
    char splitVal;
    char majorityClass;
    char isLeaf;
} TreeNode;


#endif