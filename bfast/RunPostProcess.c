#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <zlib.h>
#include <limits.h>
#include <math.h>
#include "BLibDefinitions.h"
#include "BError.h"
#include "AlignedRead.h"
#include "AlignedEnd.h"
#include "AlignedReadConvert.h"
#include "ScoringMatrix.h"
#include "RunPostProcess.h"

/* TODO */
void ReadInputFilterAndOutput(RGBinary *rg,
		char *inputFileName,
		int algorithm,
		int pairedEndInfer,
		int queueLength,
		int outputFormat,
		char *outputID,
		char *unmappedFileName)
{
	char *FnName="ReadInputFilterAndOutput";
	gzFile fp=NULL;
	int32_t i, j;
	int64_t counter, foundType, numUnmapped, numReported;
	AlignedRead *aBuffer;
	int32_t aBufferLength=0;
	int32_t numRead, aBufferIndex;
	gzFile fpReportedGZ=NULL;
	FILE *fpReported=NULL;
	gzFile fpUnmapped=NULL;
	PEDBins bins;
	int32_t *mappedEndCounts=NULL;
	int32_t maxNumEnds=-1;

	/* Get the PEDBins if necessary */
	if(1 == pairedEndInfer) {
		PEDBinsInitialize(&bins);
		pairedEndInfer = GetPEDBins(inputFileName, algorithm, queueLength, &bins);
	}

	/* Open the input file */
	if(NULL == inputFileName) {
		if(!(fp=gzdopen(fileno(stdin), "rb"))) {
			PrintError(FnName, "stdin", "Could not open stdin for reading", Exit, OpenFileError);
		}
	}
	else {
		if(!(fp=gzopen(inputFileName, "rb"))) {
			PrintError(FnName, inputFileName, "Could not open inputFileName for reading", Exit, OpenFileError);
		}
	}

	/* Open output files, if necessary */
	if(BAF == outputFormat) {
		if(!(fpReportedGZ=gzdopen(fileno(stdout), "wb"))) {
			PrintError(FnName, "stdout", "Could not open stdout for writing", Exit, OpenFileError);
		}
	}
	else {
		if(!(fpReported=fdopen(fileno(stdout), "wb"))) {
			PrintError(FnName, "stdout", "Could not open stdout for writing", Exit, OpenFileError);
		}
	}
	if(NULL != unmappedFileName) {
		if(!(fpUnmapped=gzopen(unmappedFileName, "wb"))) {
			PrintError(FnName, unmappedFileName, "Could not open unmappedFileName for writing", Exit, OpenFileError);
		}
	}

	AlignedReadConvertPrintHeader(fpReported, rg, outputFormat);

	aBufferLength=queueLength;
	aBuffer=malloc(sizeof(AlignedRead)*aBufferLength);
	if(NULL == aBuffer) {
		PrintError(FnName, "aBuffer", "Could not allocate memory", Exit, MallocMemory);
	}

	/* Go through each read */
	if(VERBOSE >= 0) {
		fprintf(stderr, "Processing reads, currently on:\n0");
	}
	counter = numReported = numUnmapped = numRead = 0;

	while(0 != (numRead = GetAlignedReads(fp, aBuffer, aBufferLength))) {

		for(aBufferIndex=0;aBufferIndex<numRead;aBufferIndex++) {

			if(VERBOSE >= 0 && counter%ALIGNENTRIES_READ_ROTATE_NUM==0) {
				fprintf(stderr, "\r%lld",
						(long long int)counter);
			}

			/* Filter */
			foundType=FilterAlignedRead(&aBuffer[aBufferIndex],
					algorithm,
					pairedEndInfer,
					&bins);

			int32_t numEnds=0;
			if(NoneFound == foundType) {
				/* Print to Not Reported file */
				if(NULL != unmappedFileName) {
					AlignedReadPrint(&aBuffer[aBufferIndex], fpUnmapped);
				}
				numUnmapped++;

				/* Free the alignments for output */
				for(i=0;i<aBuffer[aBufferIndex].numEnds;i++) {
					for(j=0;j<aBuffer[aBufferIndex].ends[i].numEntries;j++) {
						AlignedEntryFree(&aBuffer[aBufferIndex].ends[i].entries[j]);
					}
					aBuffer[aBufferIndex].ends[i].numEntries=0;
				}
			}
			else {
				numReported++;

				for(i=0;i<aBuffer[aBufferIndex].numEnds;i++) {
					if(0 < aBuffer[aBufferIndex].ends[i].numEntries) {
						numEnds++;
					}
				}
			}
			if(maxNumEnds < numEnds) {
				// Reallocate
				mappedEndCounts = realloc(mappedEndCounts, sizeof(int32_t)*(1+numEnds));
				if(NULL == mappedEndCounts) {
					PrintError(FnName, "mappedEndCounts", "Could not reallocate memory", Exit, ReallocMemory);
				}
				// Initialize
				for(i=1+maxNumEnds;i<=numEnds;i++) {
					mappedEndCounts[i] = 0;
				}
				maxNumEnds = numEnds;
			}
			mappedEndCounts[numEnds]++;

			/* Increment counter */
			counter++;
		}

		if(VERBOSE >= 0) {
			fprintf(stderr, "\r%lld",
					(long long int)counter);
		}

		/* Print to Output file */
		for(aBufferIndex=0;aBufferIndex<numRead;aBufferIndex++) {
			AlignedReadConvertPrintOutputFormat(&aBuffer[aBufferIndex], rg, fpReported, fpReportedGZ, (NULL == outputID) ? "" : outputID, outputFormat, BinaryOutput);

			/* Free memory */
			AlignedReadFree(&aBuffer[aBufferIndex]);
		}
	}
	if(VERBOSE>=0) {
		fprintf(stderr, "\r%lld\n",
				(long long int)counter);
	}

	/* Close output files, if necessary */
	if(BAF == outputFormat) {
		gzclose(fpReportedGZ);
	}
	else {
		fclose(fpReported);
	}
	if(NULL != unmappedFileName) {
		gzclose(fpUnmapped);
	}
	/* Close the input file */
	gzclose(fp);
	free(aBuffer);
	
	if(VERBOSE>=0) {
		fprintf(stderr, "%s", BREAK_LINE);
		fprintf(stderr, "Found %10d reads with no ends mapped.\n", mappedEndCounts[0]);
		assert(numUnmapped == mappedEndCounts[0]);
		for(i=1;i<=maxNumEnds;i++) {
			if(1 == i) fprintf(stderr, "Found %10d reads with %2d end mapped.\n", mappedEndCounts[i], i);
			else fprintf(stderr, "Found %10d reads with %2d ends mapped.\n", mappedEndCounts[i], i);
		}
		fprintf(stderr, "Found %10lld reads with at least one end mapping.\n",
				(long long int)numReported);
		fprintf(stderr, "%s", BREAK_LINE);
	}

	/* Free */
	if(1 == pairedEndInfer) {
		PEDBinsFree(&bins);
	}
	free(mappedEndCounts);

}

int32_t GetAlignedReads(gzFile fp, AlignedRead *aBuffer, int32_t maxToRead)
{
	int32_t numRead=0;
	while(numRead < maxToRead) {
		AlignedReadInitialize(&aBuffer[numRead]);
		if(EOF == AlignedReadRead(&aBuffer[numRead], fp)) {
			return numRead;
		}
		numRead++;
	}
	return numRead;
}

/* TODO */
int FilterAlignedRead(AlignedRead *a,
		int algorithm,
		int  pairedEndInfer,
		PEDBins *b)
{
	char *FnName="FilterAlignedRead";
	int foundType;
	int32_t *foundTypes=NULL;
	AlignedRead tmpA;
	int32_t i, j, ctr;
	int32_t best, bestIndex, numBest;

	AlignedReadInitialize(&tmpA);

	/* We should only modify "a" if it is going to be reported */ 
	/* Copy in case we do not find anything to report */
	AlignedReadCopy(&tmpA, a);


	foundType=NoneFound;
	foundTypes=malloc(sizeof(int32_t)*tmpA.numEnds);
	if(NULL == foundTypes) {
		PrintError(FnName, "foundTypes", "Could not allocate memory", Exit, MallocMemory);
	}
	for(i=0;i<tmpA.numEnds;i++) {
		foundTypes[i]=NoneFound;
	}

	/* Pick alignment for each end individually (is this a good idea?) */
	for(i=0;i<tmpA.numEnds;i++) {
		/* Choose each end */
		switch(algorithm) {
			case NoFiltering:
			case AllNotFiltered:
				foundTypes[i] = (0<tmpA.ends[i].numEntries)?Found:NoneFound;
				break;
			case Unique:
				foundTypes[i]=(1==tmpA.ends[i].numEntries)?Found:NoneFound;
				break;
			case BestScore:
			case BestScoreAll:
				best = INT_MIN;
				bestIndex = -1;
				numBest = 0;
				for(j=0;j<tmpA.ends[i].numEntries;j++) {
					if(best < tmpA.ends[i].entries[j].score) {
						best = tmpA.ends[i].entries[j].score;
						bestIndex = j;
						numBest = 1;
					}
					else if(best == tmpA.ends[i].entries[j].score) {
						numBest++;
					}
				}
				if(BestScore == algorithm &&
						1 == numBest) {
					foundTypes[i] = Found;
					/* Copy to front */
					AlignedEntryCopy(&tmpA.ends[i].entries[0], 
							&tmpA.ends[i].entries[bestIndex]);
					AlignedEndReallocate(&tmpA.ends[i], 1);
				}
				else if(BestScoreAll == algorithm) {
					foundTypes[i] = Found;
					ctr=0;
					for(j=0;j<tmpA.ends[i].numEntries;j++) {
						if(tmpA.ends[i].entries[j].score == best) {
							if(ctr != j) {
								AlignedEntryCopy(&tmpA.ends[i].entries[ctr], 
										&tmpA.ends[i].entries[j]);
							}
							ctr++;
						}
					}
					assert(ctr == numBest);
					AlignedEndReallocate(&tmpA.ends[i], numBest);
				}
				break;
			default:
				PrintError(FnName, "algorithm", "Could not understand algorithm", Exit, OutOfRange);
				break;
		}
		/* Free if not found */
		if(NoneFound == foundTypes[i]) {
			AlignedEndReallocate(&tmpA.ends[i],
					0);
		}
	}

	if(1 == tmpA.numEnds) {
		foundType=foundTypes[0];
	}
	else {
		/* Call found if at least one has been found */
		foundType=NoneFound;
		for(i=0;NoneFound==foundType && i<tmpA.numEnds;i++) {
			if(Found == foundTypes[i]) {
				foundType=Found;
				break;
			}
		}
	}

	/* If we found, then copy back */
	if(NoneFound != foundType) {
		AlignedReadFree(a);
		AlignedReadCopy(a, &tmpA);
	}
	AlignedReadFree(&tmpA);
	free(foundTypes);

	return foundType;
}

int32_t GetPEDBins(char *inputFileName, 
		int algorithm,
		int queueLength,
		PEDBins *b)
{
	char *FnName="GetPEDBins";
	gzFile fp=NULL;
	int64_t counter, foundType;
	AlignedRead *aBuffer;
	int32_t aBufferLength=0;
	int32_t numRead, aBufferIndex;

	/* Open the input file */
	if(NULL == inputFileName) {
		if(!(fp=gzdopen(fileno(stdin), "rb"))) {
			PrintError(FnName, "stdin", "Could not open stdin for reading", Exit, OpenFileError);
		}
	}
	else {
		if(!(fp=gzopen(inputFileName, "rb"))) {
			PrintError(FnName, inputFileName, "Could not open inputFileName for reading", Exit, OpenFileError);
		}
	}

	aBufferLength=queueLength;
	aBuffer=malloc(sizeof(AlignedRead)*aBufferLength);
	if(NULL == aBuffer) {
		PrintError(FnName, "aBuffer", "Could not allocate memory", Exit, MallocMemory);
	}

	/* Go through each read */
	if(VERBOSE >= 0) {
		fprintf(stderr, "Estimating paired end distance, currently on:\n0");
	}
	counter = numRead = 0;

	while(0 != (numRead = GetAlignedReads(fp, aBuffer, aBufferLength))) {

		for(aBufferIndex=0;aBufferIndex<numRead;aBufferIndex++) {

			if(2 == aBuffer[aBufferIndex].numEnds) { // Only paired end data

				if(VERBOSE >= 0 && counter%ALIGNENTRIES_READ_ROTATE_NUM==0) {
					fprintf(stderr, "\r%lld",
							(long long int)counter);
				}

				/* Filter */
				foundType=FilterAlignedRead(&aBuffer[aBufferIndex],
						algorithm,
						0,
						NULL);

				if(Found == foundType) {
					assert(2 == aBuffer[aBufferIndex].numEnds);
					/* Must only have one alignment per end and on the same contig.
					 * There is a potential this will be inferred incorrectly under
					 * many scenarios.  Be careful! */
					if(1 == aBuffer[aBufferIndex].ends[0].numEntries &&
							1 == aBuffer[aBufferIndex].ends[1].numEntries &&
							aBuffer[aBufferIndex].ends[0].entries[0].contig == aBuffer[aBufferIndex].ends[1].entries[0].contig) {
						PEDBinsInsert(b, 
								fabs(aBuffer[aBufferIndex].ends[1].entries[0].position - aBuffer[aBufferIndex].ends[0].entries[0].position));
					}
				}
			}

			/* Increment counter */
			counter++;
		}

		if(VERBOSE >= 0) {
			fprintf(stderr, "\r%lld",
					(long long int)counter);
		}

		/* Free buffer */
		for(aBufferIndex=0;aBufferIndex<numRead;aBufferIndex++) {
			/* Free memory */
			AlignedReadFree(&aBuffer[aBufferIndex]);
		}

		/* Do we really need this many? */
		if(MAX_PEDBINS_SIZE < counter) {
			break;
		}
	}
	if(VERBOSE>=0) {
		fprintf(stderr, "\r%lld\n",
				(long long int)counter);
	}

	/* Close the input file */
	gzclose(fp);
	free(aBuffer);

	if(b->numDistances < MIN_PEDBINS_SIZE) {
		fprintf(stderr, "Found only %d distances to infer the insert size distribution\n", b->numDistances);
		PrintError(FnName, "b->numDistances", "Not enough distances to infer insert size distribution", Warn, OutOfRange);
		PEDBinsFree(b);
		return 0;
	}

	if(VERBOSE>=0) {
		// Print Statistics
		PEDBinsPrintStatistics(b, stderr);
	}
	return 1;
}

void PEDBinsInitialize(PEDBins *b)
{
	b->minDistance = INT_MAX;
	b->maxDistance = INT_MIN;
	b->bins = NULL;
	b->numDistances = 0;
}

void PEDBinsFree(PEDBins *b) {
	free(b->bins);
	PEDBinsInitialize(b);
}

void PEDBinsInsert(PEDBins *b,
		int32_t distance)
{
	char *FnName="PEDBinsInsert";
	int32_t prevMinDistance, prevMaxDistance;
	int32_t i;

	if(distance < MIN_PEDBINS_DISTANCE ||
			MAX_PEDBINS_DISTANCE < distance) {
		return;
	}

	if(distance < b->minDistance) {
		if(INT_MIN < b->maxDistance) { // Bins exist
			prevMinDistance = b->minDistance;
			b->minDistance = distance;
			b->bins = realloc(b->bins, sizeof(int32_t)*(b->maxDistance - b->minDistance + 1));
			if(NULL == b->bins) {
				PrintError(FnName, "b->bins", "Could not reallocate memory", Exit, ReallocMemory);
			}
			// Move over old bins
			for(i=b->maxDistance - prevMinDistance;0<=i;i--) {
				b->bins[i + (prevMinDistance - b->minDistance)] = b->bins[i];
			}
			// Initialize new bins
			for(i=0;i < prevMinDistance - b->minDistance;i++) {
				b->bins[i] = 0;
			}
		}
		else { // No bins
			b->minDistance = b->maxDistance = distance;
			b->bins = malloc(sizeof(int32_t));
			if(NULL == b->bins) {
				PrintError(FnName, "b->bins", "Could not allocate memory", Exit, MallocMemory);
			}
		}
	}
	else if(b->maxDistance < distance) {
		if(b->minDistance < INT_MAX) { // Bins exist
			prevMaxDistance = b->maxDistance;
			b->maxDistance = distance;
			b->bins = realloc(b->bins, sizeof(int32_t)*(b->maxDistance - b->minDistance + 1));
			if(NULL == b->bins) {
				PrintError(FnName, "b->bins", "Could not reallocate memory", Exit, ReallocMemory);
			}
			// Initialize new bins
			for(i=prevMaxDistance-b->minDistance+1;i<b->maxDistance-b->minDistance+1;i++) {
				b->bins[i] = 0;
			}
		}
		else { // No bins
			b->minDistance = b->maxDistance = distance;
			b->bins = malloc(sizeof(int32_t));
			if(NULL == b->bins) {
				PrintError(FnName, "b->bins", "Could not allocate memory", Exit, MallocMemory);
			}
		}
	}

	// Add to bin
	b->bins[distance - b->minDistance]++;
	b->numDistances++;
}

void PEDBinsPrintStatistics(PEDBins *b, FILE *fp)
{
	// Mean, Range, and SD
	int32_t i;
	double mean, sd;

	// Mean
	mean = 0.0;
	for(i=0;i<b->maxDistance-b->minDistance+1;i++) {
		mean += (b->minDistance + i)*b->bins[i];
	}
	mean /= b->numDistances;

	// SD
	sd = 0.0;
	for(i=0;i<b->maxDistance-b->minDistance+1;i++) {
		sd += b->bins[i]*((b->minDistance + i) - mean)*((b->minDistance + i) - mean);
	}
	sd /= b->numDistances-1;
	sd = sqrt(sd);

	if(0<=VERBOSE) {
		fprintf(stderr, "Used %d paired end distances to infer the insert size distribution.\n",
				b->numDistances);
		fprintf(stderr, "The paired end distance range was from %d to %d.\n",
				b->minDistance, b->maxDistance);
		fprintf(stderr, "The paired end distance mean and standard deviation was %.2lf and %.2lf.\n",
				mean, sd);
		fprintf(stderr, "%s", BREAK_LINE);
	}
}