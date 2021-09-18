struct X {
    const char* s1;
    const char* s2;
};

#define X_INIT {0}
#define B "teste"

int main()
{
    struct X x;
    x =  (struct X )  {  "a",  "b" /*AQUI*/};
        
}