#ifndef RUNPOSTPROCESS_H_
#define RUNPOSTPROCESS_H_

#include "AlignedRead.h"

/* Paired End Distance Bins */
// This distance of the second end minus the first end
typedef struct {
	int32_t minDistance;
	int32_t maxDistance;
	int32_t *bins;
	int32_t numDistances;
} PEDBins;

typedef struct {
	RGBinary *rg;
	PEDBins *bins;
	int algorithm;
	int pairedEndInfer;
	int queueLength;
	int outputFormat;
	char *readGroupString;
	char *outputID;
	int32_t **mappedEndCounts;
	int32_t *mappedEndCountsNumEnds;
	int32_t *numReported, *numUnmapped, *pairedEndInferRescued;

	pthread_mutex_t *inputFP_mutex;
	int32_t *input_threadID;
	gzFile fpIn;

	FILE *fpReported;
	gzFile fpReportedGZ;
	gzFile fpUnmapped;
	pthread_mutex_t *outputFP_mutex;
	int32_t threadID;
} PostProcessThreadData;

void ReadInputFilterAndOutput(RGBinary *rg,
		char *inputFileName,
		int algorithm,
		int pairedEndInfer,
		int numThreads,
		int queueLength,
		int outputFormat,
		char *outputID,
		char *readGroup,
		char *unmappedFileName,
		FILE *fpOut);

void *ReadInputFilterAndOutputThread(void*);

int32_t GetPEDBins(char*, int, int, PEDBins*);

int32_t GetAlignedReads(gzFile, AlignedRead*, int32_t, pthread_mutex_t*);

int FilterAlignedRead(AlignedRead *a,
		int algorithm,
		int pairedEndInfer,
		PEDBins *b,
		int32_t *pairedEndInferRescue);

void PEDBinsInitialize(PEDBins*);
void PEDBinsFree(PEDBins*);
void PEDBinsInsert(PEDBins*, int32_t);
void PEDBinsPrintStatistics(PEDBins*, FILE*);
void PEDBinsMakeIntoProbability(PEDBins*);
int32_t PEDBinsGetProbability(PEDBins*, int32_t, int32_t, int32_t, int32_t);

#endif
