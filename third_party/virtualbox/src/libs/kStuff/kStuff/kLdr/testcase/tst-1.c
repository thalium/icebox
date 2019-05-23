#include "tst.h"
#include <stdio.h>

MY_IMPORT(int) FuncA(void);
MY_IMPORT(int) FuncB(void);
MY_IMPORT(int) FuncC(void);

int main()
{
    printf("graph:\n"
           "  tst-1 -> a -> d\n"
           "           b -> d\n"
           "           c -> d\n");
    return FuncA() + FuncB() + FuncC();
}
