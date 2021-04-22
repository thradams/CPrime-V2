
//#include <stdio.h>

struct error {
    int code;
    char message[100];
};

int Parse1(struct error* error) {
    return error->code;
}

int Parse2(struct error* error) {
    return error->code;
}

int Parse3(struct error* error) {
    return error->code;
}

int F(struct error* error)
{
    try {
        try(Parse1(error) == 0);
        try(Parse2(error) == 0);
        try(Parse3(error) == 0);
    }
    return error->code;
}

int main()
{
    struct error error = { 0 };

    try {
        try(F(&error) == 0);
    }
    catch (int errorcode)
    {
        printf("parsing error : %s", error.message);
    }

    printf("continuation...\n");
}

