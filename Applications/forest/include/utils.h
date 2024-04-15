#ifndef UTILS_H
#define UTILS_H

typedef unsigned char uint8_t;


int ConvertString2Number(char* str);

void ConvertNum2Str(uint8_t j, char* str);

int IsNumber(char* str);

int NumberCmp(const void *a, const void *b);

void printData(const uint8_t **dataset, int row, int col);

double Log2(double n);

void MySwap(int arr[], int left, int right);

#endif


