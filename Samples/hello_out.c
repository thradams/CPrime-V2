

#include <stdio.h>
#include <stdlib.h>


struct Box {
    int id /*= 1*/;
};
#define BOX_INIT  {/*.id=*/ 1}



struct Box* New_Box() {
    struct Box* p = malloc(sizeof * p);
    if (p) *p = (struct Box)BOX_INIT;
    return p;
}

void destroy_struct_Box_ptr(struct Box* pBox) {
    printf("dtor Box");
}

void draw_struct_Box_ptr(struct Box* pBox) {
    printf("draw Box\n");
}

struct Circle {
    int id /*= 2*/;
};

void draw_struct_Circle_ptr(struct Circle* pCircle)
{
    printf("draw Circle\n");
}

struct /*<Box | Circle>*/ Shape;
static void Box_Destroy(struct Box* p);
static void Box_Delete(struct Box* p);
static void Circle_Destroy(struct Circle* p);
static void Circle_Delete(struct Circle* p);
static void Shape_Destroy(struct Shape* p);

struct Shape { int id; };
static void Shape_draw(struct Shape* p);



int main()
{
    struct Shape* pShape = (struct Shape*)New_Box();
    if (pShape)
    {
        
        Shape_draw(pShape); { Shape_Destroy(&*pShape);  free((void*)pShape); }
    }    
}




static void Box_Destroy(struct Box* p)
{
    destroy_struct_Box_ptr(p);
}

static void Box_Delete(struct Box* p) {
    if (p) {
        Box_Destroy(p);
        free(p); 
    }
}

static void Circle_Destroy(struct Circle* p)
{
}

static void Circle_Delete(struct Circle* p) {
    if (p) {
        Circle_Destroy(p);
        free(p); 
    }
}

static void Shape_Destroy(struct Shape* p)
{
    switch (p->id)
    {
        case  1:
            Box_Destroy((struct Box*)p);
        break;
        case  2:
            Circle_Destroy((struct Circle*)p);
        break;
        default:
        break;
    }
}

static void Shape_draw(struct Shape* p)
{
    switch (p->id)
    {
        case  1:
            draw_struct_Box_ptr((struct Box*)p);
        break;
        case  2:
            draw_struct_Circle_ptr((struct Circle*)p);
        break;
        default:
        break;
    }
}

