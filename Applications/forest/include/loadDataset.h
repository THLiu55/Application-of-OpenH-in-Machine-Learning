//
// Created by Tianhao Liu on 2023/11/3.
//

#ifndef PREPROCESS_H
#define PREPROCESS_H

// max length of attribute value
#define MAX_LEN 100

// map attribute(class) to number
typedef struct
{
    char attributeName[MAX_LEN];
    char **attributes;
    int attributeNum;  // number of values in list attributes
    int isConsecutive; // 1 => yes, 0 => no
    int *attributeValue;
}AttributeMap;

// meta-data of dataset
extern char* trainingSetFile;
extern char* testingSetFile;
extern int attributeNum;
extern int classNum;
extern int numberOfTrainingRecord;
extern int numberOfTestingRecord;
extern int isPrintResult;
extern int number_of_trees;

// content of dataset
extern AttributeMap* map;
extern char ***rawData;
extern unsigned char **trainingData;
extern unsigned char **testingData;

// functions
void LoadDataset(int argc, char* argv[]);

#endif
