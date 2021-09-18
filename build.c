#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <direct.h>
#include <sys/types.h>
#include <sys/stat.h>
//https://docs.microsoft.com/pt-br/cpp/c-runtime-library/reference/mkdir-wmkdir?view=msvc-160
#define mkdir(a, b) _mkdir(a)
#define rmdir _rmdir
#define mkdir(A, B)  _mkdir(A)
#define chdir  _chdir
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

#define HEADER_FILES " cprime_lib.h "

#define SOURCE_FILES " cprime.c " \
                     " cprime_lib.c "


#define VC_DEBUG_OPTIONS   " /Od /MDd /RTC1 "
#define VC_RELEASE_OPTIONS " /GL /Gy /O2 /MD  "
#define VC_COMMON_OPTIONS  " /permissive- /GS /Zc:preprocessor- /std:c17  /W4 /Zc:wchar_t /Zi /Gm- /sdl /Zc:inline /fp:precise /errorReport:prompt  /WX /Zc:forScope /Gd /Oy- /FC /EHsc /nologo /diagnostics:column "


int main()
{

#ifdef WEB

    //first call C:\\Users\\thiago\\Source\\Repos\\emsdk\\emsdk_env.bat
   
    // then
    //>cl -DWEB build.c -o build.exe  & build
    //or directly
    //call emcc "cprime_lib.c" -o "Web\cprime.js" -s WASM=0 -s EXPORTED_FUNCTIONS="['_CompileText']" -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall', 'cwrap']"

    printf("*******************************************************************\n");

    printf("Emscripten\n");

    printf("*******************************************************************\n");

    
    int r = system("emcc "
           SOURCE_FILES
           " -s WASM=0 -s EXPORTED_FUNCTIONS=\"['_CompileText']\" -s EXTRA_EXPORTED_RUNTIME_METHODS=\"['ccall', 'cwrap']\""
           "  -o Web/cprime.js");
    if (r != 0)
    {
        printf("call C:\\Users\\thiago\\Source\\Repos\\emsdk\\emsdk_env.bat\n");
    }
    return;
#endif

#if defined(_WIN32) && defined(_MSC_VER) && !defined(__clang__)

    /*
    * cl build.c & build
    */
    printf("*******************************************************************\n");

    printf("Windows - Microsoft Compiler\n");
    
    printf("*******************************************************************\n");

    if (system("cl "
           SOURCE_FILES
           VC_RELEASE_OPTIONS
           " -D_CRT_SECURE_NO_WARNINGS "
           " -o cprime.exe") != 0) exit(1);

    if (system("cprime "
        SOURCE_FILES
        HEADER_FILES
        " -outDir Out") != 0) exit(1);

    chdir("Out");
    if (system("cl "
        SOURCE_FILES
        VC_RELEASE_OPTIONS
        " -D_CRT_SECURE_NO_WARNINGS "
        " -lKernel32.lib -lUser32.lib -lAdvapi32.lib "
        " -o cprime.exe") != 0) exit(1);

    chdir("..");
    
    /*
    * Compile sample using cprime
    */
    if (system("cprime ./Samples/Hello.c -o ./Samples/hello_out.c") != 0) exit(1);
    
    /*
    * Compile cprime output using cl compiler
    */
    if (system("cl ./Samples/hello_out.c -o ./Samples/hello_out.exe") != 0) exit(1);
    
    /*run the sample*/
    chdir("Samples");
    if (system("hello_out.exe") != 0) exit(1);
    chdir("..");

    if (system("cprime ./Samples/if.c -o ./Samples/if_out.c") != 0) exit(1);
    if (system("cprime ./Samples/polimorphism1.c -o ./Samples/polimorphism_out.c") != 0) exit(1);

#elif defined(_WIN32) && defined(__clang__)
    
    /*
    * clang build.c -o build.exe & build
    */

    printf("*******************************************************************\n");

    printf("Windows - Clang\n");

    printf("*******************************************************************\n");

    system("clang  "
           SOURCE_FILES
           " -D_CRT_SECURE_NO_WARNINGS "
           " -std=c17 "
           " -D_MT "
           " -Xlinker /NODEFAULTLIB "
           " -lucrt.lib -lvcruntime.lib -lmsvcrt.lib "
           " -lKernel32.lib -lUser32.lib -lAdvapi32.lib"
           " -luuid.lib -lWs2_32.lib -lRpcrt4.lib -lBcrypt.lib "
           " -o cprime.exe");
#elif defined(__linux__) && defined(__clang__)
    
    /*
    *  clang  build.c -o build ; ./build
    */
    printf("*******************************************************************\n");

    printf("Linux - Clang\n");
    
    printf("*******************************************************************\n");

    system("clang  "
           SOURCE_FILES
           " -D_CRT_SECURE_NO_WARNINGS "
           " -std=c17 "
           " -Wall "
           " -o cprime");

#elif defined(__linux__) && defined(__GNUC__)
    
    /*
    *  gcc  build.c -o build ; ./build
    */

    printf("Linux - Gcc\n");

    system("gcc  -Wall"
           SOURCE_FILES
           " -DNDEBUG"
           " -o cprime");
#else 
 #error Unknown Platform/Compiler
#endif

}
