

void defer1() {}
void defer2() {}

int main()
{
    int j;
    
    /*try-block*/ { int meunome = 0;
    {        
        /*try*/ int i = 0; if (!( i == 1)) {  meunome = 1; goto _catch_label1;}
        /*try*/ int i2 = 0; if (!( i2 == 1)) {  meunome = 1; defer1(); goto _catch_label1;} defer2(); defer1();
        /*B*/
    }
    /*C*/  /*catch*/ goto _exit_try_label1; _catch_label1:;
    {
    }_exit_try_label1:;} /*end try-block*/
}
