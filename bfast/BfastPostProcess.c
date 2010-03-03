#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <config.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

#include "BError.h"
#include "BLibDefinitions.h"
#include "BLib.h"
#include "RunPostProcess.h"
#include "BfastPostProcess.h"

/*
   OPTIONS.  Field 1 in ARGP.
   Order of fields: {NAME, KEY, ARG, FLAGS, DOC, OPTIONAL_GROUP_NAME}.
   */
enum { 
	DescInputFilesTitle, DescFastaFileName, DescInputFileName, 
	DescAlgoTitle, DescAlgorithm, DescPairedEndInfer, DescNumThreads, DescQueueLength, 
	DescOutputTitle, DescOutputFormat, DescOutputID, DescRGFileName, DescTiming,
	DescMiscTitle, DescParameters, DescHelp
};

static struct argp_option options[] = {
	{0, 0, 0, 0, "=========== Input Files =============================================================", 1},
	{"fastaFileName", 'f', "fastaFileName", 0, "Specifies the file name of the FASTA reference genome", 1},
	{"alignFileName", 'i', "alignFileName", 0, "Specifies the input file from the balign program", 1},
	{0, 0, 0, 0, "=========== Algorithm Options =======================================================", 2},
	{"algorithm", 'a', "algorithm", 0, "Specifies the algorithm to choose the alignment for each end" 
		"\n\t\t\t  of the read:"
			"\n\t\t\t  0: No filtering will occur."
			"\n\t\t\t  1: All alignments that pass the filters will be outputted"
			"\n\t\t\t  2: Only consider reads that have been aligned uniquely"
			"\n\t\t\t  3: Choose uniquely the alignment with the best score"
			"\n\t\t\t  4: Choose all alignments with the best score",
		2},
	{"pairedEndInfer", 'P', 0, OPTION_NO_USAGE, "Specifies to break ties when one end of a paired end read by"
		"\n\t\t\t  estimating the insert size distribution.  This works only"
			"\n\t\t\t  if the other end is mapped uniquely (using -a 3).", 2},
	{"numThreads", 'n', "numThreads", 0, "Specifies the number of threads to use (Default 1)", 2},
	{"queueLength", 'Q', "queueLength", 0, "Specifies the number of reads to cache", 2},
	{0, 0, 0, 0, "=========== Output Options ==========================================================", 3},
	{"unmappedFileName", 'u', "unmappedFileName", 0, "Dump unmapped reads including all their alignments"
		"\n\t\t\t  into this file (always BAF format)", 3},
	{"outputFormat", 'O', "outputFormat", 0, "Specifies the output format 0: BAF 1: MAF 2: GFF 3: SAM", 3},
	{"outputID", 'o', "outputID", 0, "Specifies output ID to prepend to the read name (SAM only)", 3},
	{"RGFileName", 'r', "RGFileName", 0, "Specifies to add the RG in the specified file to the SAM"
		"\n\t\t\t  header and updates the RG tag (and LB/PU tags if present) in"
		"\n\t\t\t  the reads (SAM only)", 3},
	{"timing", 't', 0, OPTION_NO_USAGE, "Specifies to output timing information", 3},
	{0, 0, 0, 0, "=========== Miscellaneous Options ===================================================", 4},
	{"Parameters", 'p', 0, OPTION_NO_USAGE, "Print program parameters", 4},
	{"Help", 'h', 0, OPTION_NO_USAGE, "Display usage summary", 4},
	{0, 0, 0, 0, 0, 0}
};

static char OptionString[]=
"a:i:f:n:o:r:u:O:Q:hptP";

	int
BfastPostProcess(int argc, char **argv)
{
	struct arguments arguments;
	time_t startTime = time(NULL);
	time_t endTime;
	RGBinary rg;
	char *readGroup=NULL;

	if(argc>1) {
		/* Set argument defaults. (overriden if user specifies them)  */ 
		BfastPostProcessAssignDefaultValues(&arguments);

		/* Parse command line args */
		if(BfastPostProcessGetOptParse(argc, argv, OptionString, &arguments)==0)
		{
			switch(arguments.programMode) {
				case ExecuteGetOptHelp:
					BfastPostProcessGetOptHelp();
					break;
				case ExecutePrintProgramParameters:
					BfastPostProcessPrintProgramParameters(stderr, &arguments);
					break;
				case ExecuteProgram:
					if(BfastPostProcessValidateInputs(&arguments)) {
						if(0 <= VERBOSE) {
							fprintf(stderr, "Input arguments look good!\n");
							fprintf(stderr, BREAK_LINE);
						}
					}
					else {
						PrintError("PrintError", NULL, "validating command-line inputs", Exit, InputArguments);
					}
					BfastPostProcessPrintProgramParameters(stderr, &arguments);
					/* Execute program */
					if(BAF != arguments.outputFormat) {
						/* Read binary */
						RGBinaryReadBinary(&rg,
								NTSpace,
								arguments.fastaFileName);
						if(SAM == arguments.outputFormat && NULL != arguments.RGFileName) {
							readGroup = ReadInReadGroup(arguments.RGFileName);
						}
					}
					ReadInputFilterAndOutput(&rg,
							arguments.alignFileName,
							arguments.algorithm,
							arguments.pairedEndInfer,
							arguments.numThreads,
							arguments.queueLength,
							arguments.outputFormat,
							arguments.outputID,
							readGroup,
							arguments.unmappedFileName,
							stdout);
					if(BAF != arguments.outputFormat) {
						/* Free rg binary */
						RGBinaryDelete(&rg);
						if(SAM == arguments.outputFormat && NULL != arguments.RGFileName) {
							free(readGroup);
						}
					}
					if(arguments.timing == 1) {
						/* Get the time information */
						endTime = time(NULL);
						int seconds = endTime - startTime;
						int hours = seconds/3600;
						seconds -= hours*3600;
						int minutes = seconds/60;
						seconds -= minutes*60;
						if(0 <= VERBOSE) {
							fprintf(stderr, "Total time elapsed: %d hours, %d minutes and %d seconds.\n",
									hours,
									minutes,
									seconds
								   );
						}
					}
					if(0 <= VERBOSE) {
						fprintf(stderr, "Terminating successfully!\n");
						fprintf(stderr, "%s", BREAK_LINE);
					}
					break;
				default:
					PrintError("PrintError", "programMode", "Could not determine program mode", Exit, OutOfRange);
			}

		}
		else {
			PrintError("PrintError", NULL, "Could not parse command line arguments", Exit, InputArguments);
		}
		/* Free program parameters */
		BfastPostProcessFreeProgramParameters(&arguments);
	}
	else {
		BfastPostProcessGetOptHelp();
	}
	return 0;
}

/* TODO */
int BfastPostProcessValidateInputs(struct arguments *args) {

	char *FnName="BfastPostProcessValidateInputs";

	/* Check if we are piping */
	if(NULL == args->alignFileName) {
		VERBOSE = -1;
	}

	if(0 <= VERBOSE) {
		fprintf(stderr, BREAK_LINE);
		fprintf(stderr, "Checking input parameters supplied by the user ...\n");
	}

	if(args->fastaFileName!=0) {
		if(0 <= VERBOSE) {
			fprintf(stderr, "Validating fastaFileName %s. \n",
					args->fastaFileName);
		}
		if(ValidateFileName(args->fastaFileName)==0)
			PrintError(FnName, "fastaFileName", "Command line argument", Exit, IllegalFileName);	
	}	
	else if(args->outputFormat != BAF) {
		PrintError(FnName, "fastaFileName", "Required command line argument", Exit, IllegalFileName);
	}

	if(args->alignFileName!=0) {		
		if(0 <= VERBOSE) {
			fprintf(stderr, "Validating alignFileName %s. \n", 
					args->alignFileName);
		}
		if(ValidateFileName(args->alignFileName)==0)
			PrintError(FnName, "alignFileName", "Command line argument", Exit, IllegalFileName);	
	}	

	if(args->algorithm < MIN_FILTER || 			
			args->algorithm > MAX_FILTER) {
		PrintError(FnName, "algorithm", "Command line argument", Exit, OutOfRange);	
	}	

	if(args->numThreads <= 0) {
		PrintError(FnName, "numThreads", "Command line argument", Exit, OutOfRange);
	}

	if(args->queueLength<=0) {		
		PrintError(FnName, "queueLength", "Command line argument", Exit, OutOfRange);
	}

	if(!(args->outputFormat == BAF ||				
				args->outputFormat == MAF ||
				args->outputFormat == GFF ||
				args->outputFormat == SAM)) {
		PrintError(FnName, "outputFormat", "Command line argument", Exit, OutOfRange);	
	}	
	if(args->unmappedFileName!=0) {		
		if(0 <= VERBOSE) {
			fprintf(stderr, "Validating unmappedFileName %s. \n", 
					args->unmappedFileName);
		}
		if(ValidateFileName(args->unmappedFileName)==0)
			PrintError(FnName, "unmappedFileName", "Command line argument", Exit, IllegalFileName);	
	}	
	assert(args->timing == 0 || args->timing == 1);
	assert(args->pairedEndInfer == 0 || args->pairedEndInfer == 1);

	if(args->algorithm != BestScore && 1 == args->pairedEndInfer) {
		PrintError(FnName, "pairedEndInfer", "Command line argument can only be used with -a 3", Exit, OutOfRange);
	}
	if(SAM != args->outputFormat && NULL != args->RGFileName) {
		PrintError(FnName, "RGFileName", "Command line argument can only be used when outputting to SAM format", Exit, OutOfRange);
	}

	return 1;
}

/* TODO */
	void 
BfastPostProcessAssignDefaultValues(struct arguments *args)
{
	/* Assign default values */

	args->programMode = ExecuteProgram;

	args->fastaFileName = NULL;
	args->alignFileName = NULL;

	args->algorithm=BestScore;
	args->pairedEndInfer=0;
	args->numThreads=1;
	args->queueLength=DEFAULT_QUEUE_LENGTH;

	args->outputFormat=SAM;
	args->unmappedFileName=NULL;
	args->outputID=NULL;
	args->RGFileName=NULL;

	args->timing = 0;

	return;
}

/* TODO */
	void 
BfastPostProcessPrintProgramParameters(FILE* fp, struct arguments *args)
{
	char algorithm[5][64] = {"[No Filtering]", "[Filtering Only]", "[Unique]", "[Best Score]", "[Best Score All]"};
	char outputType[8][32] = {"[BRG]", "[BIF]", "[BMF]", "[BAF]", "[MAF]", "[GFF]", "[SAM]", "[LastFileType]"};
	if(0 <= VERBOSE) {
		fprintf(fp, BREAK_LINE);
		fprintf(fp, "Printing Program Parameters:\n");
		fprintf(fp, "programMode:\t\t%s\n", PROGRAMMODE(args->programMode));
		fprintf(fp, "fastaFileName:\t\t%s\n", FILEREQUIRED(args->fastaFileName));
		fprintf(fp, "alignFileName:\t\t%s\n", FILESTDIN(args->alignFileName));
		fprintf(fp, "algorithm:\t\t%s\n", algorithm[args->algorithm]);
		fprintf(fp, "pairedEndInfer:\t\t%s\n", INTUSING(args->pairedEndInfer));
		fprintf(fp, "numThreads:\t\t%d\n", args->numThreads);
		fprintf(fp, "queueLength:\t\t%d\n", args->queueLength);
		fprintf(fp, "outputFormat:\t\t%s\n", outputType[args->outputFormat]);
		fprintf(fp, "unmappedFileName:\t%s\n", FILEUSING(args->unmappedFileName));
		fprintf(fp, "outputID:\t\t%s\n", FILEUSING(args->outputID));
		fprintf(fp, "RGFileName:\t\t%s\n", FILEUSING(args->RGFileName));
		fprintf(fp, "timing:\t\t\t%s\n", INTUSING(args->timing));
		fprintf(fp, BREAK_LINE);
	}
	return;
}

/* TODO */
void BfastPostProcessFreeProgramParameters(struct arguments *args)
{
	free(args->fastaFileName);
	args->fastaFileName=NULL;
	free(args->alignFileName);
	args->alignFileName=NULL;
	free(args->unmappedFileName);
	args->unmappedFileName=NULL;
	free(args->outputID);
	args->outputID=NULL;
	free(args->RGFileName);
	args->RGFileName=NULL;
}

/* TODO */
void
BfastPostProcessGetOptHelp() {

	struct argp_option *a=options;
	fprintf(stderr, "\nUsage: bfast postprocess [options]\n");
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
	fprintf(stderr, "\nsend bugs to %s\n", 
			PACKAGE_BUGREPORT);
	return;
}

/* TODO */
	int
BfastPostProcessGetOptParse(int argc, char** argv, char OptionString[], struct arguments* arguments) 
{
	int key;
	int OptErr=0;
	while((OptErr==0) && ((key = getopt (argc, argv, OptionString)) != -1)) {
		/*
		   fprintf(stderr, "Key is %c and OptErr = %d\n", key, OptErr);
		   */
		switch (key) {
			case 'a':
				arguments->algorithm = atoi(optarg);break;
			case 'f':
				arguments->fastaFileName=strdup(optarg);break;
			case 'h':
				arguments->programMode=ExecuteGetOptHelp;break;
			case 'i':
				arguments->alignFileName=strdup(optarg);break;
				break;
			case 'n':
				arguments->numThreads=atoi(optarg); break;
			case 'o':
				arguments->outputID=strdup(optarg);break;
			case 'p':
				arguments->programMode=ExecutePrintProgramParameters;break;
			case 'r':
				arguments->RGFileName=strdup(optarg);break;
			case 't':
				arguments->timing = 1;break;
			case 'u':
				arguments->unmappedFileName=strdup(optarg);break;
			case 'O':
				switch(atoi(optarg)) {
					case 0:
						arguments->outputFormat = BAF;
						break;
					case 1:
						arguments->outputFormat = MAF;
						break;
					case 2:
						arguments->outputFormat = GFF;
						break;
					case 3:
						arguments->outputFormat = SAM;
						break;
					default:
						arguments->outputFormat = -1;
						/* Deal with this when we validate the input parameters */
						break;
				}
				break;
			case 'P':
				arguments->pairedEndInfer=1; break;
			case 'Q':
				arguments->queueLength=atoi(optarg);break;
			default:
				OptErr=1;
		} /* while */
	} /* switch */
	return OptErr;
}
