
#include <stdio.h>
#include <stdlib.h>

struct Box {
    int id /*= 1*/;
};

void draw_struct_Box_ptr(struct Box* pBox) {
    printf("Box");
}

void box_serialize(struct Box* pBox) {
    printf("Box");
}

struct Circle {
    int id /*= 2*/;
};

void draw_struct_Circle_ptr(struct Circle* pCircle)
{
    printf("Circle");
}

void circle_serialize(struct Circle* pBox) {
    printf("Box");
}

struct Data {
    int id /*= 3*/;
};

void data_serialize(struct Data* pData)
{
    printf("Data");
}


struct /*<Box | Circle>*/ Shape;


void serialize_struct_Circle_ptr(struct Circle* pCircle) {
    circle_serialize(pCircle);
}

void serialize_struct_Box_ptr(struct Box* pCircle) {
    box_serialize(pCircle);
}

void serialize_struct_Data_ptr(struct Data* pData) {
    data_serialize(pData);
}

struct /*<Shape | Data>*/ Serializable;
static void* mallocinit(int size, void* value);
#ifndef NEW
#define NEW(...) mallocinit(sizeof(__VA_ARGS__), & __VA_ARGS__)
#endif
#define BOX_INIT  {/*.id=*/ 1}

struct Serializable { int id; };
static void Serializable_serialize(struct Serializable* p);
static void Box_Destroy(struct Box* p);
static void Box_Delete(struct Box* p);
static void Circle_Destroy(struct Circle* p);
static void Circle_Delete(struct Circle* p);
static void Data_Destroy(struct Data* p);
static void Data_Delete(struct Data* p);
static void Serializable_Destroy(struct Serializable* p);
static void Serializable_Delete(struct Serializable* p);




int main()
{
    struct Serializable* /*auto*/ pSerializable =NEW((struct Box)BOX_INIT);
    Serializable_serialize(pSerializable);
    Serializable_Delete(pSerializable);
}




static void* mallocinit(int size, void* value) {
   void* memcpy(void* restrict s1, const void* restrict s2, size_t n);
   void* p = malloc(size);
   if (p) memcpy(p, value, size);
   return p;
}

static void Serializable_serialize(struct Serializable* p)
{
    switch (p->id)
    {
        case  1:
            serialize_struct_Box_ptr((struct Box*)p);
        break;
        case  2:
            serialize_struct_Circle_ptr((struct Circle*)p);
        break;
        case  3:
            serialize_struct_Data_ptr((struct Data*)p);
        break;
        default:
        break;
    }
}

static void Box_Destroy(struct Box* p)
{
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

static void Data_Destroy(struct Data* p)
{
}

static void Data_Delete(struct Data* p) {
    if (p) {
        Data_Destroy(p);
        free(p); 
    }
}

static void Serializable_Destroy(struct Serializable* p)
{
    switch (p->id)
    {
        case  1:
            Box_Destroy((struct Box*)p);
        break;
        case  2:
            Circle_Destroy((struct Circle*)p);
        break;
        case  3:
            Data_Destroy((struct Data*)p);
        break;
        default:
        break;
    }
}

static void Serializable_Delete(struct Serializable* p) {
    if (p) {
        Serializable_Destroy(p);
        free(p); 
    }
}

