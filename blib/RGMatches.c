#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <sys/types.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include "BLibDefinitions.h"
#include "BError.h"
#include "RGMatch.h"
#include "RGMatches.h"

#define RGMATCHES_CHECK 0

/* TODO */
int32_t RGMatchesRead(gzFile fp,
		RGMatches *m)
{
	char *FnName = "RGMatchesRead";
	int32_t i;

	/* Read read name length */
	if(gzread(fp, &m->readNameLength, sizeof(int32_t))!=sizeof(int32_t)) {
		return EOF;
	}
	assert(m->readNameLength < SEQUENCE_NAME_LENGTH);
	assert(m->readNameLength > 0);

	/* Allocate memory for the read name */
	m->readName = malloc(sizeof(char)*(m->readNameLength + 1));
	if(NULL == m->readName) {
		PrintError(FnName,
				"m->readName",
				"Could not allocate memory",
				Exit,
				MallocMemory);
	}

	/* Read in read name */
	if(gzread(fp, m->readName, sizeof(char)*m->readNameLength)!=sizeof(char)*m->readNameLength) {
		PrintError(FnName,
				"m->readName",
				"Could not read in read name",
				Exit,
				ReadFileError);
	}
	m->readName[m->readNameLength]='\0';
	/* Read numEnds */
	if(gzread(fp, &m->numEnds, sizeof(int32_t))!=sizeof(int32_t)) {
		PrintError(FnName,
				"numEnds",
				"Could not read in numEnds",
				Exit,
				ReadFileError);
	}

	/* Allocate the ends */
	m->ends = malloc(sizeof(RGMatch)*m->numEnds);
	if(NULL == m->ends) {
		PrintError(FnName,
				"m->ends",
				"Could not allocate memory",
				Exit,
				MallocMemory);
	}

	/* Read each end */
	for(i=0;i<m->numEnds;i++) {
		/* Initialize */
		RGMatchInitialize(&m->ends[i]);
		/* Read */
		RGMatchRead(fp,
				&m->ends[i]);
	}

	/* Check m */
	if(1==RGMATCHES_CHECK) {
		RGMatchesCheck(m);
	}

	return 1;
}

int32_t RGMatchesReadText(FILE *fp,
		RGMatches *m)
{
	char *FnName = "RGMatchesReadText";
	int32_t i;
	char readName[SEQUENCE_NAME_LENGTH]="\0";

	/* Read the matches from the input file */
	if(fscanf(fp, "%s %d", 
				readName,
				&m->numEnds) == EOF) {
		return EOF;
	}
	for(i=0;i<strlen(readName);i++) {
		readName[i] = readName[i+1];
	}
	m->readNameLength = strlen(readName);
	assert(m->readNameLength < SEQUENCE_NAME_LENGTH);
	assert(m->readNameLength > 0);
	/* Allocate memory for the read name */
	m->readName = malloc(sizeof(char)*(m->readNameLength + 1));
	if(NULL == m->readName) {
		PrintError(FnName,
				"m->readName",
				"Could not allocate memory",
				Exit,
				MallocMemory);
	}
	strcpy(m->readName, readName);

	/* Allocate the ends */
	m->ends = malloc(sizeof(RGMatch)*m->numEnds);
	if(NULL == m->ends) {
		PrintError(FnName,
				"m->ends",
				"Could not allocate memory",
				Exit,
				MallocMemory);
	}

	/* Read each end */
	for(i=0;i<m->numEnds;i++) {
		/* Initialize */
		RGMatchInitialize(&m->ends[i]);
		/* Read */
		RGMatchReadText(fp,
				&m->ends[i]);
	}

	/* Check m */
	if(1==RGMATCHES_CHECK) {
		RGMatchesCheck(m);
	}

	return 1;
}

/* TODO */
void RGMatchesPrint(gzFile fp,
		RGMatches *m)
{
	char *FnName = "RGMatchesPrint";
	int32_t i;
	assert(fp!=NULL);

	/* Check m */
	/*
	   if(1==RGMATCHES_CHECK) {
	   RGMatchesCheck(m);
	   }
	   */

	/* Print num ends, read name length, and read name */
	if(gzwrite(fp, &m->readNameLength, sizeof(int32_t))!=sizeof(int32_t) ||
			gzwrite(fp, m->readName, sizeof(char)*m->readNameLength)!=sizeof(char)*m->readNameLength ||
			gzwrite(fp, &m->numEnds, sizeof(int32_t))!=sizeof(int32_t))  {
		PrintError(FnName,
				NULL,
				"Could not write m->readNameLength, m->readName, and m->numEnds",
				Exit,
				WriteFileError);
	}

	/* Print each end */
	for(i=0;i<m->numEnds;i++) {
		RGMatchPrint(fp,
				&m->ends[i]);
	}
}
/* TODO */
void RGMatchesPrintText(FILE *fp,
		RGMatches *m)
{
	char *FnName = "RGMatchesPrintText";
	int32_t i;
	assert(fp!=NULL);

	/* Check m */
	/*
	   if(1==RGMATCHES_CHECK) {
	   RGMatchesCheck(m);
	   }
	   */

	/* Print the matches to the output file */
	/* Print read name length, read name, and num ends*/
	if(0 > fprintf(fp, "@%s %d\n",
				m->readName,
				m->numEnds)) {
		PrintError(FnName,
				NULL,
				"Could not write m->readNameLength, m->readName, and m->numEnds",
				Exit,
				WriteFileError);
	}

	/* Print each end */
	for(i=0;i<m->numEnds;i++) {
		RGMatchPrintText(fp,
				&m->ends[i]);
	}
}

/* TODO */
void RGMatchesPrintFastq(FILE *fp,
		RGMatches *m)
{
	/*
	   char *FnName = "RGMatchesPrintFastq";
	   */
	int32_t i;
	assert(fp!=NULL);

	/* Print the matches to the output file */
	/* Print read name length, read name, and num ends*/
	for(i=0;i<m->numEnds;i++) {
		RGMatchPrintFastq(fp,
				m->readName,
				&m->ends[i]);
	}
}

/* TODO */
void RGMatchesRemoveDuplicates(RGMatches *m,
		int32_t maxNumMatches)
{
	int32_t i;
	for(i=0;i<m->numEnds;i++) {
		RGMatchRemoveDuplicates(&m->ends[i], maxNumMatches);
		assert(m->ends[i].numEntries <= maxNumMatches);
	}
}

/* TODO */
/* Merges matches from the same read */
int32_t RGMatchesMergeFilesAndOutput(gzFile *tempFPs,
		int32_t numFiles,
		gzFile outputFP,
		int32_t maxNumMatches)
{
	char *FnName="RGMatchesMergeFilesAndOutput";
	int32_t i;
	int32_t foundMatch;
	int32_t counter;
	RGMatches matches;
	RGMatches tempMatches;
	int32_t numMatches=0;
	int32_t numFinished = 0;

	/* Initialize matches */
	RGMatchesInitialize(&matches);
	RGMatchesInitialize(&tempMatches);

	/* Read in each sequence/match one at a time */
	counter = 0;
	if(VERBOSE >=0) {
		fprintf(stderr, "\r[%d]", 0);
	}
	while(numFinished == 0) {
		if(VERBOSE >=0 && counter%RGMATCH_MERGE_ROTATE_NUM == 0) {
			fprintf(stderr, "\r[%d]", counter);
		}
		counter++;

		/* Read matches for one read from each file */ 
		for(i=0;i<numFiles;i++) {
			if(RGMatchesRead(tempFPs[i],
						&tempMatches)==EOF) {
				numFinished++;
			}
			else {
				if(matches.readName != NULL &&
						strcmp(matches.readName, tempMatches.readName)!=0) {
					PrintError(FnName,
							NULL,
							"Read names do not match",
							Exit,
							OutOfRange);
				}
				/* Append temp matches to matches */
				RGMatchesAppend(&matches, &tempMatches);
			}

			/* Free temp matches */
			RGMatchesFree(&tempMatches);
		}
		/* We must finish all at the same time */
		assert(numFinished == 0 || numFinished == numFiles);

		if(numFinished == 0) {
			/* Remove duplicates */
			RGMatchesRemoveDuplicates(&matches, maxNumMatches);

			/* Print to output file */
			for(i=foundMatch=0;foundMatch == 0 && i<matches.numEnds;i++) {
				if(0 < matches.ends[i].numEntries) {
					foundMatch = 1;
					numMatches++;
				}
			}
			RGMatchesPrint(outputFP,
					&matches);
		}
		/* Free memory */
		RGMatchesFree(&matches);
	}

	if(VERBOSE >=0) {
		fprintf(stderr, "\r[%d]... completed.\n", counter-1);
	}

	return numMatches;
}

/* TODO */
/* Merges matches from different reads */
int32_t RGMatchesMergeThreadTempFilesIntoOutputTempFile(gzFile *threadFPs,
		int32_t numThreads,
		gzFile outputFP)
{
	char *FnName = "RGMatchesMergeThreadTempFilesIntoOutputTempFile";
	int32_t counter;
	int32_t i;
	RGMatches matches;
	int32_t numFinished;
	int *finished=NULL;

	/* Initialize matches */
	RGMatchesInitialize(&matches);

	/* Allocate memory for the finished array */
	finished = malloc(sizeof(int)*numThreads);
	if(NULL == finished) {
		PrintError(FnName,
				"finished",
				"Could not allocate memory",
				Exit,
				MallocMemory);
	}
	/* Initialize finished array */
	for(i=0;i<numThreads;i++) {
		finished[i] = 0;
	}

	counter = 0;
	numFinished = 0;
	if(VERBOSE >= 0) {
		fprintf(stderr, "\r[%d]", counter);
	}
	while(numFinished < numThreads) {
		/* For each thread */
		for(i=0;i<numThreads;i++) {
			/* Only try reading from those that are not finished */
			if(0 == finished[i]) {
				if(RGMatchesRead(threadFPs[i],
							&matches)==EOF) {
					finished[i] = 1;
					numFinished++;
				}
				else {
					assert(numFinished < numThreads);

					/*
					   if(match.numEntries > 0) {
					   counter++;
					   }
					   */
					if(VERBOSE >=0 && counter%RGMATCH_MERGE_ROTATE_NUM == 0) {
						fprintf(stderr, "\r[%d]", counter);
					}
					counter++;

					RGMatchesPrint(outputFP,
							&matches);

				}
				/* Free memory */
				RGMatchesFree(&matches);
			}
		}
	}
	if(VERBOSE >= 0) {
		fprintf(stderr, "\r[%d]\n", counter);
	}
	assert(numFinished == numThreads);
	for(i=0;i<numThreads;i++) {
		assert(1==finished[i]);
	}

	/* Free memory */
	free(finished);

	return counter;
}

/* TODO */
void RGMatchesAppend(RGMatches *dest, RGMatches *src)
{
	char *FnName = "RGMatchesAppend";
	int32_t i;
	/* Check that we are not appending to ourselves */
	assert(src != dest);
	assert(NULL != src);
	assert(NULL != dest);

	/* Check to see if we need to add in the read name */
	if(dest->readNameLength <= 0) {
		assert(dest->readName == NULL);
		dest->readNameLength = src->readNameLength;

		/* Allocate memory for the read name */
		dest->readName = malloc(sizeof(char)*(dest->readNameLength+1));
		if(NULL==dest->readName) {
			PrintError(FnName,
					"dest->readName",
					"Could not allocate memory",
					Exit,
					MallocMemory);
		}
		strcpy(dest->readName, src->readName);

		assert(dest->numEnds <= src->numEnds);
		if(dest->numEnds < src->numEnds) {
			dest->ends = realloc(dest->ends, sizeof(RGMatch)*src->numEnds);
			if(NULL==dest->ends) {
				PrintError(FnName,
						"dest->ends",
						"Could not reallocate memory",
						Exit,
						ReallocMemory);
			}
			for(i=dest->numEnds;i<src->numEnds;i++) {
				RGMatchInitialize(&dest->ends[i]);
			}
			dest->numEnds = src->numEnds;
		}
	}
	assert(src->numEnds == dest->numEnds);

	/* Append the matches */
	assert(src->numEnds == dest->numEnds);
	for(i=0;i<dest->numEnds;i++) {
		RGMatchAppend(&dest->ends[i], &src->ends[i]);
	}
}

/* TODO */
void RGMatchesReallocate(RGMatches *m,
		int32_t numEnds)
{
	char *FnName="RGMatchesReallocate";
	int32_t i;

	if(numEnds < m->numEnds) {
		for(i=numEnds;i<m->numEnds;i++) {
			RGMatchFree(&m->ends[i]);
		}
	}

	m->ends = realloc(m->ends, sizeof(RGMatch)*numEnds);
	if(NULL == m->ends) {
		PrintError(FnName,
				"m->ends",
				"Could not allocate memory",
				Exit,
				MallocMemory);
	}

	for(i=m->numEnds;i<numEnds;i++) {
		RGMatchInitialize(&m->ends[i]);
	}
	m->numEnds = numEnds;
}

/* TODO */
void RGMatchesFree(RGMatches *m) 
{
	int32_t i;
	free(m->readName);
	for(i=0;i<m->numEnds;i++) {
		RGMatchFree(&m->ends[i]);
	}
	free(m->ends);
	RGMatchesInitialize(m);
}

/* TODO */
void RGMatchesInitialize(RGMatches *m)
{
	m->readNameLength = 0;
	m->readName = NULL;
	m->numEnds = 0;
	m->ends=NULL;
}

/* TODO */
void RGMatchesMirrorPairedEnd(RGMatches *m,
		RGBinary *rg,
		int32_t pairedEndLength,
		int32_t mirroringType,
		int32_t forceMirroring)
{
	int i, tempNumEntries;
	int numEntriesOne, numEntriesTwo;

	/* For paired end only */
	if(2 == m->numEnds) { 
		numEntriesOne = m->ends[0].numEntries;
		numEntriesTwo = m->ends[1].numEntries;

		/* Copy matches from first to second */
		if(forceMirroring == 1 || 
				(m->ends[0].numEntries > 0 && m->ends[1].numEntries <= 0)) {
			/* Copy forward */
			if(MirrorBoth == mirroringType ||
					MirrorForward == mirroringType) {
				RGMatchReallocate(&m->ends[1], numEntriesOne + numEntriesTwo);
				for(i=0;i<numEntriesOne;i++) {
					m->ends[1].contigs[i+numEntriesTwo] = m->ends[0].contigs[i];
					m->ends[1].strands[i+numEntriesTwo] = m->ends[0].strands[i];
					/* Adjust position */
					m->ends[1].positions[i+numEntriesTwo] = m->ends[0].positions[i] + m->ends[0].readLength + pairedEndLength;
				}
			}
			/* Copy reverse */
			if(MirrorBoth == mirroringType || 
					MirrorReverse == mirroringType) {
				tempNumEntries = m->ends[1].numEntries;
				RGMatchReallocate(&m->ends[1], numEntriesOne + tempNumEntries);
				for(i=0;i<numEntriesOne;i++) {
					m->ends[1].contigs[i+tempNumEntries] = m->ends[0].contigs[i];
					m->ends[1].strands[i+tempNumEntries] = m->ends[0].strands[i];
					/* Adjust position */
					m->ends[1].positions[i+tempNumEntries] = m->ends[0].positions[i] - m->ends[0].readLength - pairedEndLength;
				}
			}
		}
		/* Copy matches from second to first */
		if(forceMirroring == 1 || 
				(m->ends[0].numEntries <= 0 && m->ends[1].numEntries > 0)) {
			/* Copy forward */
			if(MirrorBoth == mirroringType ||
					MirrorForward == mirroringType) {
				RGMatchReallocate(&m->ends[0], numEntriesOne + numEntriesTwo);
				for(i=0;i<numEntriesTwo;i++) {
					m->ends[0].contigs[i+numEntriesOne] = m->ends[1].contigs[i];
					m->ends[0].strands[i+numEntriesOne] = m->ends[1].strands[i];
					/* Adjust position */
					m->ends[0].positions[i+numEntriesOne] = m->ends[1].positions[i] - m->ends[0].readLength - pairedEndLength;
				}
			}
			/* Copy reverse */
			if(MirrorBoth == mirroringType || 
					MirrorReverse == mirroringType) {
				tempNumEntries = m->ends[0].numEntries;
				RGMatchReallocate(&m->ends[0], tempNumEntries + numEntriesTwo);
				for(i=0;i<numEntriesTwo;i++) {
					m->ends[0].contigs[i+tempNumEntries] = m->ends[1].contigs[i];
					m->ends[0].strands[i+tempNumEntries] = m->ends[1].strands[i];
					/* Adjust position */
					m->ends[0].positions[i+tempNumEntries] = m->ends[1].positions[i] + m->ends[0].readLength + pairedEndLength;
				}
			}
		}
		/* Check m */
		/*
		   if(1==RGMATCHES_CHECK) {
		   RGMatchesCheck(m);
		   }
		   */
		RGMatchesRemoveDuplicates(m, INT_MAX);
		/* Adjust positions in case they trail off the end of the contigs */
		for(i=0;i<m->ends[0].numEntries;i++) {
			m->ends[0].positions[i] = MAX(1, MIN(m->ends[0].positions[i], rg->contigs[m->ends[0].contigs[i]-1].sequenceLength));
		}
		for(i=0;i<m->ends[1].numEntries;i++) {
			m->ends[1].positions[i] = MAX(1, MIN(m->ends[1].positions[i], rg->contigs[m->ends[1].contigs[i]-1].sequenceLength));
		}
	}
}

/* TODO */
void RGMatchesCheck(RGMatches *m) 
{
	char *FnName="RGMatchesCheck";
	int32_t i;
	/* Basic asserts */
	assert(m->numEnds == 0 || m->numEnds == 1);
	/* Check that the read name length is the same as the length of the read name */
	if(((int)strlen(m->readName)) != m->readNameLength) {
		PrintError(FnName,
				NULL,
				"strlen(m->readName)) != m->readNameLength",
				Exit,
				OutOfRange);
	}
	for(i=0;i<m->numEnds;i++) {
		RGMatchCheck(&m->ends[i]);
	}
}

/* TODO */
void RGMatchesFilterOutOfRange(RGMatches *m,
		int32_t startChr,
		int32_t startPos,
		int32_t endChr,
		int32_t endPos,
		int32_t maxNumMatches)
{
	int32_t i;
	for(i=0;i<m->numEnds;i++) {
		RGMatchFilterOutOfRange(&m->ends[i],
				startChr,
				startPos,
				endChr,
				endPos,
				maxNumMatches);
	}
}
