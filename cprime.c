
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
    printf("\n");
    printf("Options:\n");
    printf("-outDir                               Set the directory for output.\n");
    printf("-help                                 Print this message.\n");
    printf("-o FILE                               Sets ouput file name.\n");
    printf("-E                                    Preprocess to console.\n");
    printf("-P                                    Preprocess to file.\n");
    printf("-I                                    Include dir.\n");
    printf("-D                                    -DMACRO -DM=1\n");
    printf("-std=c99 -std=c11 -std=c17            C standard input'.\n");    
    printf("-std=c23 -std=c2x                     '.\n");
    
    printf("-ostd=c99 -ostd=c11 -ostd=c17         C standard output'.\n");
    printf("-ostd=c23 -ostd=c2x                   '.\n");
    
    printf("-removeComments                       Remove comments from output\n");    
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
                options->bOutputPreprocessor = true;
            }
            else if (option[1] == 'E')
            {
                options->bOutputPreprocessor = true;
            }
            else if (strcmp(option, "-help") == 0)
            {
                PrintHelp();
                return;
            }
            else if (strcmp(option, "-std=c99") == 0)
            {
                options->InputLanguage = LanguageStandard_C99;
            }
            else if (strcmp(option, "-std=c11") == 0)
            {
                options->InputLanguage = LanguageStandard_C11;
            }
            else if (strcmp(option, "-std=c17") == 0)
            {
                options->InputLanguage = LanguageStandard_C17;
            }
            else if (strcmp(option, "-std=c23") == 0)
            {
                options->InputLanguage = LanguageStandard_C23;
            }
            else if (strcmp(option, "-std=c2x") == 0)
            {
                options->InputLanguage = LanguageStandard_C_EXPERIMENTAL;                
            }
            else if (strcmp(option, "-ostd=c99") == 0)
            {
                options->Target = LanguageStandard_C99;
            }
            else if (strcmp(option, "-ostd=c11") == 0)
            {
                options->Target = LanguageStandard_C11;
            }
            else if (strcmp(option, "-ostd=c17") == 0)
            {
                options->Target = LanguageStandard_C17;
            }
            else if (strcmp(option, "-ostd=c23") == 0)
            {
                options->Target = LanguageStandard_C23;
            }
            else if (strcmp(option, "-ostd=c2x") == 0)
            {
                options->Target = LanguageStandard_C_EXPERIMENTAL;
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
    options.Target = LanguageStandard_C99;
    options.bIncludeComments = true;

    GetOptions(argc, argv, &options);




    if (!Compile(&options))
    {
        //exit(1);
    }



#ifdef WIN32
    _CrtDumpMemoryLeaks();//lint !e523 !e522
#endif

    return 0;
}

