#include "tst.h"
const char *g_pszName = "d";

MY_IMPORT(int) FuncA(void);

MY_EXPORT(int) FuncD(void)
{
    return FuncA();
}

