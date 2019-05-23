#include "tst.h"
const char *g_pszName = "a";

MY_IMPORT(int) FuncD(void);

MY_EXPORT(int) FuncA(void)
{
    return FuncD() | (g_pszName[0] == 'a' ? 0x42 : 0x0001);
}

