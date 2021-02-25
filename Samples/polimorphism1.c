
#include <stdio.h>
#include <stdlib.h>

struct Box {
    int id = 1;
};

void draw(struct Box* pBox) overload {
    printf("Box");
}

void box_serialize(struct Box* pBox) {
    printf("Box");
}

struct Circle {
    int id = 2;
};

void draw(struct Circle* pCircle) overload
{
    printf("Circle");
}

void circle_serialize(struct Circle* pBox) {
    printf("Box");
}

struct Data {
    int id = 3;
};

void data_serialize(struct Data* pData)
{
    printf("Data");
}


struct <Box | Circle> Shape;


void serialize(struct Circle* pCircle) overload {
    circle_serialize(pCircle);
}

void serialize(struct Box* pCircle) overload {
    box_serialize(pCircle);
}

void serialize(struct Data* pData) overload {
    data_serialize(pData);
}

struct <Shape | Data> Serializable;


int main()
{
    struct Serializable* auto pSerializable = new (struct Box);
    serialize(pSerializable);
    destroy(pSerializable);
}

