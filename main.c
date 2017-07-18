#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#define BUFFER_SIZE 1024

#define EXIT_ERR_ARGS 1
#define EXIT_ERR_READ 2
#define EXIT_ERR_MEM 2

#define DEFAULT_VARNAME "file"

typedef unsigned char byte;

int main(int argc, char** args)
{
	//read arguments
	char* fileName;
	const char* varName = DEFAULT_VARNAME;
	const char* appName = args[0];
	char* varNameData = "Data";
    char* varNameSize = "DataSize";

    bool gaveFileName = false;
	
	//read arguments
	int currOption = -1;
	while((currOption = getopt(argc, args, "v:sf:h")) != -1)
	{
		switch(currOption)
		{
			case 'v':
				//optarg is extern
				varName = optarg;
				int len = strlen(varName);
				for(int i = 0; i < len; i++)
				{
					if(!isalnum(varName[i]) && varName[i] != '_')
					{
						fprintf(stderr, "Forbidden characters in variable name.\n");
						exit(EXIT_ERR_ARGS);
					}
				}
				break;
				
			case 's':
				varNameData = "_data";
				varNameSize = "_data_size";
				break;
				
			case 'f':
				fileName = optarg; 
				gaveFileName = true;
				break;
				
			case '?':
			case 'h':
				printf("Usage: %s [OPTIONS]\n\n", appName);
				printf("Encodes given data as array of bytes in C/C++ header file, which can be included into source code and will be put into data memory fragment on execution. Header includes two variables with data pointer and size.\n\n");
				
				printf("Program reads FILENAME file and prints header to STDOUT. \nIf no FILENAME is given, reads from STDIN until EOF (^D). File can be empty.\n");
				printf("\t-v VARNAME\tDefault variable names are const unsigned char %s%s[] and const size_t %s%s. \n\t\t\tUse this option to change them to VARNAME%s[] and VARNAME%s.\n", DEFAULT_VARNAME, varNameData, DEFAULT_VARNAME, varNameSize, varNameData, varNameSize);
				printf("\t-s\t\tUse snake case (my_amazing_file_data) instead of camel case (myAmazingFileData).\n");
				printf("\t-f FILENAME\tFile to read contents from, if not given, reads from STDIN.\n");
				printf("\t-h, --help\tPrint this help screen.\n");
				exit(0);
				break;
		}
	}

	

    int retVal = 0;

    if(gaveFileName)
    {
        //Those are C exceptions with goto
        //open file
        FILE* file = fopen(fileName, "rb");
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
        puts("#pragma once");
        printf("const size_t %s%s = %lu;\n", varName, varNameSize, fileSize);
        printf("const unsigned char %s%s[] = {", varName, varNameData);

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
        for(i = 0; i < readBytes - 1; i++)
        {
            fprintf(bufferFile, "%hhu,", buffer[i]);
        }
        if(i < readBytes)
        {
            fprintf(bufferFile,"%hhu", buffer[i]);
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
        return retVal;
    }
    else
    {
        puts("#pragma once");
        printf("const unsigned char %s%s[] = {", varName, varNameData);

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
        printf("const size_t %s%s = %lu;\n", varName, varNameSize, fileSize);

	pipeFreadError:
        free(buffer);
	pipeMallocErr:
		return retVal;
    }
	return 0;
}
