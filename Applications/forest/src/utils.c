#include "../include/utils.h"
#include "../include/loadDataset.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

extern char* trainingSetFile;
extern char* testingSetFile;
extern int attributeNum;
extern int classNum;
extern int numberOfTrainingRecord;
extern int numberOfTestingRecord;
extern int isPrintResult;

extern AttributeMap* map;
extern char ***rawData;
extern unsigned char **trainingData;
extern unsigned char **testingData;

int ConvertString2Number(char* str) {
    int result = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-') {
        sign = -1;
        i++;
    } else if (str[0] == '+') {
        i++;
    }

    while (str[i] != '\0') {
        if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10 + (str[i] - '0');
            i++;
        } else {
            break; 
        }
    }
    return sign * result;
}

void ConvertNum2Str(uint8_t num, char str[10]) {
    snprintf(str, 10, "%d", num);
}


int IsNumber(char* str)
{
    int ret = 1;

    int i = 0;
    for (i = 0; str[i] != '\0'; ++i)
    {
        if (str[i] == '.')
        {
            continue;
        }

        if (str[i] < '0' || str[i] > '9')
        {
            return 0;
        }
    }
    return ret;
}

int NumberCmp(const void *a, const void *b) {
    return *(int*)a - *(int*)b;
}

void printData(const uint8_t **dataset, int row, int col) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            char str[10];
            ConvertNum2Str(dataset[i][j], str);
            printf("%s ", str);
        }
        printf("\n");
    }
}

double Log2(double n) {
    return log(n) / log(2);
}

void MySwap(int arr[], int left, int right) {
    int tmp = arr[left];
    arr[left] = arr[right];
    arr[right] = tmp;
}