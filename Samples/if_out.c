
extern void* malloc(int s);
extern void free(void* p);
void print();

//#include <stdio.h>

typedef long HRESULT;
#define SUCCEEDED(hr) ((hr) >= 0)
#define FAILED(hr) ((hr) < 0)

#define S_OK    0
#define E_FAIL  -1


HRESULT F1() {
    return S_OK;
}
HRESULT F2() {
    return E_FAIL;
}
HRESULT F3() {
    return S_OK;
}

int main()
{
    HRESULT hr = E_FAIL;

    do {
        hr = F1(); if( SUCCEEDED(hr)){
        hr = F2(); if( SUCCEEDED(hr)){
        hr = F3(); if( SUCCEEDED(hr)){}}}
    } while(0);

    if (FAILED(hr))
        printf("error %d\n", hr);

    printf("continuation...\n");
}



void F()
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
