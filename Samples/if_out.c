

int main()
{
    
        int i = 0;

        //if normal
        if (i == 0)
        {
        }

        //expressao ; condicao
        i = 1;  if( i == 1)
        {}

        //expressao ; condicao; defer
        i = 1; if( i == 1){
        {} i = 0;}

        //init ; condicao
        {int j = 1; if( j == 1)
        {}}


        //init ; condicao; defer
        {int j = 1; if( j == 1){
        {} j--;}}

}
