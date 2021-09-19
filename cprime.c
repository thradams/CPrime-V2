
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
    printf("Examples: " EXECUTABLE_NAME " hello.c -o hello_out.c\n");
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


void GetOptions(int argc, char* argv[], struct CompilerOptions* options)
{
    //https://docs.microsoft.com/pt-br/cpp/build/reference/compiler-options-listed-by-category?view=msvc-160
    for (int i = 1; i < argc; i++)
    {
        const char* option = argv[i];
        if (option[0] == '-' || option[0] == '/')
        {
            if (option[1] == 'P')
            {
                options->Target = CompilerTarget_Preprocessed;
            }
            else if (option[1] == 'E')
            {
                options->Target = CompilerTarget_Preprocessed;
            }
            else if (strcmp(option, "-help") == 0)
            {
                PrintHelp();
                return;
            }
            else if (strcmp(option, "-cx") == 0)
            {
                options->Target = CompilerTarget_CXX;
            }
            else if (strcmp(option, "-ca") == 0)
            {
                options->Target = CompilerTarget_C99;
            }
            else if (strcmp(option, "-removeComments") == 0)
            {
                options->bIncludeComments = false;
            }
            else if (strcmp(option, "-outDir") == 0)
            {
                if (i + 1 < argc)
                {
                    GetFullPathS(argv[i + 1], options->outputDir);
                    i++;
                }
                else
                {
                    printf("missing file\n");
                    break;
                }
            }
            else if (option[1] == 'o')
            {
                if (i + 1 < argc)
                {
                    GetFullPathS(argv[i + 1], options->output);
                    i++;
                }
                else
                {
                    printf("missing file\n");
                }
            }
            else if (option[1] == 'I')
            {
                char fileName[300];
                GetFullPathS(argv[i] + 2, fileName);
                StrArray_Push(&options->IncludeDir, fileName);
            }
            else if (option[1] == 'D')
            {
                StrArray_Push(&options->Defines, argv[i] + 2);
            }
        }
        else
        {
            char fileName[300];
            GetFullPathS(argv[i], fileName);
            StrArray_Push(&options->SourceFiles, fileName);
        }
    }
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


    struct CompilerOptions options = OUTPUTOPTIONS_INIT;
    options.Target = CompilerTarget_C99;
    options.bIncludeComments = true;

    GetOptions(argc, argv, &options);




    if (!Compile(&options))
    {
        exit(1);
    }



#ifdef WIN32
    //_CrtMemDumpAllObjectsSince(NULL);
#endif

    return 0;
}

