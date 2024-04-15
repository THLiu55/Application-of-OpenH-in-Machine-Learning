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

#include <openh.h>
#include <openherr.h>

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
    int dataset_cols, dataset_rows;
} TreeData; // Add a semicolon at the end of the struct definition

void init_gpu(int n_features, TreeData* data) {
    printf("init gpu\n");
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
    printf("free gpu\n");
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

double getEntropy_gpu(int left, int right, int mask_left, int mask_right, int feature_index, int* _featureNum_entropy, int* indices) {
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



int findBestSplit_gpu(double entropy, int left, int right, TreeData* data) {
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
    int best_split_vals_cp[attributeNum + 1];
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


int updateIndex_gpu(int split_feature_index, int left, int right, TreeData* data) {
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


void generateSubTree_gpu(int curLevel, int maxLevel, TreeNode tree[], int curPosition, int start, int end, TreeData* data) {
    double curEntropy = tree[curPosition].entropy;
    if (curLevel == maxLevel - 1 || curEntropy == 0) {
        tree[curPosition].isLeaf = 1;
        return;
    }
    int split_feature_index = findBestSplit_gpu(curEntropy, start, end, data);
    if (data->IGs[split_feature_index] > 0) {
        tree[curPosition].splitAttribute = split_feature_index;
        tree[curPosition].splitVal = data->best_split_vals[split_feature_index];
        tree[curPosition * 2].isLeaf = 0;
        tree[curPosition * 2].entropy = data->entropy_left[split_feature_index];
        tree[curPosition * 2 + 1].isLeaf = 0;
        tree[curPosition * 2 + 1].entropy = data->entropy_right[split_feature_index];
        int splitPoint = updateIndex_gpu(split_feature_index, start, end, data);
        // left child
        generateSubTree_gpu(curLevel + 1, maxLevel, tree, curPosition * 2, start, splitPoint - 1, data);
        // right child
        generateSubTree_gpu(curLevel + 1, maxLevel, tree, curPosition * 2 + 1, splitPoint, end, data);
    } else {
        tree[curPosition].isLeaf = 1;
    }
}


typedef struct {
    int component_id;
    int start;
    int end;
    TreeNode* forest;
    int tree_num;
} ThreadData;


void *thread_generateSubTree_gpu(void *arg) {
    ThreadData *args = (ThreadData *)arg;
    int gpuComponentId = args->component_id;
    acc_set_device_num(gpuComponentId, acc_device_nvidia);
    openh_bind_acc_self(gpuComponentId);
    TreeData* data = malloc(sizeof(TreeData));
    init_gpu(256, data);
    printf("start: %d, end: %d\n", args->start, args->end);
    printf("tree num: %d\n", args->tree_num);
    for (int i = 0; i < args->tree_num; i++) {
        printf("tree %d\n", i);
        args->forest[OFFSET(i, 1, (int) pow(2, MAX_DEPTH) + 1)].isLeaf = 0;
        args->forest[OFFSET(i, 1, (int) pow(2, MAX_DEPTH) + 1)].entropy = getEntropy_gpu(1, numberOfTrainingRecord, 1, 0, 1, data->_featureNum_entropy, data->indices);
        generateSubTree_gpu(0, MAX_DEPTH, &(args->forest[OFFSET(i, 0, (int) pow(2, MAX_DEPTH) + 1)]), 1, args->start, args->end, data);
        printf("tree %d done\n", i);
    }

    private_free_cpu(data);
    free(data);
    pthread_exit(NULL);
}


void init_global(int n_features) {
    dataset_rows = numberOfTrainingRecord + 1;
    dataset_cols = attributeNum + 1;
    possible_feature = n_features;
    dataset = malloc(sizeof(int) * dataset_rows * dataset_cols);
    for (int i = 0; i < dataset_rows; i++) {
        for (int j = 0; j < dataset_cols; j++) {
            dataset[OFFSET(i , j, dataset_cols)] = trainingData[i][j];
        }
    }
}

void init_cpu(TreeData* data) {
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


void* thread_generateSubTree_cpu(void* arg) {
    ThreadData* args = (ThreadData*) arg;
    int cpuComponentId = args->component_id;
    openh_bind_cpu_self(cpuComponentId);
    TreeData* data = malloc(sizeof(TreeData));
    init_cpu(data);
    for (int i = 0; i < args->tree_num; i++) {
        printf("Generating tree %d from cpu\n", i);
        args->forest[OFFSET(i, 1, (int) pow(2, MAX_DEPTH) + 1)].isLeaf = 0;
        args->forest[OFFSET(i, 1, (int) pow(2, MAX_DEPTH) + 1)].entropy = getEntropy(1, numberOfTrainingRecord, 1, 0, 1, data);
        generateSubTree(0, MAX_DEPTH, &(args->forest[OFFSET(i, 0, (int) pow(2, MAX_DEPTH) + 1)]), 1, args->start, args->end, data);
        printf("Tree %d generated by cpu\n", i);
    }
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
    
    openh_init();
    int cpuid = 0;
    openh_assign_acc_pcpuids(0, &cpuid, 1);

    openh_assign_cpu_free_pcpuids(0);

    openh_print();

    
    ThreadData* gpuData = malloc(sizeof(ThreadData));

    gpuData->component_id = 0;
    gpuData->start = 1;
    gpuData->end = numberOfTrainingRecord;
    gpuData->tree_num = 2;
    gpuData->forest = malloc(sizeof(TreeNode) * 2 * (int) pow(2, MAX_DEPTH) + 1);

    ThreadData* cpuData = malloc(sizeof(ThreadData));

    cpuData->component_id = 0;
    cpuData->start = 1;
    cpuData->end = numberOfTrainingRecord;
    cpuData->forest = malloc(sizeof(TreeNode) * 8 * (int) pow(2, MAX_DEPTH) + 1);
    cpuData->tree_num = 8;

    printf("allocate memory for tree (Done)\n");
    gettimeofday(&end, NULL);
    cpu_time_used = (end.tv_sec - start.tv_sec) + 
                     ((end.tv_usec - start.tv_usec) / 1000000.0);
    printf("loading dataset done, took %f second\n", cpu_time_used);    
    gettimeofday(&start, NULL);
    printf("start generating tree...\n");

    pthread_t cpuThread;
    pthread_t gpuThread;

    
    pthread_create(&cpuThread, NULL, thread_generateSubTree_cpu, (void*) cpuData);
    pthread_create(&gpuThread, NULL, thread_generateSubTree_gpu, (void*) gpuData);

    pthread_join(cpuThread, NULL);
    pthread_join(gpuThread, NULL);

    gettimeofday(&end, NULL);
    cpu_time_used = (end.tv_sec - start.tv_sec) + 
                     ((end.tv_usec - start.tv_usec) / 1000000.0);
    printf("generating tree done, took %f second\n", cpu_time_used);
    
    free(dataset);
    free(trainingData);
    free(testingData);
    return 0;
}
