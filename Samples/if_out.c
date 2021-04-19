
extern void* malloc(int s);
extern void free(void* p);
void print();

int main()
{
    do
    {
        {char* s1 = malloc(100); if( s1){{
        print();
        {char* s2 = malloc(100); if( s2){{
        print();}  free(s2);}}}  free(s1);}}
    } while(0);

}