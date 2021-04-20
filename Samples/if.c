
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
        try (hr = F1(); SUCCEEDED(hr));
        try (hr = F2(); SUCCEEDED(hr));
        try (hr = F3(); SUCCEEDED(hr));
    }

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
        if  (i = 1; i == 1)
        {}

        //expressao ; condicao; defer
        if (i = 1; i == 1; i = 0)
        {}

        //init ; condicao
        if (int j = 1; j == 1)
        {}


        //init ; condicao; defer
        if (int j = 1; j == 1; j--)
        {}

}
