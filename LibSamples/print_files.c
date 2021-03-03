#include "..\cprime_lib.h"
#include <string.h>
#include <stdio.h>

void print_files(const char* fileName)
{
    struct SyntaxTree ast = SYNTAXTREE_INIT;

    if (BuildSyntaxTreeFromFile(fileName, "", &ast))
    {      
        for (int i = 0; i < ast.Files.Size; i++)
        {
            printf("%d '%s'\n", i, ast.Files.pItems[i]->FullPath);
        }        
    }

    SyntaxTree_Destroy(&ast);
}

/*
  Given an input source this program prints all files  
  that have been included for parsing.
*/
int main(int argc, char* argv[])
{
    const char* fileName = argv[1];
    print_files(fileName);
}

