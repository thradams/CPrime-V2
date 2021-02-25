# cprime

C' (C Prime) is a front end compiler with preprocessor written in C with no external dependencies.

I am using it for:
 - Create a transpiler, extending the C language with new features and generating C code.
  
   See it online http://thradams.com/web2/cprime.html

It also can be used for:
- Static analisys
- Refactoring tool
- Other language parsing (doc generation etc)
- Formatter
- Add a backend? (webassembly for instance?)
- Reflection
-

## Using the front end

I am working the make it simple to use but there is no documentation yet.
Basically I need to write the documentation and samples to use the syntax abstract tree. 

To use the library you only need two files: cprime_lib.c and cprime_lib.h

CPrime can keep all tokens and compiler phases and this make possible
to rebuild a source code completely.


## cprime extensions

See http://thradams.com/clang.htm

See it online http://thradams.com/web2/cprime.html

The file cprime.c is the command line program for the compiler.

This repository can be used to send sugestions for the cprime extensions.

See help on the command line app.

```

Syntax: cprime [options] [file ...]

Examples: cprime hello.c
          cprime -config config.h hello.c
          cprime -config config.h hello.c -o hello.c
          cprime -config config.h -P hello.c
          cprime -E hello.c
          cprime -P hello.c
          cprime -A hello.c

struct PrintCodeOptions:
-config FILE                          Configuration file.
-outDir                               Set the directory for output.
-help                                 Print this message.
-o FILE                               Sets ouput file name.
-E                                    Preprocess to console.
-P                                    Preprocess to file.
-cx                                   Generate C'.
-ca                                   Generated C annotated
-removeComments                       Remove comments from output
                                      -outDir can define build output
```

## Road Map

 - Create samples about how to use the API.
 - Suport with zero config microsoft C extensions and include headers when compiling inside Visual Studio Command prompt.
   
   Today include dir needs to be set inside config.txt and some microsoft extensions are suported using macros.
 
 
 
