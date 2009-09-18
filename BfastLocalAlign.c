#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <config.h>
#include <unistd.h>
#include <time.h>

#include "BLibDefinitions.h"
#include "BError.h"
#include "RGBinary.h"
#include "BLib.h"
#include "RunAligner.h"
#include "BfastLocalAlign.h"

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC, OPTIONAL_GROUP_NAME}.
   */
enum { 
	DescInputFilesTitle, DescRGFileName, DescMatchFileName, DescScoringMatrixFileName, 
	DescAlgoTitle, DescAlignmentType, DescSpace, DescStartReadNum, DescEndReadNum, DescOffsetLength, DescMaxNumMatches, DescAvgMismatchQuality, DescNumThreads, DescQueueLength,
	DescPairedEndOptionsTitle, DescPairedEndLength, DescMirroringType, DescForceMirroring, 
	DescOutputTitle, DescOutputID, DescOutputDir, DescTmpDir, DescTiming, 
	DescMiscTitle, DescHelp
};

static struct argp_option options[] = {
	{0, 0, 0, 0, "=========== Input Files =============================================================", 1},
	{"brgFileName", 'r', "brgFileName", 0, "Specifies the file name of the reference genome file", 1},
	{"matchFileName", 'm', "matchFileName", 0, "Specifies the bfast matches file", 1},
	{"scoringMatrixFileName", 'x', "scoringMatrixFileName", 0, "Specifies the file name storing the scoring matrix", 1},
	{0, 0, 0, 0, "=========== Algorithm Options: (Unless specified, default value = 0) ================", 2},
	{"alignmentType", 'a', "alignmentType", 0, "0: Full alignment 1: mismatches only", 2},
	{"space", 'A', "space", 0, "0: NT space 1: Color space", 2},
	{"startReadNum", 's', "startReadNum", 0, "Specifies the read to begin with (skip the first startReadNum-1 reads)", 2},
	{"endReadNum", 'e', "endReadNum", 0, "Specifies the last read to use (inclusive)", 2},
	{"offsetLength", 'O', "offset", 0, "Specifies the number of bases before and after the match to include in the reference genome", 2},
	{"maxNumMatches", 'M', "maxNumMatches", 0, "Specifies the maximum number of candidates to initiate alignment for a given match", 2},
	{"avgMismatchQuality", 'q', "avgMismatchQuality", 0, "Specifies the average mismatch quality", 2},
	{"numThreads", 'n', "numThreads", 0, "Specifies the number of threads to use (Default 1)", 2},
	{"queueLength", 'Q', "queueLength", 0, "Specifies the number of reads to cache", 2},
	{0, 0, 0, 0, "=========== Paired End Options ======================================================", 3},
	{"pairedEndLength", 'l', "pairedEndLength", 0, "Specifies that if one read of the pair has CALs and the other does not,"
		"\n\t\t\tthis distance will be used to infer the latter read's CALs", 3},
	{"mirroringType", 'L', "mirroringType", 0, "0: No mirroring should occur"
		"\n\t\t\t1: specifies that we assume that the first end is before the second end (5'->3')"
			"\n\t\t\t2: specifies that we assume that the second end is before the first end (5'->3)"
			"\n\t\t\t3: specifies that we mirror CALs in both directions", 3},
	{"forceMirroring", 'f', 0, OPTION_NO_USAGE, "Specifies that we should always mirror CALs using the distance from -l", 3},
	{0, 0, 0, 0, "=========== Output Options ==========================================================", 4},
	{"outputID", 'o', "outputID", 0, "Specifies the name to identify the output files", 4},
	{"outputDir", 'd', "outputDir", 0, "Specifies the output directory for the output files", 4},
	{"tmpDir", 'T', "tmpDir", 0, "Specifies the directory in which to store temporary files", 4},
	{"timing", 't', 0, OPTION_NO_USAGE, "Specifies to output timing information", 4},
	{0, 0, 0, 0, "=========== Miscellaneous Options ===================================================", 5},
	{"Parameters", 'p', 0, OPTION_NO_USAGE, "Print program parameters", 5},
	{"Help", 'h', 0, OPTION_NO_USAGE, "Display usage summary", 5},
	{0, 0, 0, 0, 0, 0}
};

static char OptionString[]=
"a:d:e:l:m:n:o:q:r:s:x:A:E:L:M:O:Q:S:T:bfhpt";

	int
BfastLocalAlign(int argc, char **argv)
{
	struct arguments arguments;
	RGBinary rg;
	time_t startTotalTime = time(NULL);
	time_t endTotalTime;
	time_t startTime;
	time_t endTime;
	int totalReferenceGenomeTime = 0; /* Total time to load and delete the reference genome */
	int totalAlignTime = 0; /* Total time to align the reads */
	int totalFileHandlingTime = 0; /* Total time partitioning and merging matches and alignments respectively */
	int seconds, minutes, hours;
	if(argc>1) {
		/* Set argument defaults. (overriden if user specifies them)  */ 
		BfastLocalAlignAssignDefaultValues(&arguments);

		/* Parse command line args */
		if(BfastLocalAlignGetOptParse(argc, argv, OptionString, &arguments)==0)
		{
			switch(arguments.programMode) {
				case ExecuteGetOptHelp:
					BfastLocalAlignGetOptHelp();
					break;
				case ExecutePrintProgramParameters:
					BfastLocalAlignPrintProgramParameters(stderr, &arguments);
					break;
				case ExecuteProgram:
					if(BfastLocalAlignValidateInputs(&arguments)) {
						fprintf(stderr, "**** Input arguments look good! *****\n");
						fprintf(stderr, BREAK_LINE);
					}
					else {
						PrintError("PrintError",
								NULL,
								"validating command-line inputs",
								Exit,
								InputArguments);

					}
					BfastLocalAlignPrintProgramParameters(stderr, &arguments);
					/* Execute Program */
					startTime = time(NULL);
					RGBinaryReadBinary(&rg,
							arguments.brgFileName);
					/* Unpack */
					/*
					   RGBinaryUnPack(&rg);
					   */
					endTime = time(NULL);
					totalReferenceGenomeTime = endTime - startTime;
					/* Run the aligner */
					RunAligner(&rg,
							arguments.matchFileName,
							arguments.scoringMatrixFileName,
							arguments.alignmentType,
							arguments.bestOnly,
							arguments.space,
							arguments.startReadNum,
							arguments.endReadNum,
							arguments.offsetLength,
							arguments.maxNumMatches,
							arguments.avgMismatchQuality,
							arguments.numThreads,
							arguments.queueLength,
							arguments.usePairedEndLength,
							arguments.pairedEndLength,
							arguments.mirroringType,
							arguments.forceMirroring,
							arguments.outputID,
							arguments.outputDir,
							arguments.tmpDir,
							&totalAlignTime,
							&totalFileHandlingTime);
					/* Free the Reference Genome */
					RGBinaryDelete(&rg);

					if(arguments.timing == 1) {
						/* Output loading reference genome time */
						seconds = totalReferenceGenomeTime;
						hours = seconds/3600;
						seconds -= hours*3600;
						minutes = seconds/60;
						seconds -= minutes*60;
						fprintf(stderr, "Reference Genome loading time took: %d hours, %d minutes and %d seconds.\n",
								hours,
								minutes,
								seconds
							   );

						/* Output aligning time */
						seconds = totalAlignTime;
						hours = seconds/3600;
						seconds -= hours*3600;
						minutes = seconds/60;
						seconds -= minutes*60;
						fprintf(stderr, "Align time took: %d hours, %d minutes and %d seconds.\n",
								hours,
								minutes,
								seconds
							   );

						/* Output file handling time */
						seconds = totalFileHandlingTime;
						hours = seconds/3600;
						seconds -= hours*3600;
						minutes = seconds/60;
						seconds -= minutes*60;
						fprintf(stderr, "File handling time took: %d hours, %d minutes and %d seconds.\n",
								hours,
								minutes,
								seconds
							   );

						/* Output total time */
						endTotalTime = time(NULL);
						seconds = endTotalTime - startTotalTime;
						hours = seconds/3600;
						seconds -= hours*3600;
						minutes = seconds/60;
						seconds -= minutes*60;
						fprintf(stderr, "Total time elapsed: %d hours, %d minutes and %d seconds.\n",
								hours,
								minutes,
								seconds
							   );
					}
					fprintf(stderr, "Terminating successfully!\n");
					fprintf(stderr, "%s", BREAK_LINE);
					break;
				default:
					PrintError("PrintError",
							"programMode",
							"Could not determine program mode",
							Exit,
							OutOfRange);
			}
		}
		else {
			PrintError("PrintError",
					NULL,
					"Could not parse command line argumnets",
					Exit,
					InputArguments);
		}
		/* Free program parameters */
		BfastLocalAlignFreeProgramParameters(&arguments);
	}
	else {
		BfastLocalAlignGetOptHelp();
	}
	return 0;
}

int BfastLocalAlignValidateInputs(struct arguments *args) {

	char *FnName="BfastLocalAlignValidateInputs";

	fprintf(stderr, BREAK_LINE);
	fprintf(stderr, "Checking input parameters supplied by the user ...\n");

	if(args->brgFileName!=0) {
		fprintf(stderr, "Validating brgFileName %s. \n",
				args->brgFileName);
		if(ValidateFileName(args->brgFileName)==0)
			PrintError(FnName, "brgFileName", "Command line argument", Exit, IllegalFileName);
	}

	if(args->matchFileName!=0) {
		fprintf(stderr, "Validating matchFileName path %s. \n",
				args->matchFileName);
		if(ValidateFileName(args->matchFileName)==0)
			PrintError(FnName, "matchFileName", "Command line argument", Exit, IllegalFileName);
	}

	if(args->scoringMatrixFileName!=0) {
		fprintf(stderr, "Validating scoringMatrixFileName path %s. \n",
				args->scoringMatrixFileName);
		if(ValidateFileName(args->scoringMatrixFileName)==0)
			PrintError(FnName, "scoringMatrixFileName", "Command line argument", Exit, IllegalFileName);
	}

	if(args->alignmentType != FullAlignment && args->alignmentType != MismatchesOnly) {
		PrintError(FnName, "alignmentType", "Command line argument", Exit, OutOfRange);
	}

	if(args->bestOnly != BestOnly && args->bestOnly != AllAlignments) {
		PrintError(FnName, "bestOnly", "Command line argument", Exit, OutOfRange);
	}

	if(args->space != NTSpace && args->space != ColorSpace) {
		PrintError(FnName, "space", "Command line argument", Exit, OutOfRange);
	}

	if(args->startReadNum < 0) {
		PrintError(FnName, "startReadNum", "Command line argument", Exit, OutOfRange);
	}

	if(args->endReadNum < 0) {
		PrintError(FnName, "endReadNum", "Command line argument", Exit, OutOfRange);
	}

	if(args->offsetLength < 0) {
		PrintError(FnName, "offsetLength", "Command line argument", Exit, OutOfRange);
	}

	if(args->maxNumMatches < 0) {
		PrintError(FnName, "maxNumMatches", "Command line argument", Exit, OutOfRange);
	}

	if(args->avgMismatchQuality <= 0) {
		PrintError(FnName, "avgMismatchQuality", "Command line argument", Exit, OutOfRange);
	}

	if(args->numThreads<=0) {
		PrintError(FnName, "numThreads", "Command line argument", Exit, OutOfRange);
	} 

	if(args->queueLength<=0) {
		PrintError(FnName, "queueLength", "Command line argument", Exit, OutOfRange);
	}

	if(args->outputID!=0) {
		fprintf(stderr, "Validating outputID %s. \n",
				args->outputID);
		if(ValidateFileName(args->outputID)==0)
			PrintError(FnName, "outputID", "Command line argument", Exit, IllegalFileName);
	}

	if(args->outputDir!=0) {
		fprintf(stderr, "Validating outputDir path %s. \n",
				args->outputDir);
		if(ValidateFileName(args->outputDir)==0)
			PrintError(FnName, "outputDir", "Command line argument", Exit, IllegalFileName);
	}

	if(args->tmpDir!=0) {
		fprintf(stderr, "Validating tmpDir path %s. \n",
				args->tmpDir);
		if(ValidateFileName(args->tmpDir)==0)
			PrintError(FnName, "tmpDir", "Command line argument", Exit, IllegalFileName);
	}

	/* If this does not hold, we have done something wrong internally */
	assert(args->timing == 0 || args->timing == 1);
	assert(args->usePairedEndLength == 0 || args->usePairedEndLength == 1);
	assert(args->forceMirroring == 0 || args->forceMirroring == 1);
	assert(NoMirroring <= args->mirroringType && args->mirroringType <= MirrorBoth);
	if(args->mirroringType != NoMirroring && args->usePairedEndLength == 0) {
		PrintError(FnName, "pairedEndLength", "Must specify a paired end length when using mirroring", Exit, OutOfRange);
	}
	if(args->forceMirroring == 1 && args->usePairedEndLength == 0) {
		PrintError(FnName, "pairedEndLength", "Must specify a paired end length when using force mirroring", Exit, OutOfRange);
	}

	return 1;
}

	void 
BfastLocalAlignAssignDefaultValues(struct arguments *args)
{
	/* Assign default values */

	args->programMode = ExecuteProgram;

	args->brgFileName =
		(char*)malloc(sizeof(DEFAULT_FILENAME));
	assert(args->brgFileName!=0);
	strcpy(args->brgFileName, DEFAULT_FILENAME);

	args->matchFileName =
		(char*)malloc(sizeof(DEFAULT_FILENAME));
	assert(args->matchFileName!=0);
	strcpy(args->matchFileName, DEFAULT_FILENAME);

	args->scoringMatrixFileName=NULL;

	args->alignmentType = FullAlignment;
	args->bestOnly = AllAlignments;
	args->space = NTSpace;
	args->startReadNum=0;
	args->endReadNum=INT_MAX;
	args->offsetLength=0;
	args->maxNumMatches=INT_MAX;
	args->avgMismatchQuality=AVG_MISMATCH_QUALITY;
	args->numThreads = 1;
	args->queueLength = DEFAULT_MATCHES_QUEUE_LENGTH;
	args->usePairedEndLength = 0;
	args->pairedEndLength = 0;
	args->mirroringType = NoMirroring;
	args->forceMirroring = 0;

	args->outputID =
		(char*)malloc(sizeof(DEFAULT_OUTPUT_ID));
	assert(args->outputID!=0);
	strcpy(args->outputID, DEFAULT_OUTPUT_ID);

	args->outputDir =
		(char*)malloc(sizeof(DEFAULT_OUTPUT_DIR));
	assert(args->outputDir!=0);
	strcpy(args->outputDir, DEFAULT_OUTPUT_DIR);

	args->tmpDir =
		(char*)malloc(sizeof(DEFAULT_OUTPUT_DIR));
	assert(args->tmpDir!=0);
	strcpy(args->tmpDir, DEFAULT_OUTPUT_DIR);

	args->timing = 0;

	return;
}

	void 
BfastLocalAlignPrintProgramParameters(FILE* fp, struct arguments *args)
{
	char programmode[3][64] = {"ExecuteGetOptHelp", "ExecuteProgram", "ExecutePrintProgramParameters"};
	char using[2][64] = {"Not using", "Using"};
	fprintf(fp, BREAK_LINE);
	fprintf(fp, "Printing Program Parameters:\n");
	fprintf(fp, "programMode:\t\t\t\t%d\t[%s]\n", args->programMode, programmode[args->programMode]);
	fprintf(fp, "brgFileName:\t\t\t\t%s\n", args->brgFileName);
	fprintf(fp, "matchFileName:\t\t\t\t%s\n", args->matchFileName);
	fprintf(fp, "scoringMatrixFileName:\t\t\t%s\n", args->scoringMatrixFileName);
	fprintf(fp, "alignmentType:\t\t\t\t%d\n", args->alignmentType);
	/*
	   fprintf(fp, "bestOnly:\t\t\t\t%d\n", args->bestOnly);
	   */
	fprintf(fp, "space:\t\t\t\t\t%d\n", args->space);
	fprintf(fp, "startReadNum:\t\t\t\t%d\n", args->startReadNum);
	fprintf(fp, "endReadNum:\t\t\t\t%d\n", args->endReadNum);
	fprintf(fp, "offsetLength:\t\t\t\t%d\n", args->offsetLength);
	fprintf(fp, "maxNumMatches:\t\t\t\t%d\n", args->maxNumMatches);
	fprintf(fp, "avgMismatchQuality:\t\t\t%d\n", args->avgMismatchQuality); 
	fprintf(fp, "numThreads:\t\t\t\t%d\n", args->numThreads);
	fprintf(fp, "queueLength:\t\t\t\t%d\n", args->queueLength);
	fprintf(fp, "pairedEndLength:\t\t\t%d\t[%s]\n", args->pairedEndLength, using[args->usePairedEndLength]);
	fprintf(fp, "mirroringType:\t\t\t\t%d\n", args->mirroringType);
	fprintf(fp, "forceMirroring:\t\t\t\t%d\n", args->forceMirroring);
	fprintf(fp, "outputID:\t\t\t\t%s\n", args->outputID);
	fprintf(fp, "outputDir:\t\t\t\t%s\n", args->outputDir);
	fprintf(fp, "tmpDir:\t\t\t\t\t%s\n", args->tmpDir);
	fprintf(fp, "timing:\t\t\t\t\t%d\n", args->timing);
	fprintf(fp, BREAK_LINE);
	return;

}

/* TODO */
void BfastLocalAlignFreeProgramParameters(struct arguments *args)
{
	free(args->brgFileName);
	args->brgFileName=NULL;
	free(args->matchFileName);
	args->matchFileName=NULL;
	free(args->scoringMatrixFileName);
	args->scoringMatrixFileName=NULL;
	free(args->outputID);
	args->outputID=NULL;
	free(args->outputDir);
	args->outputDir=NULL;
	free(args->tmpDir);
	args->tmpDir=NULL;
}

void
BfastLocalAlignGetOptHelp() {

	struct argp_option *a=options;
	fprintf(stderr, "%s %s\n", "bfast ", PACKAGE_VERSION);
	fprintf(stderr, "\nUsage: bfast localalign [options]\n");
	while((*a).group>0) {
		switch((*a).key) {
			case 0:
				fprintf(stderr, "\n%s\n", (*a).doc); break;
			default:
				if((*a).arg != 0) {
					fprintf(stderr, "-%c\t%12s\t%s\n", (*a).key, (*a).arg, (*a).doc); 
				}
				else {
					fprintf(stderr, "-%c\t%12s\t%s\n", (*a).key, "", (*a).doc); 
				}
				break;
		}
		a++;
	}
	fprintf(stderr, "\n send bugs to %s\n", PACKAGE_BUGREPORT);
	return;
}

	int
BfastLocalAlignGetOptParse(int argc, char** argv, char OptionString[], struct arguments* arguments) 
{
	char key;
	int OptErr=0;
	while((OptErr==0) && ((key = getopt (argc, argv, OptionString)) != -1)) {
		/*
		   fprintf(stderr, "Key is %c and OptErr = %d\n", key, OptErr);
		   */
		switch (key) {
			case 'a':
				arguments->alignmentType=atoi(optarg);break;
				/*
				   case 'b':
				   arguments->bestOnly=atoi(optarg);break;
				   */
			case 'd':
				StringCopyAndReallocate(&arguments->outputDir, optarg);
				/* set the tmp directory to the output director */
				if(strcmp(arguments->tmpDir, DEFAULT_FILENAME)==0) {
					StringCopyAndReallocate(&arguments->tmpDir, optarg);
				}
				break;
			case 'e':
				arguments->endReadNum=atoi(optarg);break;
			case 'f':
				arguments->forceMirroring=1;break;
			case 'h':
				arguments->programMode=ExecuteGetOptHelp; break;
			case 'l':
				arguments->usePairedEndLength=1;
				arguments->pairedEndLength = atoi(optarg);break;
			case 'm':
				StringCopyAndReallocate(&arguments->matchFileName, optarg);
				break;
			case 'n':
				arguments->numThreads=atoi(optarg); break;
			case 'o':
				StringCopyAndReallocate(&arguments->outputID, optarg);
				break;
			case 'p':
				arguments->programMode=ExecutePrintProgramParameters; break;
			case 'q':
				arguments->avgMismatchQuality = atoi(optarg); break;
			case 'r':
				StringCopyAndReallocate(&arguments->brgFileName, optarg);
				break;
			case 's':
				arguments->startReadNum=atoi(optarg);break;
			case 't':
				arguments->timing = 1;break;
			case 'x':
				StringCopyAndReallocate(&arguments->scoringMatrixFileName, optarg);
				break;
			case 'A':
				arguments->space=atoi(optarg);break;
			case 'L':
				arguments->mirroringType=atoi(optarg);break;
			case 'M':
				arguments->maxNumMatches=atoi(optarg);break;
			case 'O':
				arguments->offsetLength=atoi(optarg);break;
			case 'Q':
				arguments->queueLength=atoi(optarg);break;
			case 'T':
				StringCopyAndReallocate(&arguments->tmpDir, optarg);
				break;
			default:
				OptErr=1;
		} /* while */
	} /* switch */
	return OptErr;
}