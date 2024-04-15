#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../include/utils.h"
#include "../include/loadDataset.h"


/**

Read in parameters

./train -r trainning.txt -t testing.txt -d 3 -c 2 -s 499 -m 200 -p 0
-r <trainging dataset file name>
-t <testing dataset file>
-d <number of attribute constructing a record>
-c <number of classes>
-s <Number of training records>
-m <Number of testing records>
-p <print the prediction or not>

**/

char* trainingSetFile;
char* testingSetFile;
int attributeNum;
int classNum;
int numberOfTrainingRecord;
int numberOfTestingRecord;
int isPrintResult;
int number_of_trees;

AttributeMap* map;
char ***rawData;
unsigned char **trainingData;
unsigned char **testingData;

void Read(int argc, char* argv[])
{
    int k;
    for (k = 1; k < argc; ++k)
    {
        if (strcmp(argv[k],"-r") == 0)
        {
            ++k;
            trainingSetFile = argv[k];
        }
        else if (!strcmp(argv[k], "-t"))
        {
            ++k;
            testingSetFile = argv[k];
        }
        else if (!strcmp(argv[k], "-d"))
        {
            ++k;
            attributeNum = ConvertString2Number(argv[k]);
        }
        else if (!strcmp(argv[k], "-c"))
        {
            ++k;
            classNum = ConvertString2Number(argv[k]);
        }
        else if (!strcmp(argv[k], "-s"))
        {
            ++k;
            numberOfTrainingRecord = ConvertString2Number(argv[k]);
        }
        else if (!strcmp(argv[k], "-m"))
        {
            ++k;
            numberOfTestingRecord = ConvertString2Number(argv[k]);
        }
        else if (!strcmp(argv[k], "-p"))
        {
            ++k;
            isPrintResult = ConvertString2Number(argv[k]);
        }
        else if (!strcmp(argv[k], "-n"))
        {
            ++k;
            number_of_trees = ConvertString2Number(argv[k]);
        }
        else{
            printf("input error, please check you input\n");
        }
    }
}


/**

Initiate memory for dataset

**/
void MallocMemory() {
    int i, j;
    // raw data
    int numberOfRecord = numberOfTrainingRecord > numberOfTestingRecord ? numberOfTrainingRecord : numberOfTestingRecord;
    rawData = (char***) malloc(numberOfRecord * sizeof(char**));
    if (!rawData) {
        fprintf(stderr, "malloc memory failed\n");
        abort();
    }
    for (i = 0; i <= numberOfRecord; i++) {
        rawData[i] = (char**) malloc(sizeof(char*) * (attributeNum + 1));
        if (!rawData[i]) {
            fprintf(stderr, "malloc memory failed\n");
            abort();
        }

        for (j = 0; j <= attributeNum; j++) {
            rawData[i][j] = (char*) malloc(MAX_LEN);
            if (!rawData[i][j]) {
                fprintf(stderr, "malloc memory failed\n");
                abort();
            }
        }
    }

    // malloc training data
    trainingData = (uint8_t **)malloc(sizeof(uint8_t*) * (numberOfTrainingRecord + 1));
    if (!trainingData) {
        fprintf(stderr, "malloc memory failed\n");
        abort();
    }
    for (i = 0; i <= numberOfTrainingRecord; i++) {
        trainingData[i] = (uint8_t *) malloc(sizeof(uint8_t) * (attributeNum + 1));
        if (!trainingData[i]) {
            fprintf(stderr, "malloc memory failed\n");
            abort();
        }
    }

    // malloc test data
    testingData = (uint8_t **) malloc(sizeof(uint8_t *) * (numberOfTestingRecord + 1));
    if (!testingData) {
        fprintf(stderr, "malloc memory failed\n");
        abort();
    }
    for (i = 0; i <= numberOfTestingRecord; i++) {
        testingData[i] = (uint8_t *) malloc(sizeof(uint8_t) * (attributeNum + 1));
        if (!testingData) {
            fprintf(stderr, "malloc memory failed\n");
            abort();
        }
    }

    // malloc map
    map = (AttributeMap*) malloc(sizeof(AttributeMap) * (attributeNum + 1));
    if (!map) {
        fprintf(stderr, "melloc memory faild\n");
        abort();
    }
}


/*

Initiate map name (new colume name)
label: classLab
feature: attribute1, attribute2 ...

*/
void InitMapName() {
    int i;
    char str[10];
    strcpy(map[0].attributeName, "classLab");
    for (i = 1; i <= attributeNum; i++) {
        strcpy(map[i].attributeName, "attribute");
        ConvertNum2Str(i, str);
        strcat(map[i].attributeName, str);
    }
}


/*

Create a map between original feature and numeric feauture (converted)

*/
void ConstructMap()
{
    struct str_list {
        char str[MAX_LEN];
        struct str_list *next;
    } *list, *tail, *ptr;

    list = tail = ptr = NULL;
    int totalNum;
    int i, j, k, num = 0;
    totalNum = numberOfTrainingRecord;
    /* analyse input data(raw data) */
    for (j = 0; j <= attributeNum; j++) {//0 is the classLab
        for (i = 1; i <= totalNum; i++) {
            ptr = list;
            while (ptr) {
                if (strncmp(ptr->str, rawData[i][j], MAX_LEN - 1) == 0) {
                    break;
                }

                ptr = ptr->next;
            }

            if (!ptr) {
                ptr = (struct str_list *)malloc(sizeof(struct str_list));
                if (!ptr) {
                    fprintf(stderr, "malloc memory failed, abort.\n");
                    abort();
                }

                strncpy(ptr->str, rawData[i][j], MAX_LEN - 1);
                ptr->next = NULL;
                if (list) {
                    tail->next = ptr;
                    tail = tail->next;
                }
                else {
                    list = tail = ptr;
                }

                num++;
            }
        }

        if (num == 0) {
            fprintf(stderr, "impossible.\n");
            exit(-1);
        }

        /* assign the list to attribute_map */
        map[j].attributeNum = num;
        map[j].isConsecutive = 0;
        map[j].attributes = (char **)malloc(sizeof(char *) * num);
        if (!map[j].attributes) {
            fprintf(stderr, "malloc memory failed, abort.\n");
            abort();
        }

        ptr = list;
        k = 0;
        while (k < num && ptr) {
            map[j].attributes[k] = (char *)malloc(MAX_LEN);
            if (!map[j].attributes[k]) {
                fprintf(stderr, "malloc memory failed, abort.\n");
                abort();
            }

            strncpy(map[j].attributes[k], ptr->str, MAX_LEN - 1);
            k++;
            ptr = ptr->next;
        }

        if (j > 0)
        {
            if (map[j].attributeNum > 20 && IsNumber(map[j].attributes[0]) == 1) //Consecutive
            {
                map[j].isConsecutive = 1;
                int a;
                map[j].attributeValue = (int*) malloc(sizeof(int)*num);
                for (a = 0; a < num; ++a)
                {
                    map[j].attributeValue[a] = ConvertString2Number(map[j].attributes[a]);
                }
                //TestMap();
                qsort(map[j].attributeValue, map[j].attributeNum, sizeof(map[j].attributeValue[0]),NumberCmp);
                //TestMap();
            }
        }

        /* free the list */
        while ((ptr = list)) {
            list = list->next;
            free(ptr);
        }
        list = tail = ptr = NULL;

        num = 0;
    }

}


/*

map attribute to a range from 0 to map[i].attributesNum - 1

*/
int MapAttribute2Num(int i, char* str) {
    if (i > attributeNum) {
        return -1;
    }

    int count = 0;
    for (; count < map[i].attributeNum; count++) {
        if (strncmp(map[i].attributes[count], str, MAX_LEN - 1) == 0) {
            return count;
        }
    }

    return -1;
}

/*

convert raw data to training/testing data
consecutive: string -> number
not consecutive: string -> mapped index

*/
void ConvertRawData2Map(int flag)
{
    int i, j;
    /* assign the training data and test data */
    for (j = 0; j <= attributeNum; j++) {
        i = 1;
        if (flag == 1) {
            while (i <= numberOfTrainingRecord) {
                if (map[j].isConsecutive == 1) {
                    trainingData[i][j] = ConvertString2Number(rawData[i][j]);
                } else {
                    trainingData[i][j] = MapAttribute2Num(j, rawData[i][j]);
                }
                i++;
            }
        }
        else if (flag == 2) {
            while (i <= numberOfTestingRecord) {
                if (map[j].isConsecutive == 1) {
                    testingData[i][j] = ConvertString2Number(rawData[i][j]);
                } else {
                    testingData[i][j] = MapAttribute2Num(j, rawData[i][j]);
                }
                i++;
            }
        }
    }
}


/*

Read data from txt file to rawData, create attribute map, transform rawData to train/test dataset
FLAG: 1 -> training dataset, 2 -> testing dataset
*/
void OnReadData(char* filename, int flag) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "can't open file '%s', exit\n", filename);
        exit(-1);
    }
    int i, j;
    int n = 4096;
    char buffer[n];
    char *begin = NULL;
    char *end = NULL;
    int totalnum;
    if (flag == 1) {
        totalnum = numberOfTrainingRecord;
    } else {
        totalnum = numberOfTestingRecord;
    }
    // read all the data
    i = 1;
    while (i <= totalnum) {
        char* ptr = fgets(buffer, n, fp);
        if (!ptr) {
            fprintf(stderr, "please add the attribute name\n");
            exit(-1);
        }
        begin = buffer;
        // skip the first value (ID)
        end = strchr(begin, (int)('\t'));
        if (end) {
            begin = end + 1;  
        } else {
            fprintf(stderr, "line tab wasn't found\n");
            exit(-1);
        }
        // read the following features (between two \t)
        for (j = 0; j < attributeNum; j++) {
            end = strchr(begin, (int)('\t'));

            if (!end) {
                printf("(%d, %d)", i, j);
                fprintf(stderr, "line 404 tab wasn't found.\n");
                exit(-1);
            }
            memset(rawData[i][j], 0, MAX_LEN);
            strncpy(rawData[i][j], begin, end - begin);
            begin = end + 1;
        }
        // read the last feature (between \t and (\r or \n))
        end = strchr(begin, (int)('\r'));
        if (!end) {
            end = strchr(begin, (int)('\n'));
        }
        memset(rawData[i][j], 0, MAX_LEN);
        if (end) {
            strncpy(rawData[i][j], begin, end - begin);
        }
        else {
            strcpy(rawData[i][j], begin);
        }
        i++;
    }

    // init a attribute map during training phase
    if (flag == 1) {
        ConstructMap();
    }
    ConvertRawData2Map(flag);
}


void LoadDataset(int argc, char* argv[]) {
    Read(argc, argv);
    MallocMemory();
    InitMapName();
    OnReadData(trainingSetFile, 1);
    OnReadData(testingSetFile, 2);
}