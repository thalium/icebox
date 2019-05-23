#include "tst.h"
const char *g_pszName = "d";

MY_EXPORT(int) FuncD(void)
{
    return g_pszName[0] == 'd' ? 0x42000000 : 0x1000;
}

