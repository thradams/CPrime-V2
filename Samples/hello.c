

void defer1() {}
void defer2() {}

int main()
{
    int j;
    try
    {        
        try (int i = 0; i == 1; defer1());
        try (int i2 = 0; i2 == 1; defer2());
        /*B*/
    }
    /*C*/ catch (int meunome)
    {
    }
}
