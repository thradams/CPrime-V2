
extern void* malloc(int s);
extern void free(void* p);
void print();

int main()
{
    {
        try (char* s1 = malloc(100); s1;  free(s1));
        print();
        try (char* s2 = malloc(100); s2;  free(s2));
        print();
    }
}