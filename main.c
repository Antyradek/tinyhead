#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <popt.h>
#define VERSION "1.3.0"

#define BUFFER_SIZE 1024

#define EXIT_ERR_ARGS -1
#define EXIT_ERR_READ -2
#define EXIT_ERR_MEM -3

#define DEFAULT_VARNAME "file"

#define VAR_SIZE_TYPE "const size_t"
#define VAR_DATA_TYPE "const unsigned char"

typedef unsigned char byte;

///Print file headers such as include guards and other header includes
void printHeaders(const int usecplusplus)
{
	puts("#pragma once");
	printf("#include <%s>\n", usecplusplus ? "cstddef" : "stddef.h");
}

///Print comment in given style
void printComment(const int javadocStyle, const char* value)
{
	if(value != NULL)
	{
		printf("%s %s %s\n", javadocStyle ? "/**" : "///", value, javadocStyle ? "*/" : "");
	}
}

int main(int argc, char** argv)
{
	//execution context
	char* filename = NULL;
	char* varName = DEFAULT_VARNAME;
	const char* appName = argv[0];
	char* varNameData = "Data";
	char* varNameSize = "DataSize";
	char* dataComment = NULL;
	char* sizeComment = NULL;
	int useCPP = 0;
	int useJavadoc = 0;
	int useSnakeCase = 0;
	int printVersion = 0;
	
	//popt arguments
	struct poptOption optionsArray[] =
	{
		{"variable", 	'v', 	POPT_ARG_STRING, 	&varName, 		0, 	"String to use as genarated variable name", 			"VARNAME"},
		{"filename", 	'f', 	POPT_ARG_STRING, 	&filename, 		0, 	"File to read contents from", 							"FILENAME"},
		{"snakecase", 	's', 	POPT_ARG_NONE, 		&useSnakeCase, 	0, 	"Use snake_case, instead of camelCase", 				NULL},
		{"cplusplus",	'p',	POPT_ARG_NONE,		&useCPP,		0,	"Use C++ header, instead of C one",						NULL},
		{"data-comment",'D',	POPT_ARG_STRING,	&dataComment,	0,	"String to use as a comment above data array",			"DATACOMMENT"},
		{"size-comment",'S',	POPT_ARG_STRING,	&sizeComment, 	0,	"String to use as a comment above size variable", 		"SIZECOMMENT"},
		{"use-javadoc",	'J',	POPT_ARG_NONE,		&useJavadoc,	0,	"Use javadoc-style comments ('/**'), instead of C# ('///')", NULL},
		POPT_AUTOHELP
		{"version",		'\0',	POPT_ARG_NONE,		&printVersion,	0,	"Print version and exit",						NULL},
		POPT_TABLEEND
	};
	
	//initialise popt context
	poptContext optCon = poptGetContext(NULL, argc, (const char**) argv, optionsArray, 0);
	
	//read options (all are parsed via arg in optionsArray)
	if(poptGetNextOpt(optCon) != -1)
	{
		poptPrintHelp(optCon, stderr, 0);
		exit(EXIT_ERR_ARGS);
	}
	
	//in case of version, print it and exit
	if(printVersion)
	{
		fprintf(stderr, "%s %s by Radosław Świątkiewicz\n", appName, VERSION);
		exit(0);
	}
	
	//check if varname is correct
	int len = strlen(varName);
	for(int i = 0; i < len; i++)
	{
		if(!isalnum(varName[i]) && varName[i] != '_')
		{
			fprintf(stderr, "Forbidden characters in variable name.\n");
			exit(EXIT_ERR_ARGS);
		}
	}
	
	//change suffix if snake case is used
	if(useSnakeCase)
	{
		varNameData = "_data";
		varNameSize = "_data_size";
	}
	
	int retVal = 0;
	
	if(filename != NULL)
	{
		//Those are C exceptions with goto
		//open file
		FILE* file = fopen(filename, "rb");
		if(file == NULL)
		{
			perror(appName);
			retVal = EXIT_ERR_READ;
			goto fopenErr;
		}
		
		//this function may return non-zero with empty files and ftell will still work correctly
		fseek(file, 0, SEEK_END);
		
		//get file size
		const long fileSize = ftell(file);
		if(fileSize < 0)
		{
			perror(appName);
			retVal = EXIT_ERR_READ;
			goto ftellErr;
		}
		
		rewind(file);
		
		//allocate buffer
		byte* buffer = malloc(sizeof(byte) * fileSize);
		if(buffer == NULL)
		{
			perror(appName);
			retVal = EXIT_ERR_MEM;
			goto mallocErr;
		}
		
		//read file into memory
		const size_t readBytes = fread(buffer, sizeof(byte), fileSize, file);
		if(ferror(file))
		{
			perror(appName);
			retVal = EXIT_ERR_READ;
			goto freadErr;
		}
		
		//print header
		printHeaders(useCPP);
		printComment(useJavadoc, sizeComment);
		printf(VAR_SIZE_TYPE " %s%s = %lu;\n", varName, varNameSize, fileSize);
		printComment(useJavadoc, dataComment);
		printf(VAR_DATA_TYPE " %s%s[] = {", varName, varNameData);
		
		//create buffer file, to output everything at once
		FILE* bufferFile = tmpfile();
		if(bufferFile == NULL)
		{
			perror(appName);
			retVal = EXIT_ERR_MEM;
			goto bufferFileErr;
		}
		
		//read buffer, print to buffer file
		size_t i = 0;
		if(readBytes > 0)
		{
			for(i = 0; i < readBytes - 1; i++)
			{
				fprintf(bufferFile, "%hhu,", buffer[i]);
			}
			if(i < readBytes)
			{
				fprintf(bufferFile,"%hhu", buffer[i]);
			}
		}
		fprintf(bufferFile, "};\n");
		
		//print buffer file at once
		const long tmpFileSize = ftell(bufferFile);
		rewind(bufferFile);
		
		char* bufferBuffer = malloc(sizeof(char) * tmpFileSize);
		if(bufferBuffer == NULL)
		{
			perror(appName);
			retVal = EXIT_ERR_MEM;
			goto bufferBufferErr;
		}
		
		fread(bufferBuffer, sizeof(char), tmpFileSize, bufferFile);
		if(ferror(bufferFile))
		{
			perror(appName);
			retVal = EXIT_ERR_MEM;
			goto bufferBufferFileErr;
		}
		fputs(bufferBuffer, stdout);
		
	bufferBufferFileErr:
	bufferBufferErr:
		free(bufferBuffer);
	bufferFileErr:
		fclose(bufferFile);
	freadErr:
		free(buffer);
	mallocErr:
	ftellErr:
		fclose(file);
	fopenErr:
		poptFreeContext(optCon);
		return retVal;
	}
	else
	{
		printHeaders(useCPP);
		printComment(useJavadoc, dataComment);
		printf(VAR_DATA_TYPE " %s%s[] = {", varName, varNameData);
		
		byte* buffer = malloc(sizeof(byte) * BUFFER_SIZE);
		if(buffer == NULL)
		{
			perror(appName);
			retVal = EXIT_ERR_MEM;
			goto pipeMallocErr;
		}
		
		size_t fileSize = 0;
		while(true)
		{
			int readBytes = fread(buffer, sizeof(char), BUFFER_SIZE, stdin);
			if(ferror(stdin))
			{
				perror(appName);
				retVal = EXIT_ERR_READ;
				goto pipeFreadError;
			}
			fileSize += readBytes;
			int i = 0;
			
			if(readBytes == BUFFER_SIZE)
			{
				for(i = 0; i < readBytes; i++)
				{
					printf("%hhu,", buffer[i]);
				}
			}
			else if(readBytes != BUFFER_SIZE && feof(stdin))
			{
				for(i = 0; i < readBytes - 1; i++)
				{
					printf("%hhu,", buffer[i]);
				}
				if(i < readBytes)
				{
					printf("%hhu", buffer[i]);
				}
				printf("};\n");
				break;
			}
		}
		printComment(useJavadoc, sizeComment);
		printf(VAR_SIZE_TYPE " %s%s = %lu;\n", varName, varNameSize, fileSize);
		
	pipeFreadError:
		free(buffer);
	pipeMallocErr:
		poptFreeContext(optCon);
		return retVal;
	}
	//execution should never reach here
	poptFreeContext(optCon);
	return 0;
}
