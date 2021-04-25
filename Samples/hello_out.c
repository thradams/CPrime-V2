
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
    
    /*try-block*/ {  {
        /*try*/ if (!(Parse1(error) == 0)) {  goto _catch_label1;}
        /*try*/ if (!(Parse2(error) == 0)) {  goto _catch_label1;}
        /*try*/ if (!(Parse3(error) == 0)) {  goto _catch_label1;}
    }_catch_label1:;} /*end try-block*/
    return error->code;
}

int main()
{
    struct error error = { 0 };

    

    /*try-block*/ {  {
        /*try*/ if (!(F(&error) == 0)) {  goto _catch_label1;}
        
    }
    /*catch*/ goto _exit_try_label1;_catch_label1:;
    {
        printf("parsing error : %s", error.message);
    }_exit_try_label1:;} /*end try-block*/

    printf("continuation...\n");
}

