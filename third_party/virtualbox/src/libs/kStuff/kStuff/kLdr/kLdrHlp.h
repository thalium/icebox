
int     kldrHlpGetEnv(const char *pszVar, char *pszVal, KSIZE cchVal);
int     kldrHlpGetEnvUZ(const char *pszVar, KSIZE *pcb);

void    kldrHlpExit(int rc);
void    kldrHlpSleep(unsigned cMillies);

char   *kldrHlpInt2Ascii(char *psz, KSIZE cch, long lVal, unsigned iBase);

