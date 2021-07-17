int main()
{
    void (*F)(void);
    F = (int() (void)){ return 1; };

    F = (int(*) (void)){ 0 };
}

