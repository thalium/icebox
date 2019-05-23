#include "tst.h"
const char *g_pszName = "b";

MY_IMPORT(int) FuncD(void);

MY_EXPORT(int) FuncB(void)
{
    return FuncD() | (g_pszName[0] == 'b' ? 0x4200 : 0x0010);
}

