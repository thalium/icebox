
extern void MyIprtPrintf(const char *pszFormat, ...) __attribute__((__iprt_format__(1,2)));
extern void foo(void);

void foo(void)
{
    MyIprtPrintf(0);
    MyIprtPrintf("%RX32 %d %s\n", 10, 42, "string");
}

