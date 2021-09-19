


struct X
{
    int a;
    int b;
    int c;
};
int main()
{
    struct X x = {
      /*1*/ 1 /*2*/, /*3*/
      #ifdef M
      ,
      /*4*/ 2 , /*5*/
      #endif /* M */
    };
      
    

}
