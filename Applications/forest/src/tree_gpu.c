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
#include <openacc.h>

#define OFFSET(x, y, m) (((x)*(m)) + (y))

int possible_feature;
int* dataset;
int dataset_cols;
int dataset_rows;

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
} TreeData; // Add a semicolon at the end of the struct definition


void init(int n_features, TreeData* data) {
    dataset_rows = numberOfTrainingRecord + 1;
    dataset_cols = attributeNum + 1;
    possible_feature = n_features;
    dataset = malloc(sizeof(int) * dataset_rows * dataset_cols);
    for (int i = 0; i < dataset_rows; i++) {
        for (int j = 0; j < dataset_cols; j++) {
            dataset[OFFSET(i , j, dataset_cols)] = trainingData[i][j];
        }
    }
    int* indices = malloc(sizeof(int) * dataset_rows * dataset_cols);
    int* indices_tmp = malloc(sizeof(int) * dataset_rows * dataset_cols);
    int* _featureNum = malloc(sizeof(int) * possible_feature * (attributeNum + 1));
    int* _startIndices = malloc(sizeof(int) * possible_feature * (attributeNum + 1));
    int* _featureNum_entropy = malloc(sizeof(int) * possible_feature * (attributeNum + 1));
    for (int i = 1; i <= attributeNum; i++) {
        for (int j = 1; j <= numberOfTrainingRecord; j++) {
            indices[OFFSET(i, j, dataset_rows)] = j;
        }
    }
    memset(_featureNum, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    memset(_startIndices, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    memset(_featureNum_entropy, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    #pragma acc enter data copyin(dataset_cols, dataset_rows, possible_feature) 
    #pragma acc enter data copyin(dataset[0:dataset_rows * dataset_cols]) 
    #pragma acc enter data copyin(indices[0:dataset_rows * dataset_cols]) 
    #pragma acc enter data copyin(indices_tmp[0:dataset_rows * dataset_cols])
    #pragma acc enter data copyin(_featureNum[0:possible_feature * (attributeNum + 1)])
    #pragma acc enter data copyin(_startIndices[0:possible_feature * (attributeNum + 1)])
    #pragma acc enter data copyin(_featureNum_entropy[0:possible_feature * (attributeNum + 1)])
    #pragma acc enter data copyin(attributeNum)
    data->indices = indices;
    data->indices_tmp = indices_tmp;
    data->_featureNum = _featureNum;
    data->_startIndices = _startIndices;
    data->_featureNum_entropy = _featureNum_entropy;
}


void private_free_cpu(TreeData* data) {
    int* indices = data->indices;
    int* indices_tmp = data->indices_tmp;
    int* _featureNum = data->_featureNum;
    int* _startIndices = data->_startIndices;
    int* _featureNum_entropy = data->_featureNum_entropy;
    free(indices);
    free(indices_tmp);
    free(_featureNum);
    free(_startIndices);
    free(_featureNum_entropy);
}



double getEntropy(int left, int right, int mask_left, int mask_right, int feature_index, int* _featureNum_entropy, int* indices) {
    for (int i = OFFSET(feature_index, 0, possible_feature); i <= OFFSET(feature_index, possible_feature - 1, possible_feature); i++) {
        _featureNum_entropy[i] = 0;
    }
    for (int i = left; i <= right; i++) {
        if (i == mask_left && i <= mask_right) {
            i = mask_right;
            continue;
        }
        int index = indices[OFFSET(feature_index, i, dataset_rows)];
        int feat_val = dataset[OFFSET(index, 0, dataset_cols)];
        _featureNum_entropy[OFFSET(feature_index, feat_val, possible_feature)]++;
    }
    double entropy = 0;
    for (int i = 0; i < possible_feature; i++) {
        if (_featureNum_entropy[OFFSET(feature_index, i, possible_feature)] > 0) {
            double p = (double)_featureNum_entropy[OFFSET(feature_index, i, possible_feature)] / (right - left + 1);
            entropy -= p * log2(p);
        }
    }
    return entropy;
}



int findBestSplit(double entropy, int left, int right, TreeData* data) {
    int* indices = data->indices;
    int* indices_tmp = data->indices_tmp;
    int* _featureNum = data->_featureNum;
    int* _startIndices = data->_startIndices;
    int* _featureNum_entropy = data->_featureNum_entropy;
    memset(_featureNum, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    memset(_startIndices, 0, sizeof(int) * possible_feature * (attributeNum + 1));
    int i;
    double left_entropy_cp[attributeNum + 1];
    double right_entropy_cp[attributeNum + 1];
    double IGs_cp[attributeNum + 1];
    double best_split_vals_cp[attributeNum + 1];
    #pragma acc parallel loop present(dataset[0:dataset_rows * dataset_cols], indices[0:dataset_rows * dataset_cols], _featureNum[0:possible_feature * (attributeNum + 1)], _startIndices[0:possible_feature * (attributeNum + 1)], indices_tmp[0:dataset_rows * dataset_cols], _featureNum_entropy[0:possible_feature * (attributeNum + 1)])
    for (i = 1; i <= attributeNum; i++) {
        int j;
        // update featureNum
        for (j = left; j <= right; j++) {
            int index = indices[OFFSET(i, j, dataset_rows)];
            int feat_val = dataset[OFFSET(index, i, dataset_cols)];
            _featureNum[OFFSET(i, feat_val, possible_feature)]++;
        }
        // update start indices
        _startIndices[OFFSET(i, 0, possible_feature)] = left;
        for (j = 1; j < possible_feature; j++) {
            _startIndices[OFFSET(i, j, possible_feature)] = _startIndices[OFFSET(i, j - 1, possible_feature)] + _featureNum[OFFSET(i, j - 1, possible_feature)];
        }
        // update indices
        for (j = left; j <= right; j++) {
            int index = indices[OFFSET(i, j, dataset_rows)];
            int feat_val = dataset[OFFSET(index, i, dataset_cols)];
            int pos = _startIndices[OFFSET(i, feat_val, possible_feature)];
            indices_tmp[OFFSET(i, pos, dataset_rows)] = index;
            _startIndices[OFFSET(i, feat_val, possible_feature)]++;
        }
        for (j = left; j <= right; j++) {
            indices[OFFSET(i, j, dataset_rows)] = indices_tmp[OFFSET(i, j, dataset_rows)];
        }
        double maxIG = 0;
        int best_split_val = 0;
        for (j = 0; j < possible_feature; j++) {
            if (_featureNum[OFFSET(i, j, possible_feature)] > 0) {
                int mask_start = _startIndices[OFFSET(i, j, possible_feature)] - _featureNum[OFFSET(i, j, possible_feature)];
                int mask_end = _startIndices[OFFSET(i, j, possible_feature)] - 1;
                double IG = entropy;
                // get_entropy(left, right, mask_left, mask_right, feature_index, _featureNum_entropy, indices)
                // double entropy_unmasked = get_entropy(left, right, mask_start, mask_end, i, _featureNum_entropy, indices);
                for (int k = OFFSET(i, 0, possible_feature); k <= OFFSET(i, possible_feature - 1, possible_feature); k++) {
                    _featureNum_entropy[k] = 0;
                }
                for (int k = left; k <= right; k++) {
                    if (k == mask_start && k <= mask_end) {
                        k = mask_end;
                        continue;
                    }
                    int index = indices[OFFSET(i, k, dataset_rows)];
                    int feat_val = dataset[OFFSET(index, 0, dataset_cols)];
                    _featureNum_entropy[OFFSET(i, feat_val, possible_feature)]++;
                }
                double entropy_unmasked = 0;
                for (int k = 0; k < possible_feature; k++) {
                    if (_featureNum_entropy[OFFSET(i, k, possible_feature)] > 0) {
                        double p = (double)_featureNum_entropy[OFFSET(i, k, possible_feature)] / (right - left + 1);
                        entropy_unmasked -= p * log2(p);
                    }
                }
                // get_entropy(left, right, mask_left, mask_right, feature_index, _featureNum_entropy, indices)
                // double entropy_masked = get_entropy(mask_start, mask_end, 1, 0, i, _featureNum_entropy, indices);
                for (int k = OFFSET(i, 0, possible_feature); k <= OFFSET(i, possible_feature - 1, possible_feature); k++) {
                    _featureNum_entropy[k] = 0;
                }
                for (int k = left; k <= right; k++) {
                    int index = indices[OFFSET(i, k, dataset_rows)];
                    int feat_val = dataset[OFFSET(index, 0, dataset_cols)];
                    _featureNum_entropy[OFFSET(i, feat_val, possible_feature)]++;
                }
                double entropy_masked = 0;
                for (int k = 0; k < possible_feature; k++) {
                    if (_featureNum_entropy[OFFSET(i, k, possible_feature)] > 0) {
                        double p = (double)_featureNum_entropy[OFFSET(i, k, possible_feature)] / (right - left + 1);
                        entropy_masked -= p * log2(p);
                    }
                }
                IG -= (double)(mask_end - mask_start + 1) / (right - left + 1) * entropy_masked;
                IG -= (double)((right - left) - (mask_end - mask_start)) / (right - left + 1) * entropy_unmasked;
                if (IG > maxIG) {
                    maxIG = IG;
                    best_split_val = j;
                    left_entropy_cp[i] = entropy_masked;
                    right_entropy_cp[i] = entropy_unmasked;
                }
            }
        }
        IGs_cp[i] = maxIG;
        best_split_vals_cp[i] = best_split_val;
    }

    data->entropy_left = left_entropy_cp;
    data->entropy_right = right_entropy_cp;
    data->IGs = IGs_cp;
    data->best_split_vals = best_split_vals_cp;
    

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
        int index = data->indices[OFFSET(split_feature_index, j, dataset_rows)];
        int feat_val = dataset[OFFSET(index, split_feature_index, dataset_cols)];
        if (feat_val == split_feature_val) {
            int tmp = data->indices[OFFSET(split_feature_index, i, dataset_rows)];
            data->indices[OFFSET(split_feature_index, i, dataset_rows)] = data->indices[OFFSET(split_feature_index, j, dataset_rows)];
            data->indices[OFFSET(split_feature_index, j, dataset_rows)] = tmp;
            i++;
        }
    }
    for (j = 1; j <= attributeNum; j++) {
        for (int k = left; k <= right; k++) {
            data->indices[OFFSET(j, k, dataset_rows)] = data->indices[OFFSET(split_feature_index, k, dataset_rows)];
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
    int start;
    int end;
    TreeNode* forest;
    int tree_num;
} ThreadData;


void *thread_generateSubTree(void *arg) {
    ThreadData *args = (ThreadData *)arg;
    TreeData* data = malloc(sizeof(TreeData));
    init(256, data);
    printf("start: %d, end: %d\n", args->start, args->end);
    printf("tree num: %d\n", args->tree_num);
    for (int i = 0; i < args->tree_num; i++) {
        printf("tree %d\n", i);
        args->forest[OFFSET(i, 1, (int) pow(2, MAX_DEPTH) + 1)].isLeaf = 0;
        args->forest[OFFSET(i, 1, (int) pow(2, MAX_DEPTH) + 1)].entropy = getEntropy(1, numberOfTrainingRecord, 1, 0, 1, data->_featureNum_entropy, data->indices);
        generateSubTree(0, MAX_DEPTH, &(args->forest[OFFSET(i, 0, (int) pow(2, MAX_DEPTH) + 1)]), 1, args->start, args->end, data);
        printf("tree %d done\n", i);
    }

    private_free_cpu(data);
    free(data);
    pthread_exit(NULL);
}



int main(int argc, char* argv[]) {
    acc_set_device_num(1, acc_device_nvidia);
    double cpu_time_used;
    struct timeval start, end;
    printf("loading dataset...\n");

    gettimeofday(&start, NULL);
    LoadDataset(argc, argv);

    printf("allocate memory for tree (Done)\n");
    gettimeofday(&end, NULL);
    cpu_time_used = (end.tv_sec - start.tv_sec) + 
                     ((end.tv_usec - start.tv_usec) / 1000000.0);
    printf("loading dataset done, took %f second\n", cpu_time_used);
    printf("training...\n");
    gettimeofday(&start, NULL);

    pthread_t thread;
    ThreadData* thread_data = malloc(sizeof(ThreadData));
    int tree_num = number_of_trees;
    thread_data->start = 1;
    thread_data->end = numberOfTrainingRecord;
    thread_data->tree_num = tree_num;
    TreeNode* forest = (TreeNode*)malloc(sizeof(TreeNode) * tree_num * (int)(pow(2, MAX_DEPTH) + 1));
    thread_data->forest = forest;
    
    pthread_create(&thread, NULL, thread_generateSubTree, (void*) thread_data);

    pthread_join(thread, NULL);
    gettimeofday(&end, NULL);
    cpu_time_used = (end.tv_sec - start.tv_sec) + 
                     ((end.tv_usec - start.tv_usec)/1000000.0);
    printf("training Done, took %f seconds.\n\n", cpu_time_used);
    #pragma acc exit data delete(dataset[0:dataset_rows * dataset_cols])
    free(dataset);
    free(trainingData);
    free(testingData);
    free(forest);
    free(thread_data);
    return 0;
}



