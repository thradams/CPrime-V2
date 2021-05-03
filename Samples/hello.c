

int main()
{
    int i;
    try
    {
        
        defer i = 1;

        {            
            defer i = 2;
        }

        throw;

        if (int i = 0; i < 10; i = 3)
        {
        }
    }
    catch
    {
    }

   

}

