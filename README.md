# cprime

C front end compiler (with preprocessor) written in C with no external dependencies.

I am using it for:
 - Create a transpiler, extending the C language with new features and generating C code.
  
   See it online http://thradams.com/web2/cprime.html

It also can be used for:
- Static analisys
- Refactoring tool
- Other language parsing (doc generation etc)
- Formatter
- Add a backend? (webassembly for instance?)

## Using the fron tend
I am working the make it simple to use (a good and documented API). 

You only need two files to use the library. cprime_lib.c and cprime_lib.h

CPrime can keep all tokens and compiler phases and this make possible
to rebuild a source code completely.

## cprime extensions

See http://thradams.com/clang.htm

See it online http://thradams.com/web2/cprime.html

The file cprime.c is the command line program for the compiler.

This repository can be used to send sugestions for the cprime extensions.
