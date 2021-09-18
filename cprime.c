
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>  
#include "cprime_lib.h"


void GetFullDirS(const char* fileName, char* out);
bool FileExists(const char* fullPath);
void GetFullPathS(const char* fileName, char* out);

#define CONFIG_FILE_NAME "config.txt"
#define EXECUTABLE_NAME "cprime"

void PrintHelp()
{
    printf("Syntax: " EXECUTABLE_NAME " [options] [file ...]\n");
    printf("\n");
    printf("Examples: " EXECUTABLE_NAME " hello.c\n");
    printf("          " EXECUTABLE_NAME " -config config.h hello.c\n");
    printf("          " EXECUTABLE_NAME " -config config.h hello.c -o hello.c\n");
    printf("          " EXECUTABLE_NAME " -config config.h -P hello.c\n");
    printf("          " EXECUTABLE_NAME " -E hello.c\n");
    printf("          " EXECUTABLE_NAME " -P hello.c\n");
    printf("          " EXECUTABLE_NAME " -A hello.c\n");
    printf("\n");
    printf("struct PrintCodeOptions:\n");
    printf("-config FILE                          Configuration file.\n");
    printf("-outDir                               Set the directory for output.\n");
    printf("-help                                 Print this message.\n");
    printf("-o FILE                               Sets ouput file name.\n");
    printf("-E                                    Preprocess to console.\n");
    printf("-P                                    Preprocess to file.\n");
    printf("-cx                                   Generate C'.\n");
    printf("-ca                                   Generated C annotated\n");
    printf("-removeComments                       Remove comments from output\n");
    printf("                                      -outDir can define build output\n");
}



int main(int argc, char* argv[])
{
    printf("\n");
    printf("C' Version " __DATE__ "\n");
    printf("https://github.com/thradams/CPrime\n\n");

#ifdef _DEBUG
    //AllTests();
#endif
    if (argc < 2)
    {
        PrintHelp();
        return 1;
    }

    char outputDirFullPath[250] = { 0 };

    char cxconfigFileFullPath[250];
    GetFullDirS(argv[0], cxconfigFileFullPath);
    strcat(cxconfigFileFullPath, CONFIG_FILE_NAME);

    
    if (FileExists(cxconfigFileFullPath))
    {
        printf("using config file %s\n", cxconfigFileFullPath);
    }
    else
    {
        cxconfigFileFullPath[0] = 0;
    }


    char outputFileFullPath[250] = { 0 };
    char inputFileFullPath[250] = { 0 };

    struct OutputOptions options = OUTPUTOPTIONS_INIT;
    options.Target = CompilerTarget_C99;

    bool bPrintPreprocessedToFile = false;
    bool bPrintPreprocessedToConsole = false;

    for (int i = 1; i < argc; i++)
    {
        const char* option = argv[i];
        if (strcmp(option, "-P") == 0)
        {
            options.Target = CompilerTarget_Preprocessed;
            bPrintPreprocessedToFile = true;
        }
        else if (strcmp(option, "-E") == 0)
        {
            options.Target = CompilerTarget_Preprocessed;
            bPrintPreprocessedToConsole = true;
        }
        else if (strcmp(option, "-r") == 0)
        {
            bPrintPreprocessedToConsole = true;
        }
        else if (strcmp(option, "-help") == 0)
        {
            PrintHelp();
            return 0;
        }
        else if (strcmp(option, "-cx") == 0)
        {
            options.Target = CompilerTarget_CXX;
        }
        else if (strcmp(option, "-ca") == 0)
        {
            options.Target = CompilerTarget_C99;
        }
        else if (strcmp(option, "-removeComments") == 0)
        {
            options.bIncludeComments = false;
        }
        else if (strcmp(option, "-pr") == 0)
        {
            options.Target = CompilerTarget_Preprocessed;
        }
        else if (strcmp(option, "-outDir") == 0)
        {
            if (i + 1 < argc)
            {
                GetFullPathS(argv[i + 1], outputDirFullPath);
                i++;
            }
            else
            {
                printf("missing file\n");
                break;
            }
        }
        else if (strcmp(option, "-config") == 0)
        {
            if (i + 1 < argc)
            {
                GetFullPathS(argv[i + 1], cxconfigFileFullPath);
                i++;
            }
            else
            {
                printf("missing file\n");
                break;
            }
        }
        else if (strcmp(option, "-o") == 0)
        {
            if (i + 1 < argc)
            {
                GetFullPathS(argv[i + 1], outputFileFullPath);
                i++;
            }
            else
            {
                printf("missing file\n");
            }
        }
        else
        {
            GetFullPathS(argv[i], inputFileFullPath);
        }
    }

    if (bPrintPreprocessedToFile)
    {
        PrintPreprocessedToFile(inputFileFullPath, cxconfigFileFullPath);
    }
    else if (bPrintPreprocessedToConsole)
    {
        PrintPreprocessedToConsole(inputFileFullPath, cxconfigFileFullPath);
    }
    else
    {
        if (!Compile(cxconfigFileFullPath, inputFileFullPath, outputFileFullPath, &options))
        {
            exit(1);
        }    
    }


#ifdef WIN32
    //_CrtMemDumpAllObjectsSince(NULL);
#endif

    return 0;
}

