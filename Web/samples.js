var sample = {};


sample["Default Initialization"] =
    `

struct Point {
  int x = 1;
  int y = 2;
};

struct Line {
  struct Point start;
  struct Point end;
};

struct Point global = {};

int main()
{
  struct Point pt = {};
  struct Line ln = {};
}

`;


sample["Destructor"] =
`
#include <stdlib.h>
#include <stdio.h>

struct Person {
  char* name;
};

void destroy(struct Person* p) overload {
    free(p->name);
    printf("person destructor called");
}

struct House {
   struct Person person;
};

int main()
{
   struct House house = {};
   destroy(house);
}

`;

sample["defer"] =
`
/***********************************************
  defer statement

  statement is called at the end of scope.
  or before jumps to outside the scope.
    
************************************************/

#include <stdlib.h>
#include <stdio.h>

int main()
{
   char* s1 = malloc(1);
   defer { printf("defer1\\n"); free(s1); }

   printf("scope 1\\n");
   {
      char* s2 = malloc(1);    
      defer { printf("defer2\\n"); free(s2); }     
      printf("scope 2\\n");
   }
}

`;

sample["Defer + Destructor"] =
    `
#include <stdlib.h>
#include <stdio.h>

struct Person {
  char* name;
};

void destroy(struct Person* p) overload {
    free(p->name);
    printf("person destructor called");
}

struct House {
   struct Person person;
};

int main()
{
   struct House house = {};
   defer destroy(house);
   printf("some work..\\n");
}
`;


sample["If with initializer (Like C++ 17)"] =
    `
#include <stdio.h>

int main()
{
   if (FILE* f = fopen("f.txt", "r"); f)
   {
     fclose(f);
   }
}

`;



sample["If with initializer and defer"] =
`
#include <stdio.h>

int main()
{
   if (FILE* f = fopen("f.txt", "r"); f; fclose(f))
   {
     
   }
}
`;


sample["If+defer and return"] =
    `
/*
 *  This sample shows why we need a temporary copy before return
 */

#include <stdlib.h>

inline void* move(void** p) {
  void* t = *p;
  *p = 0;
  return t;
}

void * F() {
  if (void* p = malloc(1); p; free(p)) 
  {
    if (void* p2 = malloc(2); p2; free(p2)) 
    {
      return move(&p2); /*p2 is moved and not destroyed anymore*/
    }
  }
}
`

sample["try-block statement"] =
    `
/*******************************************
  try compound-statement catch (optional) compound-statement

  throw can only be used inside try blocks. 
  It always LOCAL jump. No need for stack unwinding.

********************************************/
#include <stdio.h>

int main()
{
    try {
        for (int i =0 ; i < 10; i++)
        {
          for (int j =0 ; j < 10; j++)
          {
            if (i == 5 && j ==5)
              throw;
          }
        }
    }
     
    printf("continuation...\\n");
}
`;


sample["try-block-catch statement "] =
`
#include <stdio.h>

int F1(){ return 1; }

int main()
{
    int error = 0;
    try {
        if (F1() != 0)
        {
          error = 1; 
          throw;
        }
    }
    catch 
    {
      printf("catch error %d\\n", error);
    }
  
    printf("continuation...\\n");
}
`;



sample["try with defer"] =
`
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
    
int main()
{
    int error = 0;
    try {
        FILE* f = fopen(\"input.txt\", \"r\"); 
        if (f == NULL) {
            error = errno; 
            throw;
        }
        defer fclose(f);
        printf("using f..\\n");
    }
    catch
    {
            printf("error %d\\n", error);
    }
    printf("continuation...\\n");
}      
`;

sample["throw inside if defer expression"] =
`
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
    
int main()
{
    int error = 0;
    try {
        if (FILE* f = fopen("input.txt", "r"); f; fclose(f))
        {            
            printf("using f..\\n");
            throw;
        }            
    }
    catch
    {
       printf("error %d\\n", error);
    }
    printf("continuation...\\n");
} 
`;

sample["Polimorphism"] =
`

#include <stdio.h>
#include <stdlib.h>


struct Box {
    int id = 1;
};

struct Box* New_Box() {
  struct Box* p = malloc(sizeof * p);
  if (p) *p = (struct Box) {};
  return p;
}

void destroy(struct Box* pBox) overload {
      printf("dtor Box");
}

void draw(struct Box* pBox) overload {
    printf("draw Box\\n");
}

struct Circle {
    int id = 2;
};

void draw(struct Circle* pCircle) overload
{
    printf("draw Circle\\n");
}

struct <Box | Circle> Shape;

int main()
{
    struct Shape * pShape = (struct Shape *)New_Box();

    draw(pShape);

    destroy(*pShape);
    free((void*)pShape);
}

`;



sample["Function Literal"] =
    `
#include <stdio.h>

void Run(void (*callback)(void*), void* data) {
  callback(data);
}

int main()
{  
  Run((void (void* data)){
    printf("first\\n");
    Run((void (void* data) ){
      printf("second\\n");
    }, 0);     
  }, 0);
}
`;

