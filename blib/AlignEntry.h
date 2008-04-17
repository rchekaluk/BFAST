#ifndef ALIGNENTRY_H_
#define ALIGNENTRY_H_

#include <stdio.h>
#include "BLibDefinitions.h"

/* TODO */
typedef struct {
	char readName[SEQUENCE_NAME_LENGTH];
	char *read; /* The read */
	char *reference;
	unsigned int length; /* The length of the alignment */
	unsigned int chromosome;
	unsigned int position;
	char strand;
	double score;
} AlignEntry;

void AlignEntryPrint(AlignEntry*, FILE*);
int AlignEntryRead(AlignEntry*, FILE*);
int AlignEntryRemoveDuplicates(AlignEntry**, int);
void AlignEntryQuickSort(AlignEntry**, int, int);
void AlignEntryCopyAtIndex(AlignEntry*, int, AlignEntry*, int);
int AlignEntryCompareAtIndex(AlignEntry*, int, AlignEntry*, int);
#endif