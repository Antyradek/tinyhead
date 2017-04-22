#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#define BUFFER_SIZE 1024

#define EXIT_ERR_ARGS 1
#define EXIT_ERR_READ 2
#define EXIT_ERR_MEM 2

#define DEFAULT_VARNAME "file"

int main(int argc, char** args)
{
	//read arguments
	char* fileName;
	char* varName = DEFAULT_VARNAME;
	char* appName = args[0];
	char* varNameData = "Data";
    char* varNameSize = "Size";

    bool gaveFileName = false;

	for(int i = 1; i < argc; i++)
	{
		if(strcmp(args[i], "--help") == 0 || strcmp(args[i], "-h") == 0)
        {
            printf("Usage: %s [OPTIONS] [FILENAME]\n\n", appName);
            printf("Encodes given data as array of bytes in C/C++ header file, which can be included into source code and will be put into data memory fragment on execution. Header includes two variables with data pointer and size.\n\n");

            printf("Program reads FILENAME file and prints header to STDOUT. \nIf no FILENAME is given, reads from STDIN until EOF (^D). File can be empty.\n");
            printf("\t-v VARNAME\tDefault variable names are %s%s and %s%s. \n\t\t\tUse this option to change them to VARNAMEData and VARNAMESize.\n", DEFAULT_VARNAME, varNameData, DEFAULT_VARNAME, varNameSize);
            printf("\t-s\t\tUse snake case (my_amazing_file_data) instead of camel case (myAmazingFileData).\n");
            printf("\t-h, --help\tPrint this help screen.\n");
            exit(0);
        }
        else if(strcmp(args[i], "-v") == 0)
        {
            i++;
            if(i >= argc)
            {
                fprintf(stderr, "Missing value after '-v'\n");
                exit(EXIT_ERR_ARGS);
            }
            varName = args[i];
            int varNameLen = strlen(varName);
            for(int x = 0; x < varNameLen; x++)
            {
                if(!isalnum(varName[x]) && varName[x] != '_')
                {
                    fprintf(stderr, "Forbidden characters in variable name.\n");
                    exit(EXIT_ERR_ARGS);
                }
            }
        }
        else if(strcmp(args[i], "-s") == 0)
        {
            varNameData = "_data";
            varNameSize = "_size";
        }
        else
        {
            //read fileName
            fileName = args[i];
            gaveFileName = true;
        }
	}


    int retVal = 0;

    if(gaveFileName)
    {
        //Those are C exceptions with goto
        FILE* file = fopen(fileName, "rb");
        if(file == NULL)
        {
            perror(appName);
            retVal = EXIT_ERR_READ;
            goto fopenErr;
        }

        //this function may return non-zero with empty files and ftell will still work correctly
        fseek(file, 0, SEEK_END);

        const long fileSize = ftell(file);
        if(fileSize < 0)
        {
            perror(appName);
            retVal = EXIT_ERR_READ;
            goto ftellErr;
        }

        rewind(file);

        char* buffer = malloc(sizeof(char) * fileSize);
        if(buffer == NULL)
        {
            perror(appName);
            retVal = EXIT_ERR_MEM;
            goto mallocErr;
        }

        const long readBytes = fread(buffer, sizeof(char), fileSize, file);
        if(ferror(file))
        {
            perror(appName);
            retVal = EXIT_ERR_READ;
            goto freadErr;
        }

        puts("#pragma once");
        printf("const unsigned long %s%s = %ld;\n", varName, varNameSize, fileSize);
        printf("const unsigned char %s%s[] = {", varName, varNameData);

        //TODO optimize to print in parts so that Linus doesn't kill me for priting one byte only
        long i = 0;
        for(i = 0; i < readBytes - 1; i++)
        {
            printf("%d,", buffer[i]);
        }
        if(i < readBytes)
        {
            printf("%d", buffer[i]);
        }
        printf("};\n");

    freadErr:
        free(buffer);
    mallocErr:
    ftellErr:
        fclose(file);
    fopenErr:
        return retVal;

    }
    else
    {
        puts("#pragma once");
        printf("const unsigned char %s%s[] = {", varName, varNameData);

        char* buffer = malloc(sizeof(char) * BUFFER_SIZE);
        //TODO check

        long fileSize = 0;
        while(true)
        {
            int readBytes = fread(buffer, sizeof(char), BUFFER_SIZE, stdin);
			fileSize += readBytes;
			int i = 0;

			if(readBytes == BUFFER_SIZE)
            {
                for(i = 0; i < readBytes; i++)
                {
                    printf("%d,", buffer[i]);
                }
            }
            else if(readBytes != BUFFER_SIZE && feof(stdin))
            {
                for(i = 0; i < readBytes - 1; i++)
                {
                    printf("%d,", buffer[i]);
                }
                if(i < readBytes)
                {
                    printf("%d", buffer[i]);
                }
                printf("};\n");
                break;
            }
            else if(ferror(stdin))
            {
                perror(appName);
                exit(EXIT_ERR_READ);
            }
        }
        free(buffer);

        printf("const unsigned long %s%s = %ld;", varName, varNameSize, fileSize);
    }
	return 0;
}
