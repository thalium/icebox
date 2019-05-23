#include "tst.h"

MY_IMPORT(int) FuncA(void);
MY_IMPORT(int) FuncB(void);
MY_IMPORT(int) FuncC(void);
MY_IMPORT(int) FuncD(void);

int main()
{
    printf("graph:\n"
           "  tst-2 -> b -> a\n"
           "           c -> a\n"
           "           d -> a\n"
           "           a\n");
    return FuncA() + FuncB() + FuncC() + FuncD();
}
