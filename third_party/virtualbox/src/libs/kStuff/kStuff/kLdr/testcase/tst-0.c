#include "tst.h"

MY_IMPORT(int) FuncA(void);
MY_IMPORT(int) FuncB(void);
MY_IMPORT(int) FuncC(void);

int main()
{
    unsigned u;
    u = FuncA() | FuncB() | FuncC();
    return u == 0x42424242 ? 0 : 1;
}

