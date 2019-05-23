#include "tst.h"
const char *g_pszName = "b";

MY_IMPORT(int) FuncD(void);

MY_EXPORT(int) FuncB(void)
{
    return FuncD();
}

