

#include <stdio.h>
#include <stdlib.h>


struct Box {
    int id = 1;
};

struct Box* New_Box() {
    struct Box* p = malloc(sizeof * p);
    if (p) *p = (struct Box){};
    return p;
}

void destroy(struct Box* pBox) overload {
    printf("dtor Box");
}

void draw(struct Box* pBox) overload {
    printf("draw Box\n");
}

struct Circle {
    int id = 2;
};

void draw(struct Circle* pCircle) overload
{
    printf("draw Circle\n");
}

struct <Box | Circle> Shape;

int main()
{
    struct Shape* pShape = (struct Shape*)New_Box();
    if (pShape)
    {
        defer { destroy(*pShape);  free((void*)pShape); }
        draw(pShape);
    }    
}

