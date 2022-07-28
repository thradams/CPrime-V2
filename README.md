# cprime v2

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

See LibSamples.
  *  printenum.c - This program searches for an enum by name and generates a function to convert from enum to string.
  *  printtest.c - This program searches for function names that have Test in their name and generates a function that calls. This could be useful to create unit test.

The AST of prime is not so abstract. This is because I keep each space, comment and original formating and information and this requires a syntax tree less abstract that
represents almost directly each part of the grammar.

## cprime extensions

See http://thradams.com/clang.htm

See it online http://thradams.com/web2/cprime.html

The file cprime.c is the command line program for the compiler.

This repository can be used to send sugestions for the cprime extensions.

See use the command line compiler.

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

 - Improve samples and they can be used as documentation.
 
 
 
 
