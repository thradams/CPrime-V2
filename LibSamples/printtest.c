#include "..\cprime_lib.h"
#include <string.h>
#include <stdio.h>

void print_test(const char* fileName)
{
    struct SyntaxTree ast = SYNTAXTREE_INIT;

    if (BuildSyntaxTreeFromFile(fileName, "", &ast))
    {
        printf("void TestAll();\n");
        printf("{\n");
        for (int i = 0; i < ast.Declarations.Size; i++)
        {
            struct AnyDeclaration* pAnyDeclaration = ast.Declarations.pItems[i];
            struct Declaration* pDeclaration = pAnyDeclaration->Type == Declaration_ID ? (struct Declaration*)pAnyDeclaration : 0;

            if (pDeclaration &&
                Declaration_Is_FunctionDefinition(pDeclaration))
            {
                const char* funcName = pDeclaration->InitDeclaratorList.pHead->pDeclarator->pDirectDeclarator->Identifier;
                if (strstr(funcName, "_Test") != 0)
                {
                    printf("  %s();\n", funcName);
                }
            }
        }
        printf("}\n");
    }

    SyntaxTree_Destroy(&ast);
}


int main(int argc, char* argv[])
{
    const char* fileName = argv[1];
    print_test(fileName, enumName);
}

