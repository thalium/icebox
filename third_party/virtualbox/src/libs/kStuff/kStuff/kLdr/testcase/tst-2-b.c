#include "tst.h"
const char *g_pszName = "b";

MY_IMPORT(int) FuncA(void);

MY_EXPORT(int) FuncB(void)
{
    return FuncA();
}

