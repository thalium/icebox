#include "tst.h"
const char *g_pszName = "c";

MY_IMPORT(int) FuncD(void);

MY_EXPORT(int) FuncC(void)
{
    return FuncD() | (g_pszName[0] == 'c' ? 0x420000 : 0x0100);
}

