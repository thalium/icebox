#include "tst.h"
const char *g_pszName = "c";

MY_IMPORT(int) FuncA(void);

MY_EXPORT(int) FuncC(void)
{
    return FuncA();
}

