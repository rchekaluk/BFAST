#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <zlib.h>
#include "../blib/BLibDefinitions.h"
#include "../blib/BError.h"
#include "../blib/AlignedRead.h"
#include "../blib/AlignedReadConvert.h"
#include "../blib/ScoringMatrix.h"
#include "Definitions.h"
#include "Filter.h"
#include "InputOutputToFiles.h"

/* TODO */
void ReadInputFilterAndOutput(RGBinary *rg,
		char *inputFileName,
		int startContig,
		int startPos,
		int endContig,
		int endPos,
		int algorithm,
		int minScore,
		int minQual,
		int maxMismatches,
		int maxColorErrors,
		int minDistancePaired,
		int maxDistancePaired,
		int useDistancePaired,
		int contigAbPaired,
		int inversionsPaired,
		int unpaired,
		char *outputID,
		char *outputDir,
		int outputFormat)
{
	char *FnName="ReadInputFilterAndOutput";
	gzFile fp=NULL;
	int64_t counter, foundType, numContigAb, numUnpaired, numInversions, numNotReported, numReported;
	AlignedRead a;
	char outputFileName[MAX_FILENAME_LENGTH]="\0";
	gzFile fpOutGZ=NULL;
	FILE *fpOut=NULL;
	char contigAbFileName[MAX_FILENAME_LENGTH]="\0";
	gzFile fpContigAbGZ=NULL;
	FILE* fpContigAb=NULL;
	char inversionsFileName[MAX_FILENAME_LENGTH]="\0";
	gzFile fpInversionsGZ=NULL;
	FILE* fpInversions=NULL;
	char unpairedFileName[MAX_FILENAME_LENGTH]="\0";
	gzFile fpUnpairedGZ=NULL;
	FILE* fpUnpaired=NULL;
	char notReportedFileName[MAX_FILENAME_LENGTH]="\0";
	gzFile fpNotReportedGZ=NULL;
	FILE *fpNotReported=NULL;
	char fileExtension[256]="\0";

	/* Open the input file */
	if(!(fp=gzopen(inputFileName, "rb"))) {
		PrintError(FnName,
				inputFileName,
				"Could not open inputFileName for reading",
				Exit,
				OpenFileError);
	}

	/* Get file extension for the output files */
	switch(outputFormat) {
		case BAF:
			strcpy(fileExtension,  BFAST_ALIGNED_FILE_EXTENSION);
			break;
		case MAF:
			strcpy(fileExtension, BFAST_MAF_FILE_EXTENSION);
			break;
		case GFF:
			strcpy(fileExtension, BFAST_GFF_FILE_EXTENSION);
			break;
		case SAM:
			strcpy(fileExtension, BFAST_SAM_FILE_EXTENSION);
			break;
		default:
			PrintError(FnName,
					"outputFormat",
					"Could not understand output format",
					Exit,
					OutOfRange);
	}
	/* Create output file names */
	sprintf(contigAbFileName, "%s%s.contigab.file.%s.%s",
			outputDir,
			PROGRAM_NAME,
			outputID,
			fileExtension);
	sprintf(inversionsFileName, "%s%s.inversion.file.%s.%s",
			outputDir,
			PROGRAM_NAME,
			outputID,
			fileExtension);
	sprintf(unpairedFileName, "%s%s.unpaired.file.%s.%s",
			outputDir,
			PROGRAM_NAME,
			outputID,
			fileExtension);
	sprintf(notReportedFileName, "%s%s.not.reported.file.%s.%s",
			outputDir,
			PROGRAM_NAME,
			outputID,
			fileExtension);
	sprintf(outputFileName, "%s%s.reported.file.%s.%s",
			outputDir,
			PROGRAM_NAME,
			outputID,
			fileExtension);

	/* Open output files, if necessary */
	if(contigAbPaired == 1) {
		if(BAF == outputFormat) { 
			if(!(fpContigAbGZ=gzopen(contigAbFileName, "wb"))) {
				PrintError(FnName,
						contigAbFileName,
						"Could not open contigAbFileName for writing",
						Exit,
						OpenFileError);
			}
		}
		else {
			if(!(fpContigAb=fopen(contigAbFileName, "wb"))) {
				PrintError(FnName,
						contigAbFileName,
						"Could not open contigAbFileName for writing",
						Exit,
						OpenFileError);
			}
		}
		AlignedReadConvertPrintHeader(fpContigAb, rg, outputFormat);
	}
	if(inversionsPaired == 1) {
		if(BAF == outputFormat) { 
			if(!(fpInversionsGZ=gzopen(inversionsFileName, "wb"))) {
				PrintError(FnName,
						inversionsFileName,
						"Could not open inversionsFileName for writing",
						Exit,
						OpenFileError);
			}
		}
		else {
			if(!(fpInversions=fopen(inversionsFileName, "wb"))) {
				PrintError(FnName,
						inversionsFileName,
						"Could not open inversionsFileName for writing",
						Exit,
						OpenFileError);
			}
		}
		AlignedReadConvertPrintHeader(fpInversions, rg, outputFormat);
	}
	if(unpaired == 1) {
		if(BAF == outputFormat) { 
			if(!(fpUnpairedGZ=gzopen(unpairedFileName, "wb"))) {
				PrintError(FnName,
						unpairedFileName,
						"Could not open unpairedFileName for writing",
						Exit,
						OpenFileError);
			}
		}
		else {
			if(!(fpUnpaired=fopen(unpairedFileName, "wb"))) {
				PrintError(FnName,
						unpairedFileName,
						"Could not open unpairedFileName for writing",
						Exit,
						OpenFileError);
			}
		}
		AlignedReadConvertPrintHeader(fpUnpaired, rg, outputFormat);
	}
	if(BAF == outputFormat) { 
		if(!(fpNotReportedGZ=gzopen(notReportedFileName, "wb"))) {
			PrintError(FnName,
					notReportedFileName,
					"Could not open notReportedFileName for writing",
					Exit,
					OpenFileError);
		}
	}
	else {
		if(!(fpNotReported=fopen(notReportedFileName, "wb"))) {
			PrintError(FnName,
					notReportedFileName,
					"Could not open notReportedFileName for writing",
					Exit,
					OpenFileError);
		}
	}
	AlignedReadConvertPrintHeader(fpNotReported, rg, outputFormat);
	if(BAF == outputFormat) {
		if(!(fpOutGZ=gzopen(outputFileName, "wb"))) {
			PrintError(FnName,
					outputFileName,
					"Could not open outputFileName for writing",
					Exit,
					OpenFileError);
		}
	}
	else {
		if(!(fpOut=fopen(outputFileName, "wb"))) {
			PrintError(FnName,
					outputFileName,
					"Could not open outputFileName for writing",
					Exit,
					OpenFileError);
		}
	}

	AlignedReadConvertPrintHeader(fpOut, rg, outputFormat);

	/* Initialize */
	AlignedReadInitialize(&a);

	/* Go through each read */
	if(VERBOSE >= 0) {
		fprintf(stderr, "Processing reads, currently on:\n0");
	}
	counter = numReported = numNotReported = numContigAb = numUnpaired = numInversions = 0;
	while(EOF != AlignedReadRead(&a, fp)) {
		if(VERBOSE >= 0 && counter%ALIGNENTRIES_READ_ROTATE_NUM==0) {
			fprintf(stderr, "\r%lld",
					(long long int)counter);
		}
		/* Filter */
		foundType=FilterAlignedRead(&a,
				algorithm,
				minScore,
				minQual,
				startContig,        
				startPos,
				endContig,        
				endPos,
				maxMismatches,
				maxColorErrors,
				minDistancePaired,          
				maxDistancePaired,
				useDistancePaired,
				contigAbPaired,
				inversionsPaired,
				unpaired
				);

		/* Print the apporiate files based on the return type */
		switch(foundType) {
			case NoneFound:
				/* Print to Not Reported file */
				AlignedReadConvertPrintOutputFormat(&a, rg, fpNotReported, fpNotReportedGZ, outputID, outputFormat, BinaryOutput);
				numNotReported++;
				break;
			case Found:
				/* Print to Output file */
				AlignedReadConvertPrintOutputFormat(&a, rg, fpOut, fpOutGZ, outputID, outputFormat, BinaryOutput);
				numReported++;
				break;
			case ContigAb:
				if(contigAbPaired == 1) {
					/* Print to Contig Abnormalities file */
					AlignedReadConvertPrintOutputFormat(&a, rg, fpContigAb, fpContigAbGZ, outputID, outputFormat, BinaryOutput);
					numContigAb++;
				}
				break;
			case Unpaired:
				if(unpaired == 1) {
					/* Print to Unpaired file */
					AlignedReadConvertPrintOutputFormat(&a, rg, fpUnpaired, fpUnpairedGZ, outputID, outputFormat, BinaryOutput);
					numUnpaired++;
				}
				break;
			case Inversion:
				if(inversionsPaired == 1) {
					/* Print to Inversions file */
					AlignedReadConvertPrintOutputFormat(&a, rg, fpInversions, fpInversionsGZ, outputID, outputFormat, BinaryOutput);
					numInversions++;
				}
				break;
			default:
				PrintError(FnName,
						"foundType",
						"Could not understand foundType",
						Exit,
						OutOfRange);
				break;
		}

		/* Free memory */
		AlignedReadFree(&a);
		/* Increment counter */
		counter++;
	}
	if(VERBOSE>=0) {
		fprintf(stderr, "\r%lld\n",
				(long long int)counter);
		fprintf(stderr, "Out of %lld reads:\n", (long long int)counter);
		fprintf(stderr, "Found alignments for %lld reads.\n", (long long int)numReported);
		fprintf(stderr, "Could not unambiguously align %lld reads.\n", (long long int)numNotReported);
		if(1==contigAbPaired) {
			fprintf(stderr, "Found %lld paired end reads with contigomosomal abnormalities.\n", (long long int)numContigAb);
		}
		if(1==inversionsPaired) {
			fprintf(stderr, "Found %lld inverted paired end reads.\n", (long long int)numInversions); 
		}
		if(1==unpaired) {
			fprintf(stderr, "Found %lld unpaired paired end reads.\n", (long long int)numUnpaired);
		}
	}

	/* Close output files, if necessary */
	if(BAF == outputFormat) {
		gzclose(fpOutGZ);
		gzclose(fpNotReportedGZ);
		if(inversionsPaired == 1) {
			gzclose(fpInversionsGZ);
		}
		if(contigAbPaired == 1) {
			gzclose(fpContigAbGZ);
		}
		if(unpaired == 1) {
			gzclose(fpUnpairedGZ);
		}
	}
	else {
		fclose(fpOut);
		fclose(fpNotReported);
		if(inversionsPaired == 1) {
			fclose(fpInversions);
		}
		if(contigAbPaired == 1) {
			fclose(fpContigAb);
		}
		if(unpaired == 1) {
			fclose(fpUnpaired);
		}
	}
	/* Close the input file */
	gzclose(fp);
}
