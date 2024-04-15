#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include "../include/decisionTree.h"
#include "../include/loadDataset.h"
#include "../include/utils.h"
#include "stdio.h"
#include "math.h"
#include "time.h"
#include <omp.h>

#define OFFSET(x, y, m) (((x)*(m)) + (y))

int possible_feature;
int* dataset;

typedef struct {
    int* indices;
    int* indices_tmp;
    int* _featureNum;
    int* _startIndices;
    int* _featureNum_entropy;
    double* entropy_left;
    double* entropy_right;
    double* IGs;
    int* best_split_vals;
    int dataset_cols, dataset_rows;
} TreeData; // Add a semicolon at the end of the struct definition


void init_global(int n_features) {
    int dataset_rows = numberOfTrainingRecord + 1;
    int dataset_cols = attributeNum + 1;
    possible_feature = n_features;
    dataset = malloc(sizeof(int) * dataset_rows * dataset_cols);
    for (int i = 0; i < dataset_rows; i++) {
        for (int j = 0; j < dataset_cols; j++) {
            dataset[OFFSET(i , j, dataset_cols)] = trainingData[i][j];
        }
    }
}

void init(TreeData* data) {
    data->dataset_cols = attributeNum + 1;
    data->dataset_rows = numberOfTrainingRecord + 1;
    data->indices = malloc(sizeof(int) * data->dataset_rows * data->dataset_cols);
    data->indices_tmp = malloc(sizeof(int) * data->dataset_rows * data->dataset_cols);
    data->_featureNum = malloc(sizeof(int) * possible_feature * (attributeNum + 1));
    data->_startIndices = malloc(sizeof(int) * possible_feature * (attributeNum + 1));
    data->_featureNum_entropy = malloc(sizeof(int) * possible_feature * (attributeNum + 1));
    data->IGs = malloc(sizeof(double) * (attributeNum + 1));
    data->entropy_left = malloc(sizeof(double) * (attributeNum + 1));
    data->entropy_right = malloc(sizeof(double) * (attributeNum + 1));
    data->best_split_vals = malloc(sizeof(int) * (attributeNum + 1));
    for (int i = 1; i <= attributeNum; i++) {
        for (int j = 1; j <= numberOfTrainingRecord; j++) {
            data->indices[OFFSET(i, j, data->dataset_rows)] = j;
        }
    }
    memset(data->_featureNum, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    memset(data->_startIndices, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    memset(data->_featureNum_entropy, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    memset(data->IGs, 0, sizeof(double) * (attributeNum + 1));
    memset(data->entropy_left, 0, sizeof(double) * (attributeNum + 1));
    memset(data->entropy_right, 0, sizeof(double) * (attributeNum + 1));
    memset(data->best_split_vals, 0, sizeof(int) * (attributeNum + 1));
}


void private_free(TreeData* data) {
    free(data->indices);
    free(data->indices_tmp);
    free(data->_featureNum);
    free(data->_startIndices);
    free(data->_featureNum_entropy);
    free(data->IGs);
    free(data->entropy_left);
    free(data->entropy_right);
    free(data->best_split_vals);
}


double getEntropy(int left, int right, int mask_left, int mask_right, int feature_index, TreeData* data) {
    for (int i = OFFSET(feature_index, 0, possible_feature); i <= OFFSET(feature_index, possible_feature - 1, possible_feature); i++) {
        data->_featureNum_entropy[i] = 0;
    }
    for (int i = left; i <= right; i++) {
        if (i == mask_left && i <= mask_right) {
            i = mask_right;
            continue;
        }
        int index = data->indices[OFFSET(feature_index, i, data->dataset_rows)];
        int feat_val = dataset[OFFSET(index, 0, data->dataset_cols)];
        data->_featureNum_entropy[OFFSET(feature_index, feat_val, possible_feature)]++;
    }
    double entropy = 0;
    for (int i = 0; i < possible_feature; i++) {
        if (data->_featureNum_entropy[OFFSET(feature_index, i, possible_feature)] > 0) {
            double p = (double)data->_featureNum_entropy[OFFSET(feature_index, i, possible_feature)] / (right - left + 1);
            entropy -= p * log2(p);
        }
    }
    return entropy;
}


// left and right are closed from 1 to numberOfTrainingRecord
int findBestSplit(double entropy, int left, int right, TreeData* data) {
    memset(data->_featureNum, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    memset(data->_startIndices, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    memset(data->IGs, 0, sizeof(double) * attributeNum);
    memset(data->best_split_vals, 0, sizeof(int) * attributeNum);
    int i;
    #pragma omp parallel for
    for (i = 1; i <= attributeNum; i++) {
        int j;
        // update featureNum
        for (j = left; j <= right; j++) {
            int index = data->indices[OFFSET(i, j, data->dataset_rows)];
            int feat_val = dataset[OFFSET(index, i, data->dataset_cols)];
            data->_featureNum[OFFSET(i, feat_val, possible_feature)]++;
        }
        // update start indices
        data->_startIndices[OFFSET(i, 0, possible_feature)] = left;
        for (j = 1; j < possible_feature; j++) {
            data->_startIndices[OFFSET(i, j, possible_feature)] = data->_startIndices[OFFSET(i, j - 1, possible_feature)] + data->_featureNum[OFFSET(i, j - 1, possible_feature)];
        }
        // update indices
        for (j = left; j <= right; j++) {
            int index = data->indices[OFFSET(i, j, data->dataset_rows)];
            int feat_val = dataset[OFFSET(index, i, data->dataset_cols)];
            int pos = data->_startIndices[OFFSET(i, feat_val, possible_feature)];
            data->indices_tmp[OFFSET(i, pos, data->dataset_rows)] = index;
            data->_startIndices[OFFSET(i, feat_val, possible_feature)]++;
        }
        for (j = left; j <= right; j++) {
            data->indices[OFFSET(i, j, data->dataset_rows)] = data->indices_tmp[OFFSET(i, j, data->dataset_rows)];
        }
        double maxIG = 0;
        int best_split_val = 0;
        for (j = 0; j < possible_feature; j++) {
            if (data->_featureNum[OFFSET(i, j, possible_feature)] > 0) {
                int mask_start = data->_startIndices[OFFSET(i, j, possible_feature)] - data->_featureNum[OFFSET(i, j, possible_feature)];
                int mask_end = data->_startIndices[OFFSET(i, j, possible_feature)] - 1;
                double IG = entropy;
                double entropy_unmasked = getEntropy(left, right, mask_start, mask_end, i, data);
                double entropy_masked = getEntropy(mask_start, mask_end, 1, 0, i, data);
                data->entropy_left[i] = entropy_masked;
                data->entropy_right[i] = entropy_unmasked; 
                IG -= (double)(mask_end - mask_start + 1) / (right - left + 1) * entropy_masked;
                IG -= (double)((right - left) - (mask_end - mask_start)) / (right - left + 1) * entropy_unmasked;
                if (IG > maxIG) {
                    maxIG = IG;
                    best_split_val = j;
                }
            }
        }
        data->IGs[i] = maxIG;
        data->best_split_vals[i] = best_split_val;
    }

    int best_IG_index = -1;
    double overall_best_IG = 0;
    for (i = 1; i <= attributeNum; i++) {
        if (data->IGs[i] > overall_best_IG) {
            overall_best_IG = data->IGs[i];
            best_IG_index = i;
        }
    }
    return best_IG_index;
}


int updateIndex(int split_feature_index, int left, int right, TreeData* data) {
    int split_feature_val = data->best_split_vals[split_feature_index];
    int i = left;
    int j;
    for (j = left; j <= right; j++) {
        int index = data->indices[OFFSET(split_feature_index, j, data->dataset_rows)];
        int feat_val = dataset[OFFSET(index, split_feature_index, data->dataset_cols)];
        if (feat_val == split_feature_val) {
            int tmp = data->indices[OFFSET(split_feature_index, i, data->dataset_rows)];
            data->indices[OFFSET(split_feature_index, i, data->dataset_rows)] = data->indices[OFFSET(split_feature_index, j, data->dataset_rows)];
            data->indices[OFFSET(split_feature_index, j, data->dataset_rows)] = tmp;
            i++;
        }
    }
    for (j = 1; j <= attributeNum; j++) {
        for (int k = left; k <= right; k++) {
            data->indices[OFFSET(j, k, data->dataset_rows)] = data->indices[OFFSET(split_feature_index, k, data->dataset_rows)];
        }
    }
    return i;
}


void generateSubTree(int curLevel, int maxLevel, TreeNode tree[], int curPosition, int start, int end, TreeData* data) {
    double curEntropy = tree[curPosition].entropy;
    if (curLevel == maxLevel - 1 || curEntropy == 0) {
        tree[curPosition].isLeaf = 1;
        return;
    }
    int split_feature_index = findBestSplit(curEntropy, start, end, data);
    if (data->IGs[split_feature_index] > 0) {
        tree[curPosition].splitAttribute = split_feature_index;
        tree[curPosition].splitVal = data->best_split_vals[split_feature_index];
        tree[curPosition * 2].isLeaf = 0;
        tree[curPosition * 2].entropy = data->entropy_left[split_feature_index];
        tree[curPosition * 2 + 1].isLeaf = 0;
        tree[curPosition * 2 + 1].entropy = data->entropy_right[split_feature_index];
        int splitPoint = updateIndex(split_feature_index, start, end, data);
        // left child
        generateSubTree(curLevel + 1, maxLevel, tree, curPosition * 2, start, splitPoint - 1, data);
        // right child
        generateSubTree(curLevel + 1, maxLevel, tree, curPosition * 2 + 1, splitPoint, end, data);
    } else {
        tree[curPosition].isLeaf = 1;
    }
}


void printTree(TreeNode tree[]) {
    int hasNextLevel = 1;
    for (int i = 0; i < MAX_DEPTH && hasNextLevel; i++) {
        hasNextLevel = 0;
        printf("Level %d ", i);
        for (int j = (int) pow(2, i); j < (int) pow(2, i + 1); j++) {
            printf("(entropy: %f, split_attribute: %d, split_val: %d) ", tree[j].entropy, tree[j].splitAttribute, tree[i].splitVal);
            if (tree[i].isLeaf == 0) {
                hasNextLevel = 1;
            }
        }
        printf("\n\n\n");
    }
}


typedef struct {
    TreeNode* forest;
    int start;
    int end;
    int tree_num;
} thread_data;


void* thread_generateSubTree(void* arg) {
    thread_data* args = (thread_data*) arg;
    TreeData* data = malloc(sizeof(TreeData));
    init(data);
    for (int i = 0; i < args->tree_num; i++) {
        printf("Generating tree %d\n", i);
        args->forest[OFFSET(i, 1, (int) pow(2, MAX_DEPTH) + 1)].isLeaf = 0;
        args->forest[OFFSET(i, 1, (int) pow(2, MAX_DEPTH) + 1)].entropy = getEntropy(1, numberOfTrainingRecord, 1, 0, 1, data);
        generateSubTree(0, MAX_DEPTH, &(args->forest[OFFSET(i, 0, (int) pow(2, MAX_DEPTH) + 1)]), 1, args->start, args->end, data);
        printf("Tree %d Done\n", i);
    }

    // args->tree[1].isLeaf = 0;
    // args->tree[1].entropy = getEntropy(1, numberOfTrainingRecord, 1, 0, 1, data);
    // generateSubTree(0, MAX_DEPTH, args->tree, 1, args->start, args->end, data);
    private_free(data);
    free(data);
    pthread_exit(NULL);
}



int main(int argc, char* argv[]) {
    double cpu_time_used;
    struct timeval start, end;
    printf("loading dataset...\n");

    gettimeofday(&start, NULL);
    LoadDataset(argc, argv);
    init_global(256);

    printf("allocate memory for tree (Done)\n");
    gettimeofday(&end, NULL);
    cpu_time_used = (end.tv_sec - start.tv_sec) + 
                     ((end.tv_usec - start.tv_usec) / 1000000.0);
    printf("loading dataset done, took %f second\n", cpu_time_used);
    printf("training...\n");
    gettimeofday(&start, NULL);

    pthread_t thread;
    thread_data* thread_data = malloc(sizeof(thread_data));
    int num_trees = number_of_trees;
    TreeNode forest[(int) (pow(2, MAX_DEPTH) + 1) * num_trees];
    thread_data->start = 1;
    thread_data->end = numberOfTrainingRecord;
    thread_data->tree_num = num_trees;
    thread_data->forest = forest;
    pthread_create(&thread, NULL, thread_generateSubTree, (void*) thread_data);
    pthread_join(thread, NULL);
    gettimeofday(&end, NULL);
    cpu_time_used = (end.tv_sec - start.tv_sec) + 
                     ((end.tv_usec - start.tv_usec)/1000000.0);
    printf("training Done, took %f seconds.\n\n", cpu_time_used);
    free(dataset);
    free(trainingData);
    free(testingData);
    free(thread_data);
    return 0;
}



