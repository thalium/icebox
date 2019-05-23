typedef PVOID WINAPI FN_EncodePointer( PVOID Ptr );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_EncodePointer( PVOID Ptr )
{
    static FN_EncodePointer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EncodePointer", &g_Kernel32);
    return pfn( Ptr );
}

typedef PVOID WINAPI FN_DecodePointer( PVOID Ptr );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_DecodePointer( PVOID Ptr )
{
    static FN_DecodePointer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DecodePointer", &g_Kernel32);
    return pfn( Ptr );
}

typedef PVOID WINAPI FN_EncodeSystemPointer( PVOID Ptr );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_EncodeSystemPointer( PVOID Ptr )
{
    static FN_EncodeSystemPointer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EncodeSystemPointer", &g_Kernel32);
    return pfn( Ptr );
}

typedef PVOID WINAPI FN_DecodeSystemPointer( PVOID Ptr );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_DecodeSystemPointer( PVOID Ptr )
{
    static FN_DecodeSystemPointer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DecodeSystemPointer", &g_Kernel32);
    return pfn( Ptr );
}

typedef DWORD WINAPI FN_GetFreeSpace( UINT a);
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetFreeSpace( UINT a)
{
    static FN_GetFreeSpace *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFreeSpace", &g_Kernel32);
    return pfn( a);
}

typedef LONG WINAPI FN_InterlockedIncrement( LONG volatile * lpAddend );
__declspec(dllexport) LONG WINAPI kPrf2Wrap_InterlockedIncrement( LONG volatile * lpAddend )
{
    static FN_InterlockedIncrement *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InterlockedIncrement", &g_Kernel32);
    return pfn( lpAddend );
}

typedef LONG WINAPI FN_InterlockedDecrement( LONG volatile * lpAddend );
__declspec(dllexport) LONG WINAPI kPrf2Wrap_InterlockedDecrement( LONG volatile * lpAddend )
{
    static FN_InterlockedDecrement *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InterlockedDecrement", &g_Kernel32);
    return pfn( lpAddend );
}

typedef LONG WINAPI FN_InterlockedExchange( LONG volatile * Target, LONG Value );
__declspec(dllexport) LONG WINAPI kPrf2Wrap_InterlockedExchange( LONG volatile * Target, LONG Value )
{
    static FN_InterlockedExchange *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InterlockedExchange", &g_Kernel32);
    return pfn( Target, Value );
}

typedef LONG WINAPI FN_InterlockedExchangeAdd( LONG volatile * Addend, LONG Value );
__declspec(dllexport) LONG WINAPI kPrf2Wrap_InterlockedExchangeAdd( LONG volatile * Addend, LONG Value )
{
    static FN_InterlockedExchangeAdd *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InterlockedExchangeAdd", &g_Kernel32);
    return pfn( Addend, Value );
}

typedef LONG WINAPI FN_InterlockedCompareExchange( LONG volatile * Destination, LONG Exchange, LONG Comperand );
__declspec(dllexport) LONG WINAPI kPrf2Wrap_InterlockedCompareExchange( LONG volatile * Destination, LONG Exchange, LONG Comperand )
{
    static FN_InterlockedCompareExchange *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InterlockedCompareExchange", &g_Kernel32);
    return pfn( Destination, Exchange, Comperand );
}

typedef LONGLONG WINAPI FN_InterlockedCompareExchange64( LONGLONG volatile * Destination, LONGLONG Exchange, LONGLONG Comperand );
__declspec(dllexport) LONGLONG WINAPI kPrf2Wrap_InterlockedCompareExchange64( LONGLONG volatile * Destination, LONGLONG Exchange, LONGLONG Comperand )
{
    static FN_InterlockedCompareExchange64 *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InterlockedCompareExchange64", &g_Kernel32);
    return pfn( Destination, Exchange, Comperand );
}

typedef VOID WINAPI FN_InitializeSListHead( PSLIST_HEADER ListHead );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_InitializeSListHead( PSLIST_HEADER ListHead )
{
    static FN_InitializeSListHead *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InitializeSListHead", &g_Kernel32);
    pfn( ListHead );
}

typedef PSLIST_ENTRY WINAPI FN_InterlockedPopEntrySList( PSLIST_HEADER ListHead );
__declspec(dllexport) PSLIST_ENTRY WINAPI kPrf2Wrap_InterlockedPopEntrySList( PSLIST_HEADER ListHead )
{
    static FN_InterlockedPopEntrySList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InterlockedPopEntrySList", &g_Kernel32);
    return pfn( ListHead );
}

typedef PSLIST_ENTRY WINAPI FN_InterlockedPushEntrySList( PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry );
__declspec(dllexport) PSLIST_ENTRY WINAPI kPrf2Wrap_InterlockedPushEntrySList( PSLIST_HEADER ListHead, PSLIST_ENTRY ListEntry )
{
    static FN_InterlockedPushEntrySList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InterlockedPushEntrySList", &g_Kernel32);
    return pfn( ListHead, ListEntry );
}

typedef PSLIST_ENTRY WINAPI FN_InterlockedFlushSList( PSLIST_HEADER ListHead );
__declspec(dllexport) PSLIST_ENTRY WINAPI kPrf2Wrap_InterlockedFlushSList( PSLIST_HEADER ListHead )
{
    static FN_InterlockedFlushSList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InterlockedFlushSList", &g_Kernel32);
    return pfn( ListHead );
}

typedef USHORT WINAPI FN_QueryDepthSList( PSLIST_HEADER ListHead );
__declspec(dllexport) USHORT WINAPI kPrf2Wrap_QueryDepthSList( PSLIST_HEADER ListHead )
{
    static FN_QueryDepthSList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueryDepthSList", &g_Kernel32);
    return pfn( ListHead );
}

typedef BOOL WINAPI FN_FreeResource( HGLOBAL hResData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FreeResource( HGLOBAL hResData )
{
    static FN_FreeResource *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FreeResource", &g_Kernel32);
    return pfn( hResData );
}

typedef LPVOID WINAPI FN_LockResource( HGLOBAL hResData );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_LockResource( HGLOBAL hResData )
{
    static FN_LockResource *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LockResource", &g_Kernel32);
    return pfn( hResData );
}

typedef BOOL WINAPI FN_FreeLibrary( HMODULE hLibModule );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FreeLibrary( HMODULE hLibModule )
{
    static FN_FreeLibrary *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FreeLibrary", &g_Kernel32);
    return pfn( hLibModule );
}

typedef VOID WINAPI FN_FreeLibraryAndExitThread( HMODULE hLibModule, DWORD dwExitCode );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_FreeLibraryAndExitThread( HMODULE hLibModule, DWORD dwExitCode )
{
    static FN_FreeLibraryAndExitThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FreeLibraryAndExitThread", &g_Kernel32);
    pfn( hLibModule, dwExitCode );
}

typedef BOOL WINAPI FN_DisableThreadLibraryCalls( HMODULE hLibModule );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DisableThreadLibraryCalls( HMODULE hLibModule )
{
    static FN_DisableThreadLibraryCalls *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DisableThreadLibraryCalls", &g_Kernel32);
    return pfn( hLibModule );
}

typedef FARPROC WINAPI FN_GetProcAddress( HMODULE hModule, LPCSTR lpProcName );
__declspec(dllexport) FARPROC WINAPI kPrf2Wrap_GetProcAddress( HMODULE hModule, LPCSTR lpProcName )
{
    static FN_GetProcAddress *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcAddress", &g_Kernel32);
    return pfn( hModule, lpProcName );
}

typedef DWORD WINAPI FN_GetVersion( VOID );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetVersion( VOID )
{
    static FN_GetVersion *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVersion", &g_Kernel32);
    return pfn ();
}

typedef HGLOBAL WINAPI FN_GlobalAlloc( UINT uFlags, SIZE_T dwBytes );
__declspec(dllexport) HGLOBAL WINAPI kPrf2Wrap_GlobalAlloc( UINT uFlags, SIZE_T dwBytes )
{
    static FN_GlobalAlloc *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalAlloc", &g_Kernel32);
    return pfn( uFlags, dwBytes );
}

typedef HGLOBAL WINAPI FN_GlobalReAlloc( HGLOBAL hMem, SIZE_T dwBytes, UINT uFlags );
__declspec(dllexport) HGLOBAL WINAPI kPrf2Wrap_GlobalReAlloc( HGLOBAL hMem, SIZE_T dwBytes, UINT uFlags )
{
    static FN_GlobalReAlloc *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalReAlloc", &g_Kernel32);
    return pfn( hMem, dwBytes, uFlags );
}

typedef SIZE_T WINAPI FN_GlobalSize( HGLOBAL hMem );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_GlobalSize( HGLOBAL hMem )
{
    static FN_GlobalSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalSize", &g_Kernel32);
    return pfn( hMem );
}

typedef UINT WINAPI FN_GlobalFlags( HGLOBAL hMem );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GlobalFlags( HGLOBAL hMem )
{
    static FN_GlobalFlags *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalFlags", &g_Kernel32);
    return pfn( hMem );
}

typedef LPVOID WINAPI FN_GlobalLock( HGLOBAL hMem );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_GlobalLock( HGLOBAL hMem )
{
    static FN_GlobalLock *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalLock", &g_Kernel32);
    return pfn( hMem );
}

typedef HGLOBAL WINAPI FN_GlobalHandle( LPCVOID pMem );
__declspec(dllexport) HGLOBAL WINAPI kPrf2Wrap_GlobalHandle( LPCVOID pMem )
{
    static FN_GlobalHandle *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalHandle", &g_Kernel32);
    return pfn( pMem );
}

typedef BOOL WINAPI FN_GlobalUnlock( HGLOBAL hMem );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GlobalUnlock( HGLOBAL hMem )
{
    static FN_GlobalUnlock *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalUnlock", &g_Kernel32);
    return pfn( hMem );
}

typedef HGLOBAL WINAPI FN_GlobalFree( HGLOBAL hMem );
__declspec(dllexport) HGLOBAL WINAPI kPrf2Wrap_GlobalFree( HGLOBAL hMem )
{
    static FN_GlobalFree *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalFree", &g_Kernel32);
    return pfn( hMem );
}

typedef SIZE_T WINAPI FN_GlobalCompact( DWORD dwMinFree );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_GlobalCompact( DWORD dwMinFree )
{
    static FN_GlobalCompact *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalCompact", &g_Kernel32);
    return pfn( dwMinFree );
}

typedef VOID WINAPI FN_GlobalFix( HGLOBAL hMem );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GlobalFix( HGLOBAL hMem )
{
    static FN_GlobalFix *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalFix", &g_Kernel32);
    pfn( hMem );
}

typedef VOID WINAPI FN_GlobalUnfix( HGLOBAL hMem );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GlobalUnfix( HGLOBAL hMem )
{
    static FN_GlobalUnfix *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalUnfix", &g_Kernel32);
    pfn( hMem );
}

typedef LPVOID WINAPI FN_GlobalWire( HGLOBAL hMem );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_GlobalWire( HGLOBAL hMem )
{
    static FN_GlobalWire *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalWire", &g_Kernel32);
    return pfn( hMem );
}

typedef BOOL WINAPI FN_GlobalUnWire( HGLOBAL hMem );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GlobalUnWire( HGLOBAL hMem )
{
    static FN_GlobalUnWire *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalUnWire", &g_Kernel32);
    return pfn( hMem );
}

typedef VOID WINAPI FN_GlobalMemoryStatus( LPMEMORYSTATUS lpBuffer );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GlobalMemoryStatus( LPMEMORYSTATUS lpBuffer )
{
    static FN_GlobalMemoryStatus *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalMemoryStatus", &g_Kernel32);
    pfn( lpBuffer );
}

typedef BOOL WINAPI FN_GlobalMemoryStatusEx( LPMEMORYSTATUSEX lpBuffer );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GlobalMemoryStatusEx( LPMEMORYSTATUSEX lpBuffer )
{
    static FN_GlobalMemoryStatusEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalMemoryStatusEx", &g_Kernel32);
    return pfn( lpBuffer );
}

typedef HLOCAL WINAPI FN_LocalAlloc( UINT uFlags, SIZE_T uBytes );
__declspec(dllexport) HLOCAL WINAPI kPrf2Wrap_LocalAlloc( UINT uFlags, SIZE_T uBytes )
{
    static FN_LocalAlloc *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalAlloc", &g_Kernel32);
    return pfn( uFlags, uBytes );
}

typedef HLOCAL WINAPI FN_LocalReAlloc( HLOCAL hMem, SIZE_T uBytes, UINT uFlags );
__declspec(dllexport) HLOCAL WINAPI kPrf2Wrap_LocalReAlloc( HLOCAL hMem, SIZE_T uBytes, UINT uFlags )
{
    static FN_LocalReAlloc *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalReAlloc", &g_Kernel32);
    return pfn( hMem, uBytes, uFlags );
}

typedef LPVOID WINAPI FN_LocalLock( HLOCAL hMem );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_LocalLock( HLOCAL hMem )
{
    static FN_LocalLock *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalLock", &g_Kernel32);
    return pfn( hMem );
}

typedef HLOCAL WINAPI FN_LocalHandle( LPCVOID pMem );
__declspec(dllexport) HLOCAL WINAPI kPrf2Wrap_LocalHandle( LPCVOID pMem )
{
    static FN_LocalHandle *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalHandle", &g_Kernel32);
    return pfn( pMem );
}

typedef BOOL WINAPI FN_LocalUnlock( HLOCAL hMem );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LocalUnlock( HLOCAL hMem )
{
    static FN_LocalUnlock *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalUnlock", &g_Kernel32);
    return pfn( hMem );
}

typedef SIZE_T WINAPI FN_LocalSize( HLOCAL hMem );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_LocalSize( HLOCAL hMem )
{
    static FN_LocalSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalSize", &g_Kernel32);
    return pfn( hMem );
}

typedef UINT WINAPI FN_LocalFlags( HLOCAL hMem );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_LocalFlags( HLOCAL hMem )
{
    static FN_LocalFlags *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalFlags", &g_Kernel32);
    return pfn( hMem );
}

typedef HLOCAL WINAPI FN_LocalFree( HLOCAL hMem );
__declspec(dllexport) HLOCAL WINAPI kPrf2Wrap_LocalFree( HLOCAL hMem )
{
    static FN_LocalFree *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalFree", &g_Kernel32);
    return pfn( hMem );
}

typedef SIZE_T WINAPI FN_LocalShrink( HLOCAL hMem, UINT cbNewSize );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_LocalShrink( HLOCAL hMem, UINT cbNewSize )
{
    static FN_LocalShrink *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalShrink", &g_Kernel32);
    return pfn( hMem, cbNewSize );
}

typedef SIZE_T WINAPI FN_LocalCompact( UINT uMinFree );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_LocalCompact( UINT uMinFree )
{
    static FN_LocalCompact *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalCompact", &g_Kernel32);
    return pfn( uMinFree );
}

typedef BOOL WINAPI FN_FlushInstructionCache( HANDLE hProcess, LPCVOID lpBaseAddress, SIZE_T dwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FlushInstructionCache( HANDLE hProcess, LPCVOID lpBaseAddress, SIZE_T dwSize )
{
    static FN_FlushInstructionCache *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FlushInstructionCache", &g_Kernel32);
    return pfn( hProcess, lpBaseAddress, dwSize );
}

typedef LPVOID WINAPI FN_VirtualAlloc( LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_VirtualAlloc( LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect )
{
    static FN_VirtualAlloc *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualAlloc", &g_Kernel32);
    return pfn( lpAddress, dwSize, flAllocationType, flProtect );
}

typedef BOOL WINAPI FN_VirtualFree( LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_VirtualFree( LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType )
{
    static FN_VirtualFree *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualFree", &g_Kernel32);
    return pfn( lpAddress, dwSize, dwFreeType );
}

typedef BOOL WINAPI FN_VirtualProtect( LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_VirtualProtect( LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect )
{
    static FN_VirtualProtect *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualProtect", &g_Kernel32);
    return pfn( lpAddress, dwSize, flNewProtect, lpflOldProtect );
}

typedef SIZE_T WINAPI FN_VirtualQuery( LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_VirtualQuery( LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength )
{
    static FN_VirtualQuery *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualQuery", &g_Kernel32);
    return pfn( lpAddress, lpBuffer, dwLength );
}

typedef LPVOID WINAPI FN_VirtualAllocEx( HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_VirtualAllocEx( HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect )
{
    static FN_VirtualAllocEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualAllocEx", &g_Kernel32);
    return pfn( hProcess, lpAddress, dwSize, flAllocationType, flProtect );
}

typedef UINT WINAPI FN_GetWriteWatch( DWORD dwFlags, PVOID lpBaseAddress, SIZE_T dwRegionSize, PVOID * lpAddresses, ULONG_PTR * lpdwCount, PULONG lpdwGranularity );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetWriteWatch( DWORD dwFlags, PVOID lpBaseAddress, SIZE_T dwRegionSize, PVOID * lpAddresses, ULONG_PTR * lpdwCount, PULONG lpdwGranularity )
{
    static FN_GetWriteWatch *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetWriteWatch", &g_Kernel32);
    return pfn( dwFlags, lpBaseAddress, dwRegionSize, lpAddresses, lpdwCount, lpdwGranularity );
}

typedef UINT WINAPI FN_ResetWriteWatch( LPVOID lpBaseAddress, SIZE_T dwRegionSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_ResetWriteWatch( LPVOID lpBaseAddress, SIZE_T dwRegionSize )
{
    static FN_ResetWriteWatch *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ResetWriteWatch", &g_Kernel32);
    return pfn( lpBaseAddress, dwRegionSize );
}

typedef SIZE_T WINAPI FN_GetLargePageMinimum( VOID );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_GetLargePageMinimum( VOID )
{
    static FN_GetLargePageMinimum *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLargePageMinimum", &g_Kernel32);
    return pfn ();
}

typedef UINT WINAPI FN_EnumSystemFirmwareTables( DWORD FirmwareTableProviderSignature, PVOID pFirmwareTableEnumBuffer, DWORD BufferSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_EnumSystemFirmwareTables( DWORD FirmwareTableProviderSignature, PVOID pFirmwareTableEnumBuffer, DWORD BufferSize )
{
    static FN_EnumSystemFirmwareTables *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumSystemFirmwareTables", &g_Kernel32);
    return pfn( FirmwareTableProviderSignature, pFirmwareTableEnumBuffer, BufferSize );
}

typedef UINT WINAPI FN_GetSystemFirmwareTable( DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID, PVOID pFirmwareTableBuffer, DWORD BufferSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetSystemFirmwareTable( DWORD FirmwareTableProviderSignature, DWORD FirmwareTableID, PVOID pFirmwareTableBuffer, DWORD BufferSize )
{
    static FN_GetSystemFirmwareTable *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemFirmwareTable", &g_Kernel32);
    return pfn( FirmwareTableProviderSignature, FirmwareTableID, pFirmwareTableBuffer, BufferSize );
}

typedef BOOL WINAPI FN_VirtualFreeEx( HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_VirtualFreeEx( HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType )
{
    static FN_VirtualFreeEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualFreeEx", &g_Kernel32);
    return pfn( hProcess, lpAddress, dwSize, dwFreeType );
}

typedef BOOL WINAPI FN_VirtualProtectEx( HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_VirtualProtectEx( HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect )
{
    static FN_VirtualProtectEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualProtectEx", &g_Kernel32);
    return pfn( hProcess, lpAddress, dwSize, flNewProtect, lpflOldProtect );
}

typedef SIZE_T WINAPI FN_VirtualQueryEx( HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_VirtualQueryEx( HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength )
{
    static FN_VirtualQueryEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualQueryEx", &g_Kernel32);
    return pfn( hProcess, lpAddress, lpBuffer, dwLength );
}

typedef HANDLE WINAPI FN_HeapCreate( DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_HeapCreate( DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize )
{
    static FN_HeapCreate *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapCreate", &g_Kernel32);
    return pfn( flOptions, dwInitialSize, dwMaximumSize );
}

typedef BOOL WINAPI FN_HeapDestroy( HANDLE hHeap );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_HeapDestroy( HANDLE hHeap )
{
    static FN_HeapDestroy *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapDestroy", &g_Kernel32);
    return pfn( hHeap );
}

typedef LPVOID WINAPI FN_HeapAlloc( HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_HeapAlloc( HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes )
{
    static FN_HeapAlloc *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapAlloc", &g_Kernel32);
    return pfn( hHeap, dwFlags, dwBytes );
}

typedef LPVOID WINAPI FN_HeapReAlloc( HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_HeapReAlloc( HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes )
{
    static FN_HeapReAlloc *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapReAlloc", &g_Kernel32);
    return pfn( hHeap, dwFlags, lpMem, dwBytes );
}

typedef BOOL WINAPI FN_HeapFree( HANDLE hHeap, DWORD dwFlags, LPVOID lpMem );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_HeapFree( HANDLE hHeap, DWORD dwFlags, LPVOID lpMem )
{
    static FN_HeapFree *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapFree", &g_Kernel32);
    return pfn( hHeap, dwFlags, lpMem );
}

typedef SIZE_T WINAPI FN_HeapSize( HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_HeapSize( HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem )
{
    static FN_HeapSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapSize", &g_Kernel32);
    return pfn( hHeap, dwFlags, lpMem );
}

typedef BOOL WINAPI FN_HeapValidate( HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_HeapValidate( HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem )
{
    static FN_HeapValidate *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapValidate", &g_Kernel32);
    return pfn( hHeap, dwFlags, lpMem );
}

typedef SIZE_T WINAPI FN_HeapCompact( HANDLE hHeap, DWORD dwFlags );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_HeapCompact( HANDLE hHeap, DWORD dwFlags )
{
    static FN_HeapCompact *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapCompact", &g_Kernel32);
    return pfn( hHeap, dwFlags );
}

typedef HANDLE WINAPI FN_GetProcessHeap( VOID );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_GetProcessHeap( VOID )
{
    static FN_GetProcessHeap *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessHeap", &g_Kernel32);
    return pfn ();
}

typedef DWORD WINAPI FN_GetProcessHeaps( DWORD NumberOfHeaps, PHANDLE ProcessHeaps );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetProcessHeaps( DWORD NumberOfHeaps, PHANDLE ProcessHeaps )
{
    static FN_GetProcessHeaps *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessHeaps", &g_Kernel32);
    return pfn( NumberOfHeaps, ProcessHeaps );
}

typedef BOOL WINAPI FN_HeapLock( HANDLE hHeap );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_HeapLock( HANDLE hHeap )
{
    static FN_HeapLock *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapLock", &g_Kernel32);
    return pfn( hHeap );
}

typedef BOOL WINAPI FN_HeapUnlock( HANDLE hHeap );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_HeapUnlock( HANDLE hHeap )
{
    static FN_HeapUnlock *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapUnlock", &g_Kernel32);
    return pfn( hHeap );
}

typedef BOOL WINAPI FN_HeapWalk( HANDLE hHeap, LPPROCESS_HEAP_ENTRY lpEntry );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_HeapWalk( HANDLE hHeap, LPPROCESS_HEAP_ENTRY lpEntry )
{
    static FN_HeapWalk *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapWalk", &g_Kernel32);
    return pfn( hHeap, lpEntry );
}

typedef BOOL WINAPI FN_HeapSetInformation( HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_HeapSetInformation( HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength )
{
    static FN_HeapSetInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapSetInformation", &g_Kernel32);
    return pfn( HeapHandle, HeapInformationClass, HeapInformation, HeapInformationLength );
}

typedef BOOL WINAPI FN_HeapQueryInformation( HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength, PSIZE_T ReturnLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_HeapQueryInformation( HANDLE HeapHandle, HEAP_INFORMATION_CLASS HeapInformationClass, PVOID HeapInformation, SIZE_T HeapInformationLength, PSIZE_T ReturnLength )
{
    static FN_HeapQueryInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "HeapQueryInformation", &g_Kernel32);
    return pfn( HeapHandle, HeapInformationClass, HeapInformation, HeapInformationLength, ReturnLength );
}

typedef BOOL WINAPI FN_GetBinaryTypeA( LPCSTR lpApplicationName, LPDWORD lpBinaryType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetBinaryTypeA( LPCSTR lpApplicationName, LPDWORD lpBinaryType )
{
    static FN_GetBinaryTypeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetBinaryTypeA", &g_Kernel32);
    return pfn( lpApplicationName, lpBinaryType );
}

typedef BOOL WINAPI FN_GetBinaryTypeW( LPCWSTR lpApplicationName, LPDWORD lpBinaryType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetBinaryTypeW( LPCWSTR lpApplicationName, LPDWORD lpBinaryType )
{
    static FN_GetBinaryTypeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetBinaryTypeW", &g_Kernel32);
    return pfn( lpApplicationName, lpBinaryType );
}

typedef DWORD WINAPI FN_GetShortPathNameA( LPCSTR lpszLongPath, LPSTR lpszShortPath, DWORD cchBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetShortPathNameA( LPCSTR lpszLongPath, LPSTR lpszShortPath, DWORD cchBuffer )
{
    static FN_GetShortPathNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetShortPathNameA", &g_Kernel32);
    return pfn( lpszLongPath, lpszShortPath, cchBuffer );
}

typedef DWORD WINAPI FN_GetShortPathNameW( LPCWSTR lpszLongPath, LPWSTR lpszShortPath, DWORD cchBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetShortPathNameW( LPCWSTR lpszLongPath, LPWSTR lpszShortPath, DWORD cchBuffer )
{
    static FN_GetShortPathNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetShortPathNameW", &g_Kernel32);
    return pfn( lpszLongPath, lpszShortPath, cchBuffer );
}

typedef DWORD WINAPI FN_GetLongPathNameA( LPCSTR lpszShortPath, LPSTR lpszLongPath, DWORD cchBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetLongPathNameA( LPCSTR lpszShortPath, LPSTR lpszLongPath, DWORD cchBuffer )
{
    static FN_GetLongPathNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLongPathNameA", &g_Kernel32);
    return pfn( lpszShortPath, lpszLongPath, cchBuffer );
}

typedef DWORD WINAPI FN_GetLongPathNameW( LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetLongPathNameW( LPCWSTR lpszShortPath, LPWSTR lpszLongPath, DWORD cchBuffer )
{
    static FN_GetLongPathNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLongPathNameW", &g_Kernel32);
    return pfn( lpszShortPath, lpszLongPath, cchBuffer );
}

typedef BOOL WINAPI FN_GetProcessAffinityMask( HANDLE hProcess, PDWORD_PTR lpProcessAffinityMask, PDWORD_PTR lpSystemAffinityMask );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetProcessAffinityMask( HANDLE hProcess, PDWORD_PTR lpProcessAffinityMask, PDWORD_PTR lpSystemAffinityMask )
{
    static FN_GetProcessAffinityMask *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessAffinityMask", &g_Kernel32);
    return pfn( hProcess, lpProcessAffinityMask, lpSystemAffinityMask );
}

typedef BOOL WINAPI FN_SetProcessAffinityMask( HANDLE hProcess, DWORD_PTR dwProcessAffinityMask );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetProcessAffinityMask( HANDLE hProcess, DWORD_PTR dwProcessAffinityMask )
{
    static FN_SetProcessAffinityMask *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetProcessAffinityMask", &g_Kernel32);
    return pfn( hProcess, dwProcessAffinityMask );
}

typedef BOOL WINAPI FN_GetProcessHandleCount( HANDLE hProcess, PDWORD pdwHandleCount );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetProcessHandleCount( HANDLE hProcess, PDWORD pdwHandleCount )
{
    static FN_GetProcessHandleCount *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessHandleCount", &g_Kernel32);
    return pfn( hProcess, pdwHandleCount );
}

typedef BOOL WINAPI FN_GetProcessTimes( HANDLE hProcess, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetProcessTimes( HANDLE hProcess, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime )
{
    static FN_GetProcessTimes *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessTimes", &g_Kernel32);
    return pfn( hProcess, lpCreationTime, lpExitTime, lpKernelTime, lpUserTime );
}

typedef BOOL WINAPI FN_GetProcessIoCounters( HANDLE hProcess, PIO_COUNTERS lpIoCounters );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetProcessIoCounters( HANDLE hProcess, PIO_COUNTERS lpIoCounters )
{
    static FN_GetProcessIoCounters *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessIoCounters", &g_Kernel32);
    return pfn( hProcess, lpIoCounters );
}

typedef BOOL WINAPI FN_GetProcessWorkingSetSize( HANDLE hProcess, PSIZE_T lpMinimumWorkingSetSize, PSIZE_T lpMaximumWorkingSetSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetProcessWorkingSetSize( HANDLE hProcess, PSIZE_T lpMinimumWorkingSetSize, PSIZE_T lpMaximumWorkingSetSize )
{
    static FN_GetProcessWorkingSetSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessWorkingSetSize", &g_Kernel32);
    return pfn( hProcess, lpMinimumWorkingSetSize, lpMaximumWorkingSetSize );
}

typedef BOOL WINAPI FN_GetProcessWorkingSetSizeEx( HANDLE hProcess, PSIZE_T lpMinimumWorkingSetSize, PSIZE_T lpMaximumWorkingSetSize, PDWORD Flags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetProcessWorkingSetSizeEx( HANDLE hProcess, PSIZE_T lpMinimumWorkingSetSize, PSIZE_T lpMaximumWorkingSetSize, PDWORD Flags )
{
    static FN_GetProcessWorkingSetSizeEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessWorkingSetSizeEx", &g_Kernel32);
    return pfn( hProcess, lpMinimumWorkingSetSize, lpMaximumWorkingSetSize, Flags );
}

typedef BOOL WINAPI FN_SetProcessWorkingSetSize( HANDLE hProcess, SIZE_T dwMinimumWorkingSetSize, SIZE_T dwMaximumWorkingSetSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetProcessWorkingSetSize( HANDLE hProcess, SIZE_T dwMinimumWorkingSetSize, SIZE_T dwMaximumWorkingSetSize )
{
    static FN_SetProcessWorkingSetSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetProcessWorkingSetSize", &g_Kernel32);
    return pfn( hProcess, dwMinimumWorkingSetSize, dwMaximumWorkingSetSize );
}

typedef BOOL WINAPI FN_SetProcessWorkingSetSizeEx( HANDLE hProcess, SIZE_T dwMinimumWorkingSetSize, SIZE_T dwMaximumWorkingSetSize, DWORD Flags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetProcessWorkingSetSizeEx( HANDLE hProcess, SIZE_T dwMinimumWorkingSetSize, SIZE_T dwMaximumWorkingSetSize, DWORD Flags )
{
    static FN_SetProcessWorkingSetSizeEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetProcessWorkingSetSizeEx", &g_Kernel32);
    return pfn( hProcess, dwMinimumWorkingSetSize, dwMaximumWorkingSetSize, Flags );
}

typedef HANDLE WINAPI FN_OpenProcess( DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenProcess( DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId )
{
    static FN_OpenProcess *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenProcess", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, dwProcessId );
}

typedef HANDLE WINAPI FN_GetCurrentProcess( VOID );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_GetCurrentProcess( VOID )
{
    static FN_GetCurrentProcess *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentProcess", &g_Kernel32);
    return pfn ();
}

typedef DWORD WINAPI FN_GetCurrentProcessId( VOID );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetCurrentProcessId( VOID )
{
    static FN_GetCurrentProcessId *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentProcessId", &g_Kernel32);
    return pfn ();
}

typedef VOID WINAPI FN_ExitProcess( UINT uExitCode );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_ExitProcess( UINT uExitCode )
{
    static FN_ExitProcess *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ExitProcess", &g_Kernel32);
    pfn( uExitCode );
}

typedef BOOL WINAPI FN_TerminateProcess( HANDLE hProcess, UINT uExitCode );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_TerminateProcess( HANDLE hProcess, UINT uExitCode )
{
    static FN_TerminateProcess *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TerminateProcess", &g_Kernel32);
    return pfn( hProcess, uExitCode );
}

typedef BOOL WINAPI FN_GetExitCodeProcess( HANDLE hProcess, LPDWORD lpExitCode );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetExitCodeProcess( HANDLE hProcess, LPDWORD lpExitCode )
{
    static FN_GetExitCodeProcess *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetExitCodeProcess", &g_Kernel32);
    return pfn( hProcess, lpExitCode );
}

typedef VOID WINAPI FN_FatalExit( int ExitCode );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_FatalExit( int ExitCode )
{
    static FN_FatalExit *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FatalExit", &g_Kernel32);
    pfn( ExitCode );
}

typedef LPCH WINAPI FN_GetEnvironmentStrings( VOID );
__declspec(dllexport) LPCH WINAPI kPrf2Wrap_GetEnvironmentStrings( VOID )
{
    static FN_GetEnvironmentStrings *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetEnvironmentStrings", &g_Kernel32);
    return pfn ();
}

typedef LPWCH WINAPI FN_GetEnvironmentStringsW( VOID );
__declspec(dllexport) LPWCH WINAPI kPrf2Wrap_GetEnvironmentStringsW( VOID )
{
    static FN_GetEnvironmentStringsW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetEnvironmentStringsW", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_SetEnvironmentStringsA( LPCH NewEnvironment );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetEnvironmentStringsA( LPCH NewEnvironment )
{
    static FN_SetEnvironmentStringsA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetEnvironmentStringsA", &g_Kernel32);
    return pfn( NewEnvironment );
}

typedef BOOL WINAPI FN_SetEnvironmentStringsW( LPWCH NewEnvironment );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetEnvironmentStringsW( LPWCH NewEnvironment )
{
    static FN_SetEnvironmentStringsW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetEnvironmentStringsW", &g_Kernel32);
    return pfn( NewEnvironment );
}

typedef BOOL WINAPI FN_FreeEnvironmentStringsA( LPCH a);
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FreeEnvironmentStringsA( LPCH a)
{
    static FN_FreeEnvironmentStringsA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FreeEnvironmentStringsA", &g_Kernel32);
    return pfn( a);
}

typedef BOOL WINAPI FN_FreeEnvironmentStringsW( LPWCH a);
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FreeEnvironmentStringsW( LPWCH a)
{
    static FN_FreeEnvironmentStringsW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FreeEnvironmentStringsW", &g_Kernel32);
    return pfn( a);
}

typedef VOID WINAPI FN_RaiseException( DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nNumberOfArguments, CONST ULONG_PTR * lpArguments );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_RaiseException( DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nNumberOfArguments, CONST ULONG_PTR * lpArguments )
{
    static FN_RaiseException *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RaiseException", &g_Kernel32);
    pfn( dwExceptionCode, dwExceptionFlags, nNumberOfArguments, lpArguments );
}

typedef LONG WINAPI FN_UnhandledExceptionFilter( struct _EXCEPTION_POINTERS * ExceptionInfo );
__declspec(dllexport) LONG WINAPI kPrf2Wrap_UnhandledExceptionFilter( struct _EXCEPTION_POINTERS * ExceptionInfo )
{
    static FN_UnhandledExceptionFilter *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "UnhandledExceptionFilter", &g_Kernel32);
    return pfn( ExceptionInfo );
}

typedef LPTOP_LEVEL_EXCEPTION_FILTER WINAPI FN_SetUnhandledExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter );
__declspec(dllexport) LPTOP_LEVEL_EXCEPTION_FILTER WINAPI kPrf2Wrap_SetUnhandledExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter )
{
    static FN_SetUnhandledExceptionFilter *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetUnhandledExceptionFilter", &g_Kernel32);
    return pfn( lpTopLevelExceptionFilter );
}

typedef LPVOID WINAPI FN_CreateFiber( SIZE_T dwStackSize, LPFIBER_START_ROUTINE lpStartAddress, LPVOID lpParameter );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_CreateFiber( SIZE_T dwStackSize, LPFIBER_START_ROUTINE lpStartAddress, LPVOID lpParameter )
{
    static FN_CreateFiber *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateFiber", &g_Kernel32);
    return pfn( dwStackSize, lpStartAddress, lpParameter );
}

typedef LPVOID WINAPI FN_CreateFiberEx( SIZE_T dwStackCommitSize, SIZE_T dwStackReserveSize, DWORD dwFlags, LPFIBER_START_ROUTINE lpStartAddress, LPVOID lpParameter );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_CreateFiberEx( SIZE_T dwStackCommitSize, SIZE_T dwStackReserveSize, DWORD dwFlags, LPFIBER_START_ROUTINE lpStartAddress, LPVOID lpParameter )
{
    static FN_CreateFiberEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateFiberEx", &g_Kernel32);
    return pfn( dwStackCommitSize, dwStackReserveSize, dwFlags, lpStartAddress, lpParameter );
}

typedef VOID WINAPI FN_DeleteFiber( LPVOID lpFiber );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_DeleteFiber( LPVOID lpFiber )
{
    static FN_DeleteFiber *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteFiber", &g_Kernel32);
    pfn( lpFiber );
}

typedef LPVOID WINAPI FN_ConvertThreadToFiber( LPVOID lpParameter );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_ConvertThreadToFiber( LPVOID lpParameter )
{
    static FN_ConvertThreadToFiber *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ConvertThreadToFiber", &g_Kernel32);
    return pfn( lpParameter );
}

typedef LPVOID WINAPI FN_ConvertThreadToFiberEx( LPVOID lpParameter, DWORD dwFlags );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_ConvertThreadToFiberEx( LPVOID lpParameter, DWORD dwFlags )
{
    static FN_ConvertThreadToFiberEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ConvertThreadToFiberEx", &g_Kernel32);
    return pfn( lpParameter, dwFlags );
}

typedef BOOL WINAPI FN_ConvertFiberToThread( VOID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ConvertFiberToThread( VOID )
{
    static FN_ConvertFiberToThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ConvertFiberToThread", &g_Kernel32);
    return pfn ();
}

typedef VOID WINAPI FN_SwitchToFiber( LPVOID lpFiber );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_SwitchToFiber( LPVOID lpFiber )
{
    static FN_SwitchToFiber *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SwitchToFiber", &g_Kernel32);
    pfn( lpFiber );
}

typedef BOOL WINAPI FN_SwitchToThread( VOID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SwitchToThread( VOID )
{
    static FN_SwitchToThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SwitchToThread", &g_Kernel32);
    return pfn ();
}

typedef HANDLE WINAPI FN_CreateThread( LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateThread( LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId )
{
    static FN_CreateThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateThread", &g_Kernel32);
    return pfn( lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId );
}

typedef HANDLE WINAPI FN_CreateRemoteThread( HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateRemoteThread( HANDLE hProcess, LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId )
{
    static FN_CreateRemoteThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateRemoteThread", &g_Kernel32);
    return pfn( hProcess, lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId );
}

typedef HANDLE WINAPI FN_GetCurrentThread( VOID );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_GetCurrentThread( VOID )
{
    static FN_GetCurrentThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentThread", &g_Kernel32);
    return pfn ();
}

typedef DWORD WINAPI FN_GetCurrentThreadId( VOID );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetCurrentThreadId( VOID )
{
    static FN_GetCurrentThreadId *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentThreadId", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_SetThreadStackGuarantee( PULONG StackSizeInBytes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetThreadStackGuarantee( PULONG StackSizeInBytes )
{
    static FN_SetThreadStackGuarantee *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetThreadStackGuarantee", &g_Kernel32);
    return pfn( StackSizeInBytes );
}

typedef DWORD WINAPI FN_GetProcessIdOfThread( HANDLE Thread );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetProcessIdOfThread( HANDLE Thread )
{
    static FN_GetProcessIdOfThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessIdOfThread", &g_Kernel32);
    return pfn( Thread );
}

typedef DWORD WINAPI FN_GetThreadId( HANDLE Thread );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetThreadId( HANDLE Thread )
{
    static FN_GetThreadId *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetThreadId", &g_Kernel32);
    return pfn( Thread );
}

typedef DWORD WINAPI FN_GetProcessId( HANDLE Process );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetProcessId( HANDLE Process )
{
    static FN_GetProcessId *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessId", &g_Kernel32);
    return pfn( Process );
}

typedef DWORD WINAPI FN_GetCurrentProcessorNumber( VOID );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetCurrentProcessorNumber( VOID )
{
    static FN_GetCurrentProcessorNumber *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentProcessorNumber", &g_Kernel32);
    return pfn ();
}

typedef DWORD_PTR WINAPI FN_SetThreadAffinityMask( HANDLE hThread, DWORD_PTR dwThreadAffinityMask );
__declspec(dllexport) DWORD_PTR WINAPI kPrf2Wrap_SetThreadAffinityMask( HANDLE hThread, DWORD_PTR dwThreadAffinityMask )
{
    static FN_SetThreadAffinityMask *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetThreadAffinityMask", &g_Kernel32);
    return pfn( hThread, dwThreadAffinityMask );
}

typedef DWORD WINAPI FN_SetThreadIdealProcessor( HANDLE hThread, DWORD dwIdealProcessor );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SetThreadIdealProcessor( HANDLE hThread, DWORD dwIdealProcessor )
{
    static FN_SetThreadIdealProcessor *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetThreadIdealProcessor", &g_Kernel32);
    return pfn( hThread, dwIdealProcessor );
}

typedef BOOL WINAPI FN_SetProcessPriorityBoost( HANDLE hProcess, BOOL bDisablePriorityBoost );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetProcessPriorityBoost( HANDLE hProcess, BOOL bDisablePriorityBoost )
{
    static FN_SetProcessPriorityBoost *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetProcessPriorityBoost", &g_Kernel32);
    return pfn( hProcess, bDisablePriorityBoost );
}

typedef BOOL WINAPI FN_GetProcessPriorityBoost( HANDLE hProcess, PBOOL pDisablePriorityBoost );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetProcessPriorityBoost( HANDLE hProcess, PBOOL pDisablePriorityBoost )
{
    static FN_GetProcessPriorityBoost *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessPriorityBoost", &g_Kernel32);
    return pfn( hProcess, pDisablePriorityBoost );
}

typedef BOOL WINAPI FN_RequestWakeupLatency( LATENCY_TIME latency );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_RequestWakeupLatency( LATENCY_TIME latency )
{
    static FN_RequestWakeupLatency *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RequestWakeupLatency", &g_Kernel32);
    return pfn( latency );
}

typedef BOOL WINAPI FN_IsSystemResumeAutomatic( VOID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsSystemResumeAutomatic( VOID )
{
    static FN_IsSystemResumeAutomatic *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsSystemResumeAutomatic", &g_Kernel32);
    return pfn ();
}

typedef HANDLE WINAPI FN_OpenThread( DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenThread( DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwThreadId )
{
    static FN_OpenThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenThread", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, dwThreadId );
}

typedef BOOL WINAPI FN_SetThreadPriority( HANDLE hThread, int nPriority );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetThreadPriority( HANDLE hThread, int nPriority )
{
    static FN_SetThreadPriority *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetThreadPriority", &g_Kernel32);
    return pfn( hThread, nPriority );
}

typedef BOOL WINAPI FN_SetThreadPriorityBoost( HANDLE hThread, BOOL bDisablePriorityBoost );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetThreadPriorityBoost( HANDLE hThread, BOOL bDisablePriorityBoost )
{
    static FN_SetThreadPriorityBoost *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetThreadPriorityBoost", &g_Kernel32);
    return pfn( hThread, bDisablePriorityBoost );
}

typedef BOOL WINAPI FN_GetThreadPriorityBoost( HANDLE hThread, PBOOL pDisablePriorityBoost );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetThreadPriorityBoost( HANDLE hThread, PBOOL pDisablePriorityBoost )
{
    static FN_GetThreadPriorityBoost *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetThreadPriorityBoost", &g_Kernel32);
    return pfn( hThread, pDisablePriorityBoost );
}

typedef int WINAPI FN_GetThreadPriority( HANDLE hThread );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetThreadPriority( HANDLE hThread )
{
    static FN_GetThreadPriority *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetThreadPriority", &g_Kernel32);
    return pfn( hThread );
}

typedef BOOL WINAPI FN_GetThreadTimes( HANDLE hThread, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetThreadTimes( HANDLE hThread, LPFILETIME lpCreationTime, LPFILETIME lpExitTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime )
{
    static FN_GetThreadTimes *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetThreadTimes", &g_Kernel32);
    return pfn( hThread, lpCreationTime, lpExitTime, lpKernelTime, lpUserTime );
}

typedef BOOL WINAPI FN_GetThreadIOPendingFlag( HANDLE hThread, PBOOL lpIOIsPending );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetThreadIOPendingFlag( HANDLE hThread, PBOOL lpIOIsPending )
{
    static FN_GetThreadIOPendingFlag *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetThreadIOPendingFlag", &g_Kernel32);
    return pfn( hThread, lpIOIsPending );
}

typedef VOID WINAPI FN_ExitThread( DWORD dwExitCode );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_ExitThread( DWORD dwExitCode )
{
    static FN_ExitThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ExitThread", &g_Kernel32);
    pfn( dwExitCode );
}

typedef BOOL WINAPI FN_TerminateThread( HANDLE hThread, DWORD dwExitCode );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_TerminateThread( HANDLE hThread, DWORD dwExitCode )
{
    static FN_TerminateThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TerminateThread", &g_Kernel32);
    return pfn( hThread, dwExitCode );
}

typedef BOOL WINAPI FN_GetExitCodeThread( HANDLE hThread, LPDWORD lpExitCode );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetExitCodeThread( HANDLE hThread, LPDWORD lpExitCode )
{
    static FN_GetExitCodeThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetExitCodeThread", &g_Kernel32);
    return pfn( hThread, lpExitCode );
}

typedef BOOL WINAPI FN_GetThreadSelectorEntry( HANDLE hThread, DWORD dwSelector, LPLDT_ENTRY lpSelectorEntry );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetThreadSelectorEntry( HANDLE hThread, DWORD dwSelector, LPLDT_ENTRY lpSelectorEntry )
{
    static FN_GetThreadSelectorEntry *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetThreadSelectorEntry", &g_Kernel32);
    return pfn( hThread, dwSelector, lpSelectorEntry );
}

typedef EXECUTION_STATE WINAPI FN_SetThreadExecutionState( EXECUTION_STATE esFlags );
__declspec(dllexport) EXECUTION_STATE WINAPI kPrf2Wrap_SetThreadExecutionState( EXECUTION_STATE esFlags )
{
    static FN_SetThreadExecutionState *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetThreadExecutionState", &g_Kernel32);
    return pfn( esFlags );
}

typedef DWORD WINAPI FN_GetLastError( VOID );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetLastError( VOID )
{
    static FN_GetLastError *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLastError", &g_Kernel32);
    return pfn ();
}

typedef VOID WINAPI FN_SetLastError( DWORD dwErrCode );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_SetLastError( DWORD dwErrCode )
{
    static FN_SetLastError *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetLastError", &g_Kernel32);
    pfn( dwErrCode );
}

typedef VOID WINAPI FN_RestoreLastError( DWORD dwErrCode );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_RestoreLastError( DWORD dwErrCode )
{
    static FN_RestoreLastError *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RestoreLastError", &g_Kernel32);
    pfn( dwErrCode );
}

typedef BOOL WINAPI FN_GetOverlappedResult( HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetOverlappedResult( HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait )
{
    static FN_GetOverlappedResult *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetOverlappedResult", &g_Kernel32);
    return pfn( hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait );
}

typedef HANDLE WINAPI FN_CreateIoCompletionPort( HANDLE FileHandle, HANDLE ExistingCompletionPort, ULONG_PTR CompletionKey, DWORD NumberOfConcurrentThreads );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateIoCompletionPort( HANDLE FileHandle, HANDLE ExistingCompletionPort, ULONG_PTR CompletionKey, DWORD NumberOfConcurrentThreads )
{
    static FN_CreateIoCompletionPort *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateIoCompletionPort", &g_Kernel32);
    return pfn( FileHandle, ExistingCompletionPort, CompletionKey, NumberOfConcurrentThreads );
}

typedef BOOL WINAPI FN_GetQueuedCompletionStatus( HANDLE CompletionPort, LPDWORD lpNumberOfBytesTransferred, PULONG_PTR lpCompletionKey, LPOVERLAPPED * lpOverlapped, DWORD dwMilliseconds );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetQueuedCompletionStatus( HANDLE CompletionPort, LPDWORD lpNumberOfBytesTransferred, PULONG_PTR lpCompletionKey, LPOVERLAPPED * lpOverlapped, DWORD dwMilliseconds )
{
    static FN_GetQueuedCompletionStatus *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetQueuedCompletionStatus", &g_Kernel32);
    return pfn( CompletionPort, lpNumberOfBytesTransferred, lpCompletionKey, lpOverlapped, dwMilliseconds );
}

typedef BOOL WINAPI FN_PostQueuedCompletionStatus( HANDLE CompletionPort, DWORD dwNumberOfBytesTransferred, ULONG_PTR dwCompletionKey, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_PostQueuedCompletionStatus( HANDLE CompletionPort, DWORD dwNumberOfBytesTransferred, ULONG_PTR dwCompletionKey, LPOVERLAPPED lpOverlapped )
{
    static FN_PostQueuedCompletionStatus *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PostQueuedCompletionStatus", &g_Kernel32);
    return pfn( CompletionPort, dwNumberOfBytesTransferred, dwCompletionKey, lpOverlapped );
}

typedef UINT WINAPI FN_SetErrorMode( UINT uMode );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_SetErrorMode( UINT uMode )
{
    static FN_SetErrorMode *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetErrorMode", &g_Kernel32);
    return pfn( uMode );
}

typedef BOOL WINAPI FN_ReadProcessMemory( HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T * lpNumberOfBytesRead );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadProcessMemory( HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T * lpNumberOfBytesRead )
{
    static FN_ReadProcessMemory *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadProcessMemory", &g_Kernel32);
    return pfn( hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead );
}

typedef BOOL WINAPI FN_WriteProcessMemory( HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T * lpNumberOfBytesWritten );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteProcessMemory( HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T * lpNumberOfBytesWritten )
{
    static FN_WriteProcessMemory *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteProcessMemory", &g_Kernel32);
    return pfn( hProcess, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten );
}

typedef BOOL WINAPI FN_GetThreadContext( HANDLE hThread, LPCONTEXT lpContext );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetThreadContext( HANDLE hThread, LPCONTEXT lpContext )
{
    static FN_GetThreadContext *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetThreadContext", &g_Kernel32);
    return pfn( hThread, lpContext );
}

typedef BOOL WINAPI FN_SetThreadContext( HANDLE hThread, CONST CONTEXT * lpContext );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetThreadContext( HANDLE hThread, CONST CONTEXT * lpContext )
{
    static FN_SetThreadContext *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetThreadContext", &g_Kernel32);
    return pfn( hThread, lpContext );
}

typedef DWORD WINAPI FN_SuspendThread( HANDLE hThread );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SuspendThread( HANDLE hThread )
{
    static FN_SuspendThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SuspendThread", &g_Kernel32);
    return pfn( hThread );
}

typedef DWORD WINAPI FN_ResumeThread( HANDLE hThread );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_ResumeThread( HANDLE hThread )
{
    static FN_ResumeThread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ResumeThread", &g_Kernel32);
    return pfn( hThread );
}

typedef DWORD WINAPI FN_QueueUserAPC( PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_QueueUserAPC( PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData )
{
    static FN_QueueUserAPC *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueueUserAPC", &g_Kernel32);
    return pfn( pfnAPC, hThread, dwData );
}

typedef BOOL WINAPI FN_IsDebuggerPresent( VOID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsDebuggerPresent( VOID )
{
    static FN_IsDebuggerPresent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsDebuggerPresent", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_CheckRemoteDebuggerPresent( HANDLE hProcess, PBOOL pbDebuggerPresent );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CheckRemoteDebuggerPresent( HANDLE hProcess, PBOOL pbDebuggerPresent )
{
    static FN_CheckRemoteDebuggerPresent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CheckRemoteDebuggerPresent", &g_Kernel32);
    return pfn( hProcess, pbDebuggerPresent );
}

typedef VOID WINAPI FN_DebugBreak( VOID );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_DebugBreak( VOID )
{
    static FN_DebugBreak *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DebugBreak", &g_Kernel32);
    pfn ();
}

typedef BOOL WINAPI FN_WaitForDebugEvent( LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WaitForDebugEvent( LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds )
{
    static FN_WaitForDebugEvent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WaitForDebugEvent", &g_Kernel32);
    return pfn( lpDebugEvent, dwMilliseconds );
}

typedef BOOL WINAPI FN_ContinueDebugEvent( DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ContinueDebugEvent( DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus )
{
    static FN_ContinueDebugEvent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ContinueDebugEvent", &g_Kernel32);
    return pfn( dwProcessId, dwThreadId, dwContinueStatus );
}

typedef BOOL WINAPI FN_DebugActiveProcess( DWORD dwProcessId );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DebugActiveProcess( DWORD dwProcessId )
{
    static FN_DebugActiveProcess *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DebugActiveProcess", &g_Kernel32);
    return pfn( dwProcessId );
}

typedef BOOL WINAPI FN_DebugActiveProcessStop( DWORD dwProcessId );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DebugActiveProcessStop( DWORD dwProcessId )
{
    static FN_DebugActiveProcessStop *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DebugActiveProcessStop", &g_Kernel32);
    return pfn( dwProcessId );
}

typedef BOOL WINAPI FN_DebugSetProcessKillOnExit( BOOL KillOnExit );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DebugSetProcessKillOnExit( BOOL KillOnExit )
{
    static FN_DebugSetProcessKillOnExit *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DebugSetProcessKillOnExit", &g_Kernel32);
    return pfn( KillOnExit );
}

typedef BOOL WINAPI FN_DebugBreakProcess( HANDLE Process );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DebugBreakProcess( HANDLE Process )
{
    static FN_DebugBreakProcess *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DebugBreakProcess", &g_Kernel32);
    return pfn( Process );
}

typedef VOID WINAPI FN_InitializeCriticalSection( LPCRITICAL_SECTION lpCriticalSection );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_InitializeCriticalSection( LPCRITICAL_SECTION lpCriticalSection )
{
    static FN_InitializeCriticalSection *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InitializeCriticalSection", &g_Kernel32);
    pfn( lpCriticalSection );
}

typedef VOID WINAPI FN_EnterCriticalSection( LPCRITICAL_SECTION lpCriticalSection );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_EnterCriticalSection( LPCRITICAL_SECTION lpCriticalSection )
{
    static FN_EnterCriticalSection *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnterCriticalSection", &g_Kernel32);
    pfn( lpCriticalSection );
}

typedef VOID WINAPI FN_LeaveCriticalSection( LPCRITICAL_SECTION lpCriticalSection );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_LeaveCriticalSection( LPCRITICAL_SECTION lpCriticalSection )
{
    static FN_LeaveCriticalSection *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LeaveCriticalSection", &g_Kernel32);
    pfn( lpCriticalSection );
}

typedef BOOL WINAPI FN_InitializeCriticalSectionAndSpinCount( LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_InitializeCriticalSectionAndSpinCount( LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount )
{
    static FN_InitializeCriticalSectionAndSpinCount *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InitializeCriticalSectionAndSpinCount", &g_Kernel32);
    return pfn( lpCriticalSection, dwSpinCount );
}

typedef DWORD WINAPI FN_SetCriticalSectionSpinCount( LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SetCriticalSectionSpinCount( LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount )
{
    static FN_SetCriticalSectionSpinCount *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCriticalSectionSpinCount", &g_Kernel32);
    return pfn( lpCriticalSection, dwSpinCount );
}

typedef BOOL WINAPI FN_TryEnterCriticalSection( LPCRITICAL_SECTION lpCriticalSection );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_TryEnterCriticalSection( LPCRITICAL_SECTION lpCriticalSection )
{
    static FN_TryEnterCriticalSection *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TryEnterCriticalSection", &g_Kernel32);
    return pfn( lpCriticalSection );
}

typedef VOID WINAPI FN_DeleteCriticalSection( LPCRITICAL_SECTION lpCriticalSection );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_DeleteCriticalSection( LPCRITICAL_SECTION lpCriticalSection )
{
    static FN_DeleteCriticalSection *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteCriticalSection", &g_Kernel32);
    pfn( lpCriticalSection );
}

typedef BOOL WINAPI FN_SetEvent( HANDLE hEvent );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetEvent( HANDLE hEvent )
{
    static FN_SetEvent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetEvent", &g_Kernel32);
    return pfn( hEvent );
}

typedef BOOL WINAPI FN_ResetEvent( HANDLE hEvent );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ResetEvent( HANDLE hEvent )
{
    static FN_ResetEvent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ResetEvent", &g_Kernel32);
    return pfn( hEvent );
}

typedef BOOL WINAPI FN_PulseEvent( HANDLE hEvent );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_PulseEvent( HANDLE hEvent )
{
    static FN_PulseEvent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PulseEvent", &g_Kernel32);
    return pfn( hEvent );
}

typedef BOOL WINAPI FN_ReleaseSemaphore( HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReleaseSemaphore( HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount )
{
    static FN_ReleaseSemaphore *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReleaseSemaphore", &g_Kernel32);
    return pfn( hSemaphore, lReleaseCount, lpPreviousCount );
}

typedef BOOL WINAPI FN_ReleaseMutex( HANDLE hMutex );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReleaseMutex( HANDLE hMutex )
{
    static FN_ReleaseMutex *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReleaseMutex", &g_Kernel32);
    return pfn( hMutex );
}

typedef DWORD WINAPI FN_WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_WaitForSingleObject( HANDLE hHandle, DWORD dwMilliseconds )
{
    static FN_WaitForSingleObject *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WaitForSingleObject", &g_Kernel32);
    return pfn( hHandle, dwMilliseconds );
}

typedef DWORD WINAPI FN_WaitForMultipleObjects( DWORD nCount, CONST HANDLE * lpHandles, BOOL bWaitAll, DWORD dwMilliseconds );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_WaitForMultipleObjects( DWORD nCount, CONST HANDLE * lpHandles, BOOL bWaitAll, DWORD dwMilliseconds )
{
    static FN_WaitForMultipleObjects *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WaitForMultipleObjects", &g_Kernel32);
    return pfn( nCount, lpHandles, bWaitAll, dwMilliseconds );
}

typedef VOID WINAPI FN_Sleep( DWORD dwMilliseconds );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_Sleep( DWORD dwMilliseconds )
{
    static FN_Sleep *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Sleep", &g_Kernel32);
    pfn( dwMilliseconds );
}

typedef HGLOBAL WINAPI FN_LoadResource( HMODULE hModule, HRSRC hResInfo );
__declspec(dllexport) HGLOBAL WINAPI kPrf2Wrap_LoadResource( HMODULE hModule, HRSRC hResInfo )
{
    static FN_LoadResource *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LoadResource", &g_Kernel32);
    return pfn( hModule, hResInfo );
}

typedef DWORD WINAPI FN_SizeofResource( HMODULE hModule, HRSRC hResInfo );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SizeofResource( HMODULE hModule, HRSRC hResInfo )
{
    static FN_SizeofResource *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SizeofResource", &g_Kernel32);
    return pfn( hModule, hResInfo );
}

typedef ATOM WINAPI FN_GlobalDeleteAtom( ATOM nAtom );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_GlobalDeleteAtom( ATOM nAtom )
{
    static FN_GlobalDeleteAtom *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalDeleteAtom", &g_Kernel32);
    return pfn( nAtom );
}

typedef BOOL WINAPI FN_InitAtomTable( DWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_InitAtomTable( DWORD nSize )
{
    static FN_InitAtomTable *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InitAtomTable", &g_Kernel32);
    return pfn( nSize );
}

typedef ATOM WINAPI FN_DeleteAtom( ATOM nAtom );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_DeleteAtom( ATOM nAtom )
{
    static FN_DeleteAtom *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteAtom", &g_Kernel32);
    return pfn( nAtom );
}

typedef UINT WINAPI FN_SetHandleCount( UINT uNumber );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_SetHandleCount( UINT uNumber )
{
    static FN_SetHandleCount *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetHandleCount", &g_Kernel32);
    return pfn( uNumber );
}

typedef DWORD WINAPI FN_GetLogicalDrives( VOID );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetLogicalDrives( VOID )
{
    static FN_GetLogicalDrives *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLogicalDrives", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_LockFile( HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LockFile( HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh )
{
    static FN_LockFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LockFile", &g_Kernel32);
    return pfn( hFile, dwFileOffsetLow, dwFileOffsetHigh, nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh );
}

typedef BOOL WINAPI FN_UnlockFile( HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_UnlockFile( HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh, DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh )
{
    static FN_UnlockFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "UnlockFile", &g_Kernel32);
    return pfn( hFile, dwFileOffsetLow, dwFileOffsetHigh, nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh );
}

typedef BOOL WINAPI FN_LockFileEx( HANDLE hFile, DWORD dwFlags, DWORD dwReserved, DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LockFileEx( HANDLE hFile, DWORD dwFlags, DWORD dwReserved, DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh, LPOVERLAPPED lpOverlapped )
{
    static FN_LockFileEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LockFileEx", &g_Kernel32);
    return pfn( hFile, dwFlags, dwReserved, nNumberOfBytesToLockLow, nNumberOfBytesToLockHigh, lpOverlapped );
}

typedef BOOL WINAPI FN_UnlockFileEx( HANDLE hFile, DWORD dwReserved, DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_UnlockFileEx( HANDLE hFile, DWORD dwReserved, DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh, LPOVERLAPPED lpOverlapped )
{
    static FN_UnlockFileEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "UnlockFileEx", &g_Kernel32);
    return pfn( hFile, dwReserved, nNumberOfBytesToUnlockLow, nNumberOfBytesToUnlockHigh, lpOverlapped );
}

typedef BOOL WINAPI FN_GetFileInformationByHandle( HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetFileInformationByHandle( HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation )
{
    static FN_GetFileInformationByHandle *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileInformationByHandle", &g_Kernel32);
    return pfn( hFile, lpFileInformation );
}

typedef DWORD WINAPI FN_GetFileType( HANDLE hFile );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetFileType( HANDLE hFile )
{
    static FN_GetFileType *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileType", &g_Kernel32);
    return pfn( hFile );
}

typedef DWORD WINAPI FN_GetFileSize( HANDLE hFile, LPDWORD lpFileSizeHigh );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetFileSize( HANDLE hFile, LPDWORD lpFileSizeHigh )
{
    static FN_GetFileSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileSize", &g_Kernel32);
    return pfn( hFile, lpFileSizeHigh );
}

typedef BOOL WINAPI FN_GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER lpFileSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetFileSizeEx( HANDLE hFile, PLARGE_INTEGER lpFileSize )
{
    static FN_GetFileSizeEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileSizeEx", &g_Kernel32);
    return pfn( hFile, lpFileSize );
}

typedef HANDLE WINAPI FN_GetStdHandle( DWORD nStdHandle );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_GetStdHandle( DWORD nStdHandle )
{
    static FN_GetStdHandle *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetStdHandle", &g_Kernel32);
    return pfn( nStdHandle );
}

typedef BOOL WINAPI FN_SetStdHandle( DWORD nStdHandle, HANDLE hHandle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetStdHandle( DWORD nStdHandle, HANDLE hHandle )
{
    static FN_SetStdHandle *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetStdHandle", &g_Kernel32);
    return pfn( nStdHandle, hHandle );
}

typedef BOOL WINAPI FN_WriteFile( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteFile( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped )
{
    static FN_WriteFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteFile", &g_Kernel32);
    return pfn( hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped );
}

typedef BOOL WINAPI FN_ReadFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped )
{
    static FN_ReadFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadFile", &g_Kernel32);
    return pfn( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped );
}

typedef BOOL WINAPI FN_FlushFileBuffers( HANDLE hFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FlushFileBuffers( HANDLE hFile )
{
    static FN_FlushFileBuffers *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FlushFileBuffers", &g_Kernel32);
    return pfn( hFile );
}

typedef BOOL WINAPI FN_DeviceIoControl( HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeviceIoControl( HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped )
{
    static FN_DeviceIoControl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeviceIoControl", &g_Kernel32);
    return pfn( hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped );
}

typedef BOOL WINAPI FN_RequestDeviceWakeup( HANDLE hDevice );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_RequestDeviceWakeup( HANDLE hDevice )
{
    static FN_RequestDeviceWakeup *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RequestDeviceWakeup", &g_Kernel32);
    return pfn( hDevice );
}

typedef BOOL WINAPI FN_CancelDeviceWakeupRequest( HANDLE hDevice );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CancelDeviceWakeupRequest( HANDLE hDevice )
{
    static FN_CancelDeviceWakeupRequest *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CancelDeviceWakeupRequest", &g_Kernel32);
    return pfn( hDevice );
}

typedef BOOL WINAPI FN_GetDevicePowerState( HANDLE hDevice, BOOL * pfOn );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetDevicePowerState( HANDLE hDevice, BOOL * pfOn )
{
    static FN_GetDevicePowerState *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDevicePowerState", &g_Kernel32);
    return pfn( hDevice, pfOn );
}

typedef BOOL WINAPI FN_SetMessageWaitingIndicator( HANDLE hMsgIndicator, ULONG ulMsgCount );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetMessageWaitingIndicator( HANDLE hMsgIndicator, ULONG ulMsgCount )
{
    static FN_SetMessageWaitingIndicator *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetMessageWaitingIndicator", &g_Kernel32);
    return pfn( hMsgIndicator, ulMsgCount );
}

typedef BOOL WINAPI FN_SetEndOfFile( HANDLE hFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetEndOfFile( HANDLE hFile )
{
    static FN_SetEndOfFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetEndOfFile", &g_Kernel32);
    return pfn( hFile );
}

typedef DWORD WINAPI FN_SetFilePointer( HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SetFilePointer( HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod )
{
    static FN_SetFilePointer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFilePointer", &g_Kernel32);
    return pfn( hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod );
}

typedef BOOL WINAPI FN_SetFilePointerEx( HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFilePointerEx( HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod )
{
    static FN_SetFilePointerEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFilePointerEx", &g_Kernel32);
    return pfn( hFile, liDistanceToMove, lpNewFilePointer, dwMoveMethod );
}

typedef BOOL WINAPI FN_FindClose( HANDLE hFindFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindClose( HANDLE hFindFile )
{
    static FN_FindClose *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindClose", &g_Kernel32);
    return pfn( hFindFile );
}

typedef BOOL WINAPI FN_GetFileTime( HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetFileTime( HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime )
{
    static FN_GetFileTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileTime", &g_Kernel32);
    return pfn( hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime );
}

typedef BOOL WINAPI FN_SetFileTime( HANDLE hFile, CONST FILETIME * lpCreationTime, CONST FILETIME * lpLastAccessTime, CONST FILETIME * lpLastWriteTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFileTime( HANDLE hFile, CONST FILETIME * lpCreationTime, CONST FILETIME * lpLastAccessTime, CONST FILETIME * lpLastWriteTime )
{
    static FN_SetFileTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileTime", &g_Kernel32);
    return pfn( hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime );
}

typedef BOOL WINAPI FN_SetFileValidData( HANDLE hFile, LONGLONG ValidDataLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFileValidData( HANDLE hFile, LONGLONG ValidDataLength )
{
    static FN_SetFileValidData *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileValidData", &g_Kernel32);
    return pfn( hFile, ValidDataLength );
}

typedef BOOL WINAPI FN_SetFileShortNameA( HANDLE hFile, LPCSTR lpShortName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFileShortNameA( HANDLE hFile, LPCSTR lpShortName )
{
    static FN_SetFileShortNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileShortNameA", &g_Kernel32);
    return pfn( hFile, lpShortName );
}

typedef BOOL WINAPI FN_SetFileShortNameW( HANDLE hFile, LPCWSTR lpShortName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFileShortNameW( HANDLE hFile, LPCWSTR lpShortName )
{
    static FN_SetFileShortNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileShortNameW", &g_Kernel32);
    return pfn( hFile, lpShortName );
}

typedef BOOL WINAPI FN_CloseHandle( HANDLE hObject );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CloseHandle( HANDLE hObject )
{
    static FN_CloseHandle *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CloseHandle", &g_Kernel32);
    return pfn( hObject );
}

typedef BOOL WINAPI FN_DuplicateHandle( HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle, DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DuplicateHandle( HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle, DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions )
{
    static FN_DuplicateHandle *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DuplicateHandle", &g_Kernel32);
    return pfn( hSourceProcessHandle, hSourceHandle, hTargetProcessHandle, lpTargetHandle, dwDesiredAccess, bInheritHandle, dwOptions );
}

typedef BOOL WINAPI FN_GetHandleInformation( HANDLE hObject, LPDWORD lpdwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetHandleInformation( HANDLE hObject, LPDWORD lpdwFlags )
{
    static FN_GetHandleInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetHandleInformation", &g_Kernel32);
    return pfn( hObject, lpdwFlags );
}

typedef BOOL WINAPI FN_SetHandleInformation( HANDLE hObject, DWORD dwMask, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetHandleInformation( HANDLE hObject, DWORD dwMask, DWORD dwFlags )
{
    static FN_SetHandleInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetHandleInformation", &g_Kernel32);
    return pfn( hObject, dwMask, dwFlags );
}

typedef DWORD WINAPI FN_LoadModule( LPCSTR lpModuleName, LPVOID lpParameterBlock );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_LoadModule( LPCSTR lpModuleName, LPVOID lpParameterBlock )
{
    static FN_LoadModule *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LoadModule", &g_Kernel32);
    return pfn( lpModuleName, lpParameterBlock );
}

typedef UINT WINAPI FN_WinExec( LPCSTR lpCmdLine, UINT uCmdShow );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_WinExec( LPCSTR lpCmdLine, UINT uCmdShow )
{
    static FN_WinExec *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WinExec", &g_Kernel32);
    return pfn( lpCmdLine, uCmdShow );
}

typedef BOOL WINAPI FN_ClearCommBreak( HANDLE hFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ClearCommBreak( HANDLE hFile )
{
    static FN_ClearCommBreak *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ClearCommBreak", &g_Kernel32);
    return pfn( hFile );
}

typedef BOOL WINAPI FN_ClearCommError( HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ClearCommError( HANDLE hFile, LPDWORD lpErrors, LPCOMSTAT lpStat )
{
    static FN_ClearCommError *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ClearCommError", &g_Kernel32);
    return pfn( hFile, lpErrors, lpStat );
}

typedef BOOL WINAPI FN_SetupComm( HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetupComm( HANDLE hFile, DWORD dwInQueue, DWORD dwOutQueue )
{
    static FN_SetupComm *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetupComm", &g_Kernel32);
    return pfn( hFile, dwInQueue, dwOutQueue );
}

typedef BOOL WINAPI FN_EscapeCommFunction( HANDLE hFile, DWORD dwFunc );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EscapeCommFunction( HANDLE hFile, DWORD dwFunc )
{
    static FN_EscapeCommFunction *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EscapeCommFunction", &g_Kernel32);
    return pfn( hFile, dwFunc );
}

typedef BOOL WINAPI FN_GetCommConfig( HANDLE hCommDev, LPCOMMCONFIG lpCC, LPDWORD lpdwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCommConfig( HANDLE hCommDev, LPCOMMCONFIG lpCC, LPDWORD lpdwSize )
{
    static FN_GetCommConfig *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCommConfig", &g_Kernel32);
    return pfn( hCommDev, lpCC, lpdwSize );
}

typedef BOOL WINAPI FN_GetCommMask( HANDLE hFile, LPDWORD lpEvtMask );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCommMask( HANDLE hFile, LPDWORD lpEvtMask )
{
    static FN_GetCommMask *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCommMask", &g_Kernel32);
    return pfn( hFile, lpEvtMask );
}

typedef BOOL WINAPI FN_GetCommProperties( HANDLE hFile, LPCOMMPROP lpCommProp );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCommProperties( HANDLE hFile, LPCOMMPROP lpCommProp )
{
    static FN_GetCommProperties *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCommProperties", &g_Kernel32);
    return pfn( hFile, lpCommProp );
}

typedef BOOL WINAPI FN_GetCommModemStatus( HANDLE hFile, LPDWORD lpModemStat );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCommModemStatus( HANDLE hFile, LPDWORD lpModemStat )
{
    static FN_GetCommModemStatus *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCommModemStatus", &g_Kernel32);
    return pfn( hFile, lpModemStat );
}

typedef BOOL WINAPI FN_GetCommState( HANDLE hFile, LPDCB lpDCB );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCommState( HANDLE hFile, LPDCB lpDCB )
{
    static FN_GetCommState *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCommState", &g_Kernel32);
    return pfn( hFile, lpDCB );
}

typedef BOOL WINAPI FN_GetCommTimeouts( HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCommTimeouts( HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts )
{
    static FN_GetCommTimeouts *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCommTimeouts", &g_Kernel32);
    return pfn( hFile, lpCommTimeouts );
}

typedef BOOL WINAPI FN_PurgeComm( HANDLE hFile, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_PurgeComm( HANDLE hFile, DWORD dwFlags )
{
    static FN_PurgeComm *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PurgeComm", &g_Kernel32);
    return pfn( hFile, dwFlags );
}

typedef BOOL WINAPI FN_SetCommBreak( HANDLE hFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetCommBreak( HANDLE hFile )
{
    static FN_SetCommBreak *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCommBreak", &g_Kernel32);
    return pfn( hFile );
}

typedef BOOL WINAPI FN_SetCommConfig( HANDLE hCommDev, LPCOMMCONFIG lpCC, DWORD dwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetCommConfig( HANDLE hCommDev, LPCOMMCONFIG lpCC, DWORD dwSize )
{
    static FN_SetCommConfig *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCommConfig", &g_Kernel32);
    return pfn( hCommDev, lpCC, dwSize );
}

typedef BOOL WINAPI FN_SetCommMask( HANDLE hFile, DWORD dwEvtMask );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetCommMask( HANDLE hFile, DWORD dwEvtMask )
{
    static FN_SetCommMask *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCommMask", &g_Kernel32);
    return pfn( hFile, dwEvtMask );
}

typedef BOOL WINAPI FN_SetCommState( HANDLE hFile, LPDCB lpDCB );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetCommState( HANDLE hFile, LPDCB lpDCB )
{
    static FN_SetCommState *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCommState", &g_Kernel32);
    return pfn( hFile, lpDCB );
}

typedef BOOL WINAPI FN_SetCommTimeouts( HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetCommTimeouts( HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts )
{
    static FN_SetCommTimeouts *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCommTimeouts", &g_Kernel32);
    return pfn( hFile, lpCommTimeouts );
}

typedef BOOL WINAPI FN_TransmitCommChar( HANDLE hFile, char cChar );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_TransmitCommChar( HANDLE hFile, char cChar )
{
    static FN_TransmitCommChar *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TransmitCommChar", &g_Kernel32);
    return pfn( hFile, cChar );
}

typedef BOOL WINAPI FN_WaitCommEvent( HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WaitCommEvent( HANDLE hFile, LPDWORD lpEvtMask, LPOVERLAPPED lpOverlapped )
{
    static FN_WaitCommEvent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WaitCommEvent", &g_Kernel32);
    return pfn( hFile, lpEvtMask, lpOverlapped );
}

typedef DWORD WINAPI FN_SetTapePosition( HANDLE hDevice, DWORD dwPositionMethod, DWORD dwPartition, DWORD dwOffsetLow, DWORD dwOffsetHigh, BOOL bImmediate );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SetTapePosition( HANDLE hDevice, DWORD dwPositionMethod, DWORD dwPartition, DWORD dwOffsetLow, DWORD dwOffsetHigh, BOOL bImmediate )
{
    static FN_SetTapePosition *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetTapePosition", &g_Kernel32);
    return pfn( hDevice, dwPositionMethod, dwPartition, dwOffsetLow, dwOffsetHigh, bImmediate );
}

typedef DWORD WINAPI FN_GetTapePosition( HANDLE hDevice, DWORD dwPositionType, LPDWORD lpdwPartition, LPDWORD lpdwOffsetLow, LPDWORD lpdwOffsetHigh );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetTapePosition( HANDLE hDevice, DWORD dwPositionType, LPDWORD lpdwPartition, LPDWORD lpdwOffsetLow, LPDWORD lpdwOffsetHigh )
{
    static FN_GetTapePosition *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTapePosition", &g_Kernel32);
    return pfn( hDevice, dwPositionType, lpdwPartition, lpdwOffsetLow, lpdwOffsetHigh );
}

typedef DWORD WINAPI FN_PrepareTape( HANDLE hDevice, DWORD dwOperation, BOOL bImmediate );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_PrepareTape( HANDLE hDevice, DWORD dwOperation, BOOL bImmediate )
{
    static FN_PrepareTape *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PrepareTape", &g_Kernel32);
    return pfn( hDevice, dwOperation, bImmediate );
}

typedef DWORD WINAPI FN_EraseTape( HANDLE hDevice, DWORD dwEraseType, BOOL bImmediate );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_EraseTape( HANDLE hDevice, DWORD dwEraseType, BOOL bImmediate )
{
    static FN_EraseTape *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EraseTape", &g_Kernel32);
    return pfn( hDevice, dwEraseType, bImmediate );
}

typedef DWORD WINAPI FN_CreateTapePartition( HANDLE hDevice, DWORD dwPartitionMethod, DWORD dwCount, DWORD dwSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_CreateTapePartition( HANDLE hDevice, DWORD dwPartitionMethod, DWORD dwCount, DWORD dwSize )
{
    static FN_CreateTapePartition *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateTapePartition", &g_Kernel32);
    return pfn( hDevice, dwPartitionMethod, dwCount, dwSize );
}

typedef DWORD WINAPI FN_WriteTapemark( HANDLE hDevice, DWORD dwTapemarkType, DWORD dwTapemarkCount, BOOL bImmediate );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_WriteTapemark( HANDLE hDevice, DWORD dwTapemarkType, DWORD dwTapemarkCount, BOOL bImmediate )
{
    static FN_WriteTapemark *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteTapemark", &g_Kernel32);
    return pfn( hDevice, dwTapemarkType, dwTapemarkCount, bImmediate );
}

typedef DWORD WINAPI FN_GetTapeStatus( HANDLE hDevice );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetTapeStatus( HANDLE hDevice )
{
    static FN_GetTapeStatus *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTapeStatus", &g_Kernel32);
    return pfn( hDevice );
}

typedef DWORD WINAPI FN_GetTapeParameters( HANDLE hDevice, DWORD dwOperation, LPDWORD lpdwSize, LPVOID lpTapeInformation );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetTapeParameters( HANDLE hDevice, DWORD dwOperation, LPDWORD lpdwSize, LPVOID lpTapeInformation )
{
    static FN_GetTapeParameters *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTapeParameters", &g_Kernel32);
    return pfn( hDevice, dwOperation, lpdwSize, lpTapeInformation );
}

typedef DWORD WINAPI FN_SetTapeParameters( HANDLE hDevice, DWORD dwOperation, LPVOID lpTapeInformation );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SetTapeParameters( HANDLE hDevice, DWORD dwOperation, LPVOID lpTapeInformation )
{
    static FN_SetTapeParameters *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetTapeParameters", &g_Kernel32);
    return pfn( hDevice, dwOperation, lpTapeInformation );
}

typedef BOOL WINAPI FN_Beep( DWORD dwFreq, DWORD dwDuration );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Beep( DWORD dwFreq, DWORD dwDuration )
{
    static FN_Beep *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Beep", &g_Kernel32);
    return pfn( dwFreq, dwDuration );
}

typedef int WINAPI FN_MulDiv( int nNumber, int nNumerator, int nDenominator );
__declspec(dllexport) int WINAPI kPrf2Wrap_MulDiv( int nNumber, int nNumerator, int nDenominator )
{
    static FN_MulDiv *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MulDiv", &g_Kernel32);
    return pfn( nNumber, nNumerator, nDenominator );
}

typedef VOID WINAPI FN_GetSystemTime( LPSYSTEMTIME lpSystemTime );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GetSystemTime( LPSYSTEMTIME lpSystemTime )
{
    static FN_GetSystemTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemTime", &g_Kernel32);
    pfn( lpSystemTime );
}

typedef VOID WINAPI FN_GetSystemTimeAsFileTime( LPFILETIME lpSystemTimeAsFileTime );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GetSystemTimeAsFileTime( LPFILETIME lpSystemTimeAsFileTime )
{
    static FN_GetSystemTimeAsFileTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemTimeAsFileTime", &g_Kernel32);
    pfn( lpSystemTimeAsFileTime );
}

typedef BOOL WINAPI FN_SetSystemTime( CONST SYSTEMTIME * lpSystemTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetSystemTime( CONST SYSTEMTIME * lpSystemTime )
{
    static FN_SetSystemTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSystemTime", &g_Kernel32);
    return pfn( lpSystemTime );
}

typedef VOID WINAPI FN_GetLocalTime( LPSYSTEMTIME lpSystemTime );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GetLocalTime( LPSYSTEMTIME lpSystemTime )
{
    static FN_GetLocalTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLocalTime", &g_Kernel32);
    pfn( lpSystemTime );
}

typedef BOOL WINAPI FN_SetLocalTime( CONST SYSTEMTIME * lpSystemTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetLocalTime( CONST SYSTEMTIME * lpSystemTime )
{
    static FN_SetLocalTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetLocalTime", &g_Kernel32);
    return pfn( lpSystemTime );
}

typedef VOID WINAPI FN_GetSystemInfo( LPSYSTEM_INFO lpSystemInfo );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GetSystemInfo( LPSYSTEM_INFO lpSystemInfo )
{
    static FN_GetSystemInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemInfo", &g_Kernel32);
    pfn( lpSystemInfo );
}

typedef BOOL WINAPI FN_SetSystemFileCacheSize( SIZE_T MinimumFileCacheSize, SIZE_T MaximumFileCacheSize, DWORD Flags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetSystemFileCacheSize( SIZE_T MinimumFileCacheSize, SIZE_T MaximumFileCacheSize, DWORD Flags )
{
    static FN_SetSystemFileCacheSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSystemFileCacheSize", &g_Kernel32);
    return pfn( MinimumFileCacheSize, MaximumFileCacheSize, Flags );
}

typedef BOOL WINAPI FN_GetSystemFileCacheSize( PSIZE_T lpMinimumFileCacheSize, PSIZE_T lpMaximumFileCacheSize, PDWORD lpFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSystemFileCacheSize( PSIZE_T lpMinimumFileCacheSize, PSIZE_T lpMaximumFileCacheSize, PDWORD lpFlags )
{
    static FN_GetSystemFileCacheSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemFileCacheSize", &g_Kernel32);
    return pfn( lpMinimumFileCacheSize, lpMaximumFileCacheSize, lpFlags );
}

typedef BOOL WINAPI FN_GetSystemRegistryQuota( PDWORD pdwQuotaAllowed, PDWORD pdwQuotaUsed );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSystemRegistryQuota( PDWORD pdwQuotaAllowed, PDWORD pdwQuotaUsed )
{
    static FN_GetSystemRegistryQuota *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemRegistryQuota", &g_Kernel32);
    return pfn( pdwQuotaAllowed, pdwQuotaUsed );
}

typedef BOOL WINAPI FN_GetSystemTimes( LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSystemTimes( LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime )
{
    static FN_GetSystemTimes *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemTimes", &g_Kernel32);
    return pfn( lpIdleTime, lpKernelTime, lpUserTime );
}

typedef VOID WINAPI FN_GetNativeSystemInfo( LPSYSTEM_INFO lpSystemInfo );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GetNativeSystemInfo( LPSYSTEM_INFO lpSystemInfo )
{
    static FN_GetNativeSystemInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNativeSystemInfo", &g_Kernel32);
    pfn( lpSystemInfo );
}

typedef BOOL WINAPI FN_IsProcessorFeaturePresent( DWORD ProcessorFeature );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsProcessorFeaturePresent( DWORD ProcessorFeature )
{
    static FN_IsProcessorFeaturePresent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsProcessorFeaturePresent", &g_Kernel32);
    return pfn( ProcessorFeature );
}

typedef BOOL WINAPI FN_SystemTimeToTzSpecificLocalTime( LPTIME_ZONE_INFORMATION lpTimeZoneInformation, LPSYSTEMTIME lpUniversalTime, LPSYSTEMTIME lpLocalTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SystemTimeToTzSpecificLocalTime( LPTIME_ZONE_INFORMATION lpTimeZoneInformation, LPSYSTEMTIME lpUniversalTime, LPSYSTEMTIME lpLocalTime )
{
    static FN_SystemTimeToTzSpecificLocalTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SystemTimeToTzSpecificLocalTime", &g_Kernel32);
    return pfn( lpTimeZoneInformation, lpUniversalTime, lpLocalTime );
}

typedef BOOL WINAPI FN_TzSpecificLocalTimeToSystemTime( LPTIME_ZONE_INFORMATION lpTimeZoneInformation, LPSYSTEMTIME lpLocalTime, LPSYSTEMTIME lpUniversalTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_TzSpecificLocalTimeToSystemTime( LPTIME_ZONE_INFORMATION lpTimeZoneInformation, LPSYSTEMTIME lpLocalTime, LPSYSTEMTIME lpUniversalTime )
{
    static FN_TzSpecificLocalTimeToSystemTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TzSpecificLocalTimeToSystemTime", &g_Kernel32);
    return pfn( lpTimeZoneInformation, lpLocalTime, lpUniversalTime );
}

typedef DWORD WINAPI FN_GetTimeZoneInformation( LPTIME_ZONE_INFORMATION lpTimeZoneInformation );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetTimeZoneInformation( LPTIME_ZONE_INFORMATION lpTimeZoneInformation )
{
    static FN_GetTimeZoneInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTimeZoneInformation", &g_Kernel32);
    return pfn( lpTimeZoneInformation );
}

typedef BOOL WINAPI FN_SetTimeZoneInformation( CONST TIME_ZONE_INFORMATION * lpTimeZoneInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetTimeZoneInformation( CONST TIME_ZONE_INFORMATION * lpTimeZoneInformation )
{
    static FN_SetTimeZoneInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetTimeZoneInformation", &g_Kernel32);
    return pfn( lpTimeZoneInformation );
}

typedef BOOL WINAPI FN_SystemTimeToFileTime( CONST SYSTEMTIME * lpSystemTime, LPFILETIME lpFileTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SystemTimeToFileTime( CONST SYSTEMTIME * lpSystemTime, LPFILETIME lpFileTime )
{
    static FN_SystemTimeToFileTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SystemTimeToFileTime", &g_Kernel32);
    return pfn( lpSystemTime, lpFileTime );
}

typedef BOOL WINAPI FN_FileTimeToLocalFileTime( CONST FILETIME * lpFileTime, LPFILETIME lpLocalFileTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FileTimeToLocalFileTime( CONST FILETIME * lpFileTime, LPFILETIME lpLocalFileTime )
{
    static FN_FileTimeToLocalFileTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FileTimeToLocalFileTime", &g_Kernel32);
    return pfn( lpFileTime, lpLocalFileTime );
}

typedef BOOL WINAPI FN_LocalFileTimeToFileTime( CONST FILETIME * lpLocalFileTime, LPFILETIME lpFileTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LocalFileTimeToFileTime( CONST FILETIME * lpLocalFileTime, LPFILETIME lpFileTime )
{
    static FN_LocalFileTimeToFileTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LocalFileTimeToFileTime", &g_Kernel32);
    return pfn( lpLocalFileTime, lpFileTime );
}

typedef BOOL WINAPI FN_FileTimeToSystemTime( CONST FILETIME * lpFileTime, LPSYSTEMTIME lpSystemTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FileTimeToSystemTime( CONST FILETIME * lpFileTime, LPSYSTEMTIME lpSystemTime )
{
    static FN_FileTimeToSystemTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FileTimeToSystemTime", &g_Kernel32);
    return pfn( lpFileTime, lpSystemTime );
}

typedef LONG WINAPI FN_CompareFileTime( CONST FILETIME * lpFileTime1, CONST FILETIME * lpFileTime2 );
__declspec(dllexport) LONG WINAPI kPrf2Wrap_CompareFileTime( CONST FILETIME * lpFileTime1, CONST FILETIME * lpFileTime2 )
{
    static FN_CompareFileTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CompareFileTime", &g_Kernel32);
    return pfn( lpFileTime1, lpFileTime2 );
}

typedef BOOL WINAPI FN_FileTimeToDosDateTime( CONST FILETIME * lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FileTimeToDosDateTime( CONST FILETIME * lpFileTime, LPWORD lpFatDate, LPWORD lpFatTime )
{
    static FN_FileTimeToDosDateTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FileTimeToDosDateTime", &g_Kernel32);
    return pfn( lpFileTime, lpFatDate, lpFatTime );
}

typedef BOOL WINAPI FN_DosDateTimeToFileTime( WORD wFatDate, WORD wFatTime, LPFILETIME lpFileTime );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DosDateTimeToFileTime( WORD wFatDate, WORD wFatTime, LPFILETIME lpFileTime )
{
    static FN_DosDateTimeToFileTime *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DosDateTimeToFileTime", &g_Kernel32);
    return pfn( wFatDate, wFatTime, lpFileTime );
}

typedef DWORD WINAPI FN_GetTickCount( VOID );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetTickCount( VOID )
{
    static FN_GetTickCount *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTickCount", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_SetSystemTimeAdjustment( DWORD dwTimeAdjustment, BOOL bTimeAdjustmentDisabled );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetSystemTimeAdjustment( DWORD dwTimeAdjustment, BOOL bTimeAdjustmentDisabled )
{
    static FN_SetSystemTimeAdjustment *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSystemTimeAdjustment", &g_Kernel32);
    return pfn( dwTimeAdjustment, bTimeAdjustmentDisabled );
}

typedef BOOL WINAPI FN_GetSystemTimeAdjustment( PDWORD lpTimeAdjustment, PDWORD lpTimeIncrement, PBOOL lpTimeAdjustmentDisabled );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSystemTimeAdjustment( PDWORD lpTimeAdjustment, PDWORD lpTimeIncrement, PBOOL lpTimeAdjustmentDisabled )
{
    static FN_GetSystemTimeAdjustment *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemTimeAdjustment", &g_Kernel32);
    return pfn( lpTimeAdjustment, lpTimeIncrement, lpTimeAdjustmentDisabled );
}

typedef DWORD WINAPI FN_FormatMessageA( DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, va_list * Arguments );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_FormatMessageA( DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, va_list * Arguments )
{
    static FN_FormatMessageA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FormatMessageA", &g_Kernel32);
    return pfn( dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments );
}

typedef DWORD WINAPI FN_FormatMessageW( DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize, va_list * Arguments );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_FormatMessageW( DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize, va_list * Arguments )
{
    static FN_FormatMessageW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FormatMessageW", &g_Kernel32);
    return pfn( dwFlags, lpSource, dwMessageId, dwLanguageId, lpBuffer, nSize, Arguments );
}

typedef BOOL WINAPI FN_CreatePipe( PHANDLE hReadPipe, PHANDLE hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreatePipe( PHANDLE hReadPipe, PHANDLE hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize )
{
    static FN_CreatePipe *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreatePipe", &g_Kernel32);
    return pfn( hReadPipe, hWritePipe, lpPipeAttributes, nSize );
}

typedef BOOL WINAPI FN_ConnectNamedPipe( HANDLE hNamedPipe, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ConnectNamedPipe( HANDLE hNamedPipe, LPOVERLAPPED lpOverlapped )
{
    static FN_ConnectNamedPipe *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ConnectNamedPipe", &g_Kernel32);
    return pfn( hNamedPipe, lpOverlapped );
}

typedef BOOL WINAPI FN_DisconnectNamedPipe( HANDLE hNamedPipe );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DisconnectNamedPipe( HANDLE hNamedPipe )
{
    static FN_DisconnectNamedPipe *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DisconnectNamedPipe", &g_Kernel32);
    return pfn( hNamedPipe );
}

typedef BOOL WINAPI FN_SetNamedPipeHandleState( HANDLE hNamedPipe, LPDWORD lpMode, LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetNamedPipeHandleState( HANDLE hNamedPipe, LPDWORD lpMode, LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout )
{
    static FN_SetNamedPipeHandleState *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetNamedPipeHandleState", &g_Kernel32);
    return pfn( hNamedPipe, lpMode, lpMaxCollectionCount, lpCollectDataTimeout );
}

typedef BOOL WINAPI FN_GetNamedPipeInfo( HANDLE hNamedPipe, LPDWORD lpFlags, LPDWORD lpOutBufferSize, LPDWORD lpInBufferSize, LPDWORD lpMaxInstances );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNamedPipeInfo( HANDLE hNamedPipe, LPDWORD lpFlags, LPDWORD lpOutBufferSize, LPDWORD lpInBufferSize, LPDWORD lpMaxInstances )
{
    static FN_GetNamedPipeInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNamedPipeInfo", &g_Kernel32);
    return pfn( hNamedPipe, lpFlags, lpOutBufferSize, lpInBufferSize, lpMaxInstances );
}

typedef BOOL WINAPI FN_PeekNamedPipe( HANDLE hNamedPipe, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesRead, LPDWORD lpTotalBytesAvail, LPDWORD lpBytesLeftThisMessage );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_PeekNamedPipe( HANDLE hNamedPipe, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesRead, LPDWORD lpTotalBytesAvail, LPDWORD lpBytesLeftThisMessage )
{
    static FN_PeekNamedPipe *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PeekNamedPipe", &g_Kernel32);
    return pfn( hNamedPipe, lpBuffer, nBufferSize, lpBytesRead, lpTotalBytesAvail, lpBytesLeftThisMessage );
}

typedef BOOL WINAPI FN_TransactNamedPipe( HANDLE hNamedPipe, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesRead, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_TransactNamedPipe( HANDLE hNamedPipe, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesRead, LPOVERLAPPED lpOverlapped )
{
    static FN_TransactNamedPipe *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TransactNamedPipe", &g_Kernel32);
    return pfn( hNamedPipe, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesRead, lpOverlapped );
}

typedef HANDLE WINAPI FN_CreateMailslotA( LPCSTR lpName, DWORD nMaxMessageSize, DWORD lReadTimeout, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateMailslotA( LPCSTR lpName, DWORD nMaxMessageSize, DWORD lReadTimeout, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateMailslotA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateMailslotA", &g_Kernel32);
    return pfn( lpName, nMaxMessageSize, lReadTimeout, lpSecurityAttributes );
}

typedef HANDLE WINAPI FN_CreateMailslotW( LPCWSTR lpName, DWORD nMaxMessageSize, DWORD lReadTimeout, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateMailslotW( LPCWSTR lpName, DWORD nMaxMessageSize, DWORD lReadTimeout, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateMailslotW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateMailslotW", &g_Kernel32);
    return pfn( lpName, nMaxMessageSize, lReadTimeout, lpSecurityAttributes );
}

typedef BOOL WINAPI FN_GetMailslotInfo( HANDLE hMailslot, LPDWORD lpMaxMessageSize, LPDWORD lpNextSize, LPDWORD lpMessageCount, LPDWORD lpReadTimeout );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetMailslotInfo( HANDLE hMailslot, LPDWORD lpMaxMessageSize, LPDWORD lpNextSize, LPDWORD lpMessageCount, LPDWORD lpReadTimeout )
{
    static FN_GetMailslotInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetMailslotInfo", &g_Kernel32);
    return pfn( hMailslot, lpMaxMessageSize, lpNextSize, lpMessageCount, lpReadTimeout );
}

typedef BOOL WINAPI FN_SetMailslotInfo( HANDLE hMailslot, DWORD lReadTimeout );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetMailslotInfo( HANDLE hMailslot, DWORD lReadTimeout )
{
    static FN_SetMailslotInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetMailslotInfo", &g_Kernel32);
    return pfn( hMailslot, lReadTimeout );
}

typedef LPVOID WINAPI FN_MapViewOfFile( HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_MapViewOfFile( HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap )
{
    static FN_MapViewOfFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MapViewOfFile", &g_Kernel32);
    return pfn( hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap );
}

typedef BOOL WINAPI FN_FlushViewOfFile( LPCVOID lpBaseAddress, SIZE_T dwNumberOfBytesToFlush );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FlushViewOfFile( LPCVOID lpBaseAddress, SIZE_T dwNumberOfBytesToFlush )
{
    static FN_FlushViewOfFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FlushViewOfFile", &g_Kernel32);
    return pfn( lpBaseAddress, dwNumberOfBytesToFlush );
}

typedef BOOL WINAPI FN_UnmapViewOfFile( LPCVOID lpBaseAddress );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_UnmapViewOfFile( LPCVOID lpBaseAddress )
{
    static FN_UnmapViewOfFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "UnmapViewOfFile", &g_Kernel32);
    return pfn( lpBaseAddress );
}

typedef BOOL WINAPI FN_EncryptFileA( LPCSTR lpFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EncryptFileA( LPCSTR lpFileName )
{
    static FN_EncryptFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EncryptFileA", &g_Kernel32);
    return pfn( lpFileName );
}

typedef BOOL WINAPI FN_EncryptFileW( LPCWSTR lpFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EncryptFileW( LPCWSTR lpFileName )
{
    static FN_EncryptFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EncryptFileW", &g_Kernel32);
    return pfn( lpFileName );
}

typedef BOOL WINAPI FN_DecryptFileA( LPCSTR lpFileName, DWORD dwReserved );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DecryptFileA( LPCSTR lpFileName, DWORD dwReserved )
{
    static FN_DecryptFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DecryptFileA", &g_Kernel32);
    return pfn( lpFileName, dwReserved );
}

typedef BOOL WINAPI FN_DecryptFileW( LPCWSTR lpFileName, DWORD dwReserved );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DecryptFileW( LPCWSTR lpFileName, DWORD dwReserved )
{
    static FN_DecryptFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DecryptFileW", &g_Kernel32);
    return pfn( lpFileName, dwReserved );
}

typedef BOOL WINAPI FN_FileEncryptionStatusA( LPCSTR lpFileName, LPDWORD lpStatus );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FileEncryptionStatusA( LPCSTR lpFileName, LPDWORD lpStatus )
{
    static FN_FileEncryptionStatusA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FileEncryptionStatusA", &g_Kernel32);
    return pfn( lpFileName, lpStatus );
}

typedef BOOL WINAPI FN_FileEncryptionStatusW( LPCWSTR lpFileName, LPDWORD lpStatus );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FileEncryptionStatusW( LPCWSTR lpFileName, LPDWORD lpStatus )
{
    static FN_FileEncryptionStatusW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FileEncryptionStatusW", &g_Kernel32);
    return pfn( lpFileName, lpStatus );
}

typedef DWORD WINAPI FN_OpenEncryptedFileRawA( LPCSTR lpFileName, ULONG ulFlags, PVOID * pvContext );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_OpenEncryptedFileRawA( LPCSTR lpFileName, ULONG ulFlags, PVOID * pvContext )
{
    static FN_OpenEncryptedFileRawA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenEncryptedFileRawA", &g_Kernel32);
    return pfn( lpFileName, ulFlags, pvContext );
}

typedef DWORD WINAPI FN_OpenEncryptedFileRawW( LPCWSTR lpFileName, ULONG ulFlags, PVOID * pvContext );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_OpenEncryptedFileRawW( LPCWSTR lpFileName, ULONG ulFlags, PVOID * pvContext )
{
    static FN_OpenEncryptedFileRawW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenEncryptedFileRawW", &g_Kernel32);
    return pfn( lpFileName, ulFlags, pvContext );
}

typedef DWORD WINAPI FN_ReadEncryptedFileRaw( PFE_EXPORT_FUNC pfExportCallback, PVOID pvCallbackContext, PVOID pvContext );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_ReadEncryptedFileRaw( PFE_EXPORT_FUNC pfExportCallback, PVOID pvCallbackContext, PVOID pvContext )
{
    static FN_ReadEncryptedFileRaw *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadEncryptedFileRaw", &g_Kernel32);
    return pfn( pfExportCallback, pvCallbackContext, pvContext );
}

typedef DWORD WINAPI FN_WriteEncryptedFileRaw( PFE_IMPORT_FUNC pfImportCallback, PVOID pvCallbackContext, PVOID pvContext );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_WriteEncryptedFileRaw( PFE_IMPORT_FUNC pfImportCallback, PVOID pvCallbackContext, PVOID pvContext )
{
    static FN_WriteEncryptedFileRaw *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteEncryptedFileRaw", &g_Kernel32);
    return pfn( pfImportCallback, pvCallbackContext, pvContext );
}

typedef VOID WINAPI FN_CloseEncryptedFileRaw( PVOID pvContext );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_CloseEncryptedFileRaw( PVOID pvContext )
{
    static FN_CloseEncryptedFileRaw *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CloseEncryptedFileRaw", &g_Kernel32);
    pfn( pvContext );
}

typedef int WINAPI FN_lstrcmpA( LPCSTR lpString1, LPCSTR lpString2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_lstrcmpA( LPCSTR lpString1, LPCSTR lpString2 )
{
    static FN_lstrcmpA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcmpA", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef int WINAPI FN_lstrcmpW( LPCWSTR lpString1, LPCWSTR lpString2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_lstrcmpW( LPCWSTR lpString1, LPCWSTR lpString2 )
{
    static FN_lstrcmpW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcmpW", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef int WINAPI FN_lstrcmpiA( LPCSTR lpString1, LPCSTR lpString2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_lstrcmpiA( LPCSTR lpString1, LPCSTR lpString2 )
{
    static FN_lstrcmpiA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcmpiA", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef int WINAPI FN_lstrcmpiW( LPCWSTR lpString1, LPCWSTR lpString2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_lstrcmpiW( LPCWSTR lpString1, LPCWSTR lpString2 )
{
    static FN_lstrcmpiW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcmpiW", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef LPSTR WINAPI FN_lstrcpynA( LPSTR lpString1, LPCSTR lpString2, int iMaxLength );
__declspec(dllexport) LPSTR WINAPI kPrf2Wrap_lstrcpynA( LPSTR lpString1, LPCSTR lpString2, int iMaxLength )
{
    static FN_lstrcpynA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcpynA", &g_Kernel32);
    return pfn( lpString1, lpString2, iMaxLength );
}

typedef LPWSTR WINAPI FN_lstrcpynW( LPWSTR lpString1, LPCWSTR lpString2, int iMaxLength );
__declspec(dllexport) LPWSTR WINAPI kPrf2Wrap_lstrcpynW( LPWSTR lpString1, LPCWSTR lpString2, int iMaxLength )
{
    static FN_lstrcpynW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcpynW", &g_Kernel32);
    return pfn( lpString1, lpString2, iMaxLength );
}

typedef LPSTR WINAPI FN_lstrcpyA( LPSTR lpString1, LPCSTR lpString2 );
__declspec(dllexport) LPSTR WINAPI kPrf2Wrap_lstrcpyA( LPSTR lpString1, LPCSTR lpString2 )
{
    static FN_lstrcpyA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcpyA", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef LPWSTR WINAPI FN_lstrcpyW( LPWSTR lpString1, LPCWSTR lpString2 );
__declspec(dllexport) LPWSTR WINAPI kPrf2Wrap_lstrcpyW( LPWSTR lpString1, LPCWSTR lpString2 )
{
    static FN_lstrcpyW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcpyW", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef LPSTR WINAPI FN_lstrcatA( LPSTR lpString1, LPCSTR lpString2 );
__declspec(dllexport) LPSTR WINAPI kPrf2Wrap_lstrcatA( LPSTR lpString1, LPCSTR lpString2 )
{
    static FN_lstrcatA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcatA", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef LPWSTR WINAPI FN_lstrcatW( LPWSTR lpString1, LPCWSTR lpString2 );
__declspec(dllexport) LPWSTR WINAPI kPrf2Wrap_lstrcatW( LPWSTR lpString1, LPCWSTR lpString2 )
{
    static FN_lstrcatW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcatW", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef int WINAPI FN_lstrlenA( LPCSTR lpString );
__declspec(dllexport) int WINAPI kPrf2Wrap_lstrlenA( LPCSTR lpString )
{
    static FN_lstrlenA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrlenA", &g_Kernel32);
    return pfn( lpString );
}

typedef int WINAPI FN_lstrlenW( LPCWSTR lpString );
__declspec(dllexport) int WINAPI kPrf2Wrap_lstrlenW( LPCWSTR lpString )
{
    static FN_lstrlenW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrlenW", &g_Kernel32);
    return pfn( lpString );
}

typedef HFILE WINAPI FN_OpenFile( LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle );
__declspec(dllexport) HFILE WINAPI kPrf2Wrap_OpenFile( LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle )
{
    static FN_OpenFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenFile", &g_Kernel32);
    return pfn( lpFileName, lpReOpenBuff, uStyle );
}

typedef HFILE WINAPI FN__lopen( LPCSTR lpPathName, int iReadWrite );
__declspec(dllexport) HFILE WINAPI kPrf2Wrap__lopen( LPCSTR lpPathName, int iReadWrite )
{
    static FN__lopen *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "_lopen", &g_Kernel32);
    return pfn( lpPathName, iReadWrite );
}

typedef HFILE WINAPI FN__lcreat( LPCSTR lpPathName, int iAttribute );
__declspec(dllexport) HFILE WINAPI kPrf2Wrap__lcreat( LPCSTR lpPathName, int iAttribute )
{
    static FN__lcreat *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "_lcreat", &g_Kernel32);
    return pfn( lpPathName, iAttribute );
}

typedef UINT WINAPI FN__lread( HFILE hFile, LPVOID lpBuffer, UINT uBytes );
__declspec(dllexport) UINT WINAPI kPrf2Wrap__lread( HFILE hFile, LPVOID lpBuffer, UINT uBytes )
{
    static FN__lread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "_lread", &g_Kernel32);
    return pfn( hFile, lpBuffer, uBytes );
}

typedef UINT WINAPI FN__lwrite( HFILE hFile, LPCCH lpBuffer, UINT uBytes );
__declspec(dllexport) UINT WINAPI kPrf2Wrap__lwrite( HFILE hFile, LPCCH lpBuffer, UINT uBytes )
{
    static FN__lwrite *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "_lwrite", &g_Kernel32);
    return pfn( hFile, lpBuffer, uBytes );
}

typedef long WINAPI FN__hread( HFILE hFile, LPVOID lpBuffer, long lBytes );
__declspec(dllexport) long WINAPI kPrf2Wrap__hread( HFILE hFile, LPVOID lpBuffer, long lBytes )
{
    static FN__hread *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "_hread", &g_Kernel32);
    return pfn( hFile, lpBuffer, lBytes );
}

typedef long WINAPI FN__hwrite( HFILE hFile, LPCCH lpBuffer, long lBytes );
__declspec(dllexport) long WINAPI kPrf2Wrap__hwrite( HFILE hFile, LPCCH lpBuffer, long lBytes )
{
    static FN__hwrite *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "_hwrite", &g_Kernel32);
    return pfn( hFile, lpBuffer, lBytes );
}

typedef HFILE WINAPI FN__lclose( HFILE hFile );
__declspec(dllexport) HFILE WINAPI kPrf2Wrap__lclose( HFILE hFile )
{
    static FN__lclose *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "_lclose", &g_Kernel32);
    return pfn( hFile );
}

typedef LONG WINAPI FN__llseek( HFILE hFile, LONG lOffset, int iOrigin );
__declspec(dllexport) LONG WINAPI kPrf2Wrap__llseek( HFILE hFile, LONG lOffset, int iOrigin )
{
    static FN__llseek *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "_llseek", &g_Kernel32);
    return pfn( hFile, lOffset, iOrigin );
}

typedef BOOL WINAPI FN_IsTextUnicode( CONST VOID * lpv, int iSize, LPINT lpiResult );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsTextUnicode( CONST VOID * lpv, int iSize, LPINT lpiResult )
{
    static FN_IsTextUnicode *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsTextUnicode", &g_Kernel32);
    return pfn( lpv, iSize, lpiResult );
}

typedef DWORD WINAPI FN_FlsAlloc( PFLS_CALLBACK_FUNCTION lpCallback );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_FlsAlloc( PFLS_CALLBACK_FUNCTION lpCallback )
{
    static FN_FlsAlloc *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FlsAlloc", &g_Kernel32);
    return pfn( lpCallback );
}

typedef PVOID WINAPI FN_FlsGetValue( DWORD dwFlsIndex );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_FlsGetValue( DWORD dwFlsIndex )
{
    static FN_FlsGetValue *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FlsGetValue", &g_Kernel32);
    return pfn( dwFlsIndex );
}

typedef BOOL WINAPI FN_FlsSetValue( DWORD dwFlsIndex, PVOID lpFlsData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FlsSetValue( DWORD dwFlsIndex, PVOID lpFlsData )
{
    static FN_FlsSetValue *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FlsSetValue", &g_Kernel32);
    return pfn( dwFlsIndex, lpFlsData );
}

typedef BOOL WINAPI FN_FlsFree( DWORD dwFlsIndex );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FlsFree( DWORD dwFlsIndex )
{
    static FN_FlsFree *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FlsFree", &g_Kernel32);
    return pfn( dwFlsIndex );
}

typedef DWORD WINAPI FN_TlsAlloc( VOID );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_TlsAlloc( VOID )
{
    static FN_TlsAlloc *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TlsAlloc", &g_Kernel32);
    return pfn ();
}

typedef LPVOID WINAPI FN_TlsGetValue( DWORD dwTlsIndex );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_TlsGetValue( DWORD dwTlsIndex )
{
    static FN_TlsGetValue *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TlsGetValue", &g_Kernel32);
    return pfn( dwTlsIndex );
}

typedef BOOL WINAPI FN_TlsSetValue( DWORD dwTlsIndex, LPVOID lpTlsValue );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_TlsSetValue( DWORD dwTlsIndex, LPVOID lpTlsValue )
{
    static FN_TlsSetValue *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TlsSetValue", &g_Kernel32);
    return pfn( dwTlsIndex, lpTlsValue );
}

typedef BOOL WINAPI FN_TlsFree( DWORD dwTlsIndex );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_TlsFree( DWORD dwTlsIndex )
{
    static FN_TlsFree *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TlsFree", &g_Kernel32);
    return pfn( dwTlsIndex );
}

typedef DWORD WINAPI FN_SleepEx( DWORD dwMilliseconds, BOOL bAlertable );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SleepEx( DWORD dwMilliseconds, BOOL bAlertable )
{
    static FN_SleepEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SleepEx", &g_Kernel32);
    return pfn( dwMilliseconds, bAlertable );
}

typedef DWORD WINAPI FN_WaitForSingleObjectEx( HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_WaitForSingleObjectEx( HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable )
{
    static FN_WaitForSingleObjectEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WaitForSingleObjectEx", &g_Kernel32);
    return pfn( hHandle, dwMilliseconds, bAlertable );
}

typedef DWORD WINAPI FN_WaitForMultipleObjectsEx( DWORD nCount, CONST HANDLE * lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_WaitForMultipleObjectsEx( DWORD nCount, CONST HANDLE * lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable )
{
    static FN_WaitForMultipleObjectsEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WaitForMultipleObjectsEx", &g_Kernel32);
    return pfn( nCount, lpHandles, bWaitAll, dwMilliseconds, bAlertable );
}

typedef DWORD WINAPI FN_SignalObjectAndWait( HANDLE hObjectToSignal, HANDLE hObjectToWaitOn, DWORD dwMilliseconds, BOOL bAlertable );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SignalObjectAndWait( HANDLE hObjectToSignal, HANDLE hObjectToWaitOn, DWORD dwMilliseconds, BOOL bAlertable )
{
    static FN_SignalObjectAndWait *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SignalObjectAndWait", &g_Kernel32);
    return pfn( hObjectToSignal, hObjectToWaitOn, dwMilliseconds, bAlertable );
}

typedef BOOL WINAPI FN_ReadFileEx( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadFileEx( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
    static FN_ReadFileEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadFileEx", &g_Kernel32);
    return pfn( hFile, lpBuffer, nNumberOfBytesToRead, lpOverlapped, lpCompletionRoutine );
}

typedef BOOL WINAPI FN_WriteFileEx( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteFileEx( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
    static FN_WriteFileEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteFileEx", &g_Kernel32);
    return pfn( hFile, lpBuffer, nNumberOfBytesToWrite, lpOverlapped, lpCompletionRoutine );
}

typedef BOOL WINAPI FN_BackupRead( HANDLE hFile, LPBYTE lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, BOOL bAbort, BOOL bProcessSecurity, LPVOID * lpContext );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BackupRead( HANDLE hFile, LPBYTE lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, BOOL bAbort, BOOL bProcessSecurity, LPVOID * lpContext )
{
    static FN_BackupRead *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BackupRead", &g_Kernel32);
    return pfn( hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, bAbort, bProcessSecurity, lpContext );
}

typedef BOOL WINAPI FN_BackupSeek( HANDLE hFile, DWORD dwLowBytesToSeek, DWORD dwHighBytesToSeek, LPDWORD lpdwLowByteSeeked, LPDWORD lpdwHighByteSeeked, LPVOID * lpContext );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BackupSeek( HANDLE hFile, DWORD dwLowBytesToSeek, DWORD dwHighBytesToSeek, LPDWORD lpdwLowByteSeeked, LPDWORD lpdwHighByteSeeked, LPVOID * lpContext )
{
    static FN_BackupSeek *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BackupSeek", &g_Kernel32);
    return pfn( hFile, dwLowBytesToSeek, dwHighBytesToSeek, lpdwLowByteSeeked, lpdwHighByteSeeked, lpContext );
}

typedef BOOL WINAPI FN_BackupWrite( HANDLE hFile, LPBYTE lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, BOOL bAbort, BOOL bProcessSecurity, LPVOID * lpContext );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BackupWrite( HANDLE hFile, LPBYTE lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, BOOL bAbort, BOOL bProcessSecurity, LPVOID * lpContext )
{
    static FN_BackupWrite *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BackupWrite", &g_Kernel32);
    return pfn( hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, bAbort, bProcessSecurity, lpContext );
}

typedef BOOL WINAPI FN_ReadFileScatter( HANDLE hFile, FILE_SEGMENT_ELEMENT aSegmentArray[], DWORD nNumberOfBytesToRead, LPDWORD lpReserved, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadFileScatter( HANDLE hFile, FILE_SEGMENT_ELEMENT aSegmentArray[], DWORD nNumberOfBytesToRead, LPDWORD lpReserved, LPOVERLAPPED lpOverlapped )
{
    static FN_ReadFileScatter *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadFileScatter", &g_Kernel32);
    return pfn( hFile, aSegmentArray, nNumberOfBytesToRead, lpReserved, lpOverlapped );
}

typedef BOOL WINAPI FN_WriteFileGather( HANDLE hFile, FILE_SEGMENT_ELEMENT aSegmentArray[], DWORD nNumberOfBytesToWrite, LPDWORD lpReserved, LPOVERLAPPED lpOverlapped );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteFileGather( HANDLE hFile, FILE_SEGMENT_ELEMENT aSegmentArray[], DWORD nNumberOfBytesToWrite, LPDWORD lpReserved, LPOVERLAPPED lpOverlapped )
{
    static FN_WriteFileGather *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteFileGather", &g_Kernel32);
    return pfn( hFile, aSegmentArray, nNumberOfBytesToWrite, lpReserved, lpOverlapped );
}

typedef HANDLE WINAPI FN_CreateMutexA( LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateMutexA( LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName )
{
    static FN_CreateMutexA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateMutexA", &g_Kernel32);
    return pfn( lpMutexAttributes, bInitialOwner, lpName );
}

typedef HANDLE WINAPI FN_CreateMutexW( LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateMutexW( LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCWSTR lpName )
{
    static FN_CreateMutexW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateMutexW", &g_Kernel32);
    return pfn( lpMutexAttributes, bInitialOwner, lpName );
}

typedef HANDLE WINAPI FN_OpenMutexA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenMutexA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName )
{
    static FN_OpenMutexA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenMutexA", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef HANDLE WINAPI FN_OpenMutexW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenMutexW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName )
{
    static FN_OpenMutexW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenMutexW", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef HANDLE WINAPI FN_CreateEventA( LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateEventA( LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName )
{
    static FN_CreateEventA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateEventA", &g_Kernel32);
    return pfn( lpEventAttributes, bManualReset, bInitialState, lpName );
}

typedef HANDLE WINAPI FN_CreateEventW( LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateEventW( LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName )
{
    static FN_CreateEventW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateEventW", &g_Kernel32);
    return pfn( lpEventAttributes, bManualReset, bInitialState, lpName );
}

typedef HANDLE WINAPI FN_OpenEventA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenEventA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName )
{
    static FN_OpenEventA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenEventA", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef HANDLE WINAPI FN_OpenEventW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenEventW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName )
{
    static FN_OpenEventW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenEventW", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef HANDLE WINAPI FN_CreateSemaphoreA( LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateSemaphoreA( LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName )
{
    static FN_CreateSemaphoreA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateSemaphoreA", &g_Kernel32);
    return pfn( lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName );
}

typedef HANDLE WINAPI FN_CreateSemaphoreW( LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateSemaphoreW( LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName )
{
    static FN_CreateSemaphoreW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateSemaphoreW", &g_Kernel32);
    return pfn( lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName );
}

typedef HANDLE WINAPI FN_OpenSemaphoreA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenSemaphoreA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName )
{
    static FN_OpenSemaphoreA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenSemaphoreA", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef HANDLE WINAPI FN_OpenSemaphoreW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenSemaphoreW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName )
{
    static FN_OpenSemaphoreW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenSemaphoreW", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef HANDLE WINAPI FN_CreateWaitableTimerA( LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCSTR lpTimerName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateWaitableTimerA( LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCSTR lpTimerName )
{
    static FN_CreateWaitableTimerA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateWaitableTimerA", &g_Kernel32);
    return pfn( lpTimerAttributes, bManualReset, lpTimerName );
}

typedef HANDLE WINAPI FN_CreateWaitableTimerW( LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCWSTR lpTimerName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateWaitableTimerW( LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCWSTR lpTimerName )
{
    static FN_CreateWaitableTimerW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateWaitableTimerW", &g_Kernel32);
    return pfn( lpTimerAttributes, bManualReset, lpTimerName );
}

typedef HANDLE WINAPI FN_OpenWaitableTimerA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpTimerName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenWaitableTimerA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpTimerName )
{
    static FN_OpenWaitableTimerA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenWaitableTimerA", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpTimerName );
}

typedef HANDLE WINAPI FN_OpenWaitableTimerW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpTimerName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenWaitableTimerW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpTimerName )
{
    static FN_OpenWaitableTimerW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenWaitableTimerW", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpTimerName );
}

typedef BOOL WINAPI FN_SetWaitableTimer( HANDLE hTimer, const LARGE_INTEGER * lpDueTime, LONG lPeriod, PTIMERAPCROUTINE pfnCompletionRoutine, LPVOID lpArgToCompletionRoutine, BOOL fResume );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetWaitableTimer( HANDLE hTimer, const LARGE_INTEGER * lpDueTime, LONG lPeriod, PTIMERAPCROUTINE pfnCompletionRoutine, LPVOID lpArgToCompletionRoutine, BOOL fResume )
{
    static FN_SetWaitableTimer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetWaitableTimer", &g_Kernel32);
    return pfn( hTimer, lpDueTime, lPeriod, pfnCompletionRoutine, lpArgToCompletionRoutine, fResume );
}

typedef BOOL WINAPI FN_CancelWaitableTimer( HANDLE hTimer );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CancelWaitableTimer( HANDLE hTimer )
{
    static FN_CancelWaitableTimer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CancelWaitableTimer", &g_Kernel32);
    return pfn( hTimer );
}

typedef HANDLE WINAPI FN_CreateFileMappingA( HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateFileMappingA( HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName )
{
    static FN_CreateFileMappingA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateFileMappingA", &g_Kernel32);
    return pfn( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName );
}

typedef HANDLE WINAPI FN_CreateFileMappingW( HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateFileMappingW( HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCWSTR lpName )
{
    static FN_CreateFileMappingW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateFileMappingW", &g_Kernel32);
    return pfn( hFile, lpFileMappingAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName );
}

typedef HANDLE WINAPI FN_OpenFileMappingA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenFileMappingA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName )
{
    static FN_OpenFileMappingA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenFileMappingA", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef HANDLE WINAPI FN_OpenFileMappingW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenFileMappingW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName )
{
    static FN_OpenFileMappingW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenFileMappingW", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef DWORD WINAPI FN_GetLogicalDriveStringsA( DWORD nBufferLength, LPSTR lpBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetLogicalDriveStringsA( DWORD nBufferLength, LPSTR lpBuffer )
{
    static FN_GetLogicalDriveStringsA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLogicalDriveStringsA", &g_Kernel32);
    return pfn( nBufferLength, lpBuffer );
}

typedef DWORD WINAPI FN_GetLogicalDriveStringsW( DWORD nBufferLength, LPWSTR lpBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetLogicalDriveStringsW( DWORD nBufferLength, LPWSTR lpBuffer )
{
    static FN_GetLogicalDriveStringsW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLogicalDriveStringsW", &g_Kernel32);
    return pfn( nBufferLength, lpBuffer );
}

typedef HANDLE WINAPI FN_CreateMemoryResourceNotification( MEMORY_RESOURCE_NOTIFICATION_TYPE NotificationType );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateMemoryResourceNotification( MEMORY_RESOURCE_NOTIFICATION_TYPE NotificationType )
{
    static FN_CreateMemoryResourceNotification *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateMemoryResourceNotification", &g_Kernel32);
    return pfn( NotificationType );
}

typedef BOOL WINAPI FN_QueryMemoryResourceNotification( HANDLE ResourceNotificationHandle, PBOOL ResourceState );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_QueryMemoryResourceNotification( HANDLE ResourceNotificationHandle, PBOOL ResourceState )
{
    static FN_QueryMemoryResourceNotification *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueryMemoryResourceNotification", &g_Kernel32);
    return pfn( ResourceNotificationHandle, ResourceState );
}

typedef HMODULE WINAPI FN_LoadLibraryA( LPCSTR lpLibFileName );
__declspec(dllexport) HMODULE WINAPI kPrf2Wrap_LoadLibraryA( LPCSTR lpLibFileName )
{
    static FN_LoadLibraryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LoadLibraryA", &g_Kernel32);
    return pfn( lpLibFileName );
}

typedef HMODULE WINAPI FN_LoadLibraryW( LPCWSTR lpLibFileName );
__declspec(dllexport) HMODULE WINAPI kPrf2Wrap_LoadLibraryW( LPCWSTR lpLibFileName )
{
    static FN_LoadLibraryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LoadLibraryW", &g_Kernel32);
    return pfn( lpLibFileName );
}

typedef HMODULE WINAPI FN_LoadLibraryExA( LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags );
__declspec(dllexport) HMODULE WINAPI kPrf2Wrap_LoadLibraryExA( LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags )
{
    static FN_LoadLibraryExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LoadLibraryExA", &g_Kernel32);
    return pfn( lpLibFileName, hFile, dwFlags );
}

typedef HMODULE WINAPI FN_LoadLibraryExW( LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags );
__declspec(dllexport) HMODULE WINAPI kPrf2Wrap_LoadLibraryExW( LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags )
{
    static FN_LoadLibraryExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LoadLibraryExW", &g_Kernel32);
    return pfn( lpLibFileName, hFile, dwFlags );
}

typedef DWORD WINAPI FN_GetModuleFileNameA( HMODULE hModule, LPCH lpFilename, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetModuleFileNameA( HMODULE hModule, LPCH lpFilename, DWORD nSize )
{
    static FN_GetModuleFileNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetModuleFileNameA", &g_Kernel32);
    return pfn( hModule, lpFilename, nSize );
}

typedef DWORD WINAPI FN_GetModuleFileNameW( HMODULE hModule, LPWCH lpFilename, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetModuleFileNameW( HMODULE hModule, LPWCH lpFilename, DWORD nSize )
{
    static FN_GetModuleFileNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetModuleFileNameW", &g_Kernel32);
    return pfn( hModule, lpFilename, nSize );
}

typedef HMODULE WINAPI FN_GetModuleHandleA( LPCSTR lpModuleName );
__declspec(dllexport) HMODULE WINAPI kPrf2Wrap_GetModuleHandleA( LPCSTR lpModuleName )
{
    static FN_GetModuleHandleA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetModuleHandleA", &g_Kernel32);
    return pfn( lpModuleName );
}

typedef HMODULE WINAPI FN_GetModuleHandleW( LPCWSTR lpModuleName );
__declspec(dllexport) HMODULE WINAPI kPrf2Wrap_GetModuleHandleW( LPCWSTR lpModuleName )
{
    static FN_GetModuleHandleW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetModuleHandleW", &g_Kernel32);
    return pfn( lpModuleName );
}

typedef BOOL WINAPI FN_GetModuleHandleExA( DWORD dwFlags, LPCSTR lpModuleName, HMODULE * phModule );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetModuleHandleExA( DWORD dwFlags, LPCSTR lpModuleName, HMODULE * phModule )
{
    static FN_GetModuleHandleExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetModuleHandleExA", &g_Kernel32);
    return pfn( dwFlags, lpModuleName, phModule );
}

typedef BOOL WINAPI FN_GetModuleHandleExW( DWORD dwFlags, LPCWSTR lpModuleName, HMODULE * phModule );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetModuleHandleExW( DWORD dwFlags, LPCWSTR lpModuleName, HMODULE * phModule )
{
    static FN_GetModuleHandleExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetModuleHandleExW", &g_Kernel32);
    return pfn( dwFlags, lpModuleName, phModule );
}

typedef BOOL WINAPI FN_NeedCurrentDirectoryForExePathA( LPCSTR ExeName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_NeedCurrentDirectoryForExePathA( LPCSTR ExeName )
{
    static FN_NeedCurrentDirectoryForExePathA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "NeedCurrentDirectoryForExePathA", &g_Kernel32);
    return pfn( ExeName );
}

typedef BOOL WINAPI FN_NeedCurrentDirectoryForExePathW( LPCWSTR ExeName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_NeedCurrentDirectoryForExePathW( LPCWSTR ExeName )
{
    static FN_NeedCurrentDirectoryForExePathW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "NeedCurrentDirectoryForExePathW", &g_Kernel32);
    return pfn( ExeName );
}

typedef BOOL WINAPI FN_CreateProcessA( LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateProcessA( LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation )
{
    static FN_CreateProcessA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateProcessA", &g_Kernel32);
    return pfn( lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation );
}

typedef BOOL WINAPI FN_CreateProcessW( LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateProcessW( LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation )
{
    static FN_CreateProcessW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateProcessW", &g_Kernel32);
    return pfn( lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation );
}

typedef BOOL WINAPI FN_SetProcessShutdownParameters( DWORD dwLevel, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetProcessShutdownParameters( DWORD dwLevel, DWORD dwFlags )
{
    static FN_SetProcessShutdownParameters *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetProcessShutdownParameters", &g_Kernel32);
    return pfn( dwLevel, dwFlags );
}

typedef BOOL WINAPI FN_GetProcessShutdownParameters( LPDWORD lpdwLevel, LPDWORD lpdwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetProcessShutdownParameters( LPDWORD lpdwLevel, LPDWORD lpdwFlags )
{
    static FN_GetProcessShutdownParameters *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessShutdownParameters", &g_Kernel32);
    return pfn( lpdwLevel, lpdwFlags );
}

typedef DWORD WINAPI FN_GetProcessVersion( DWORD ProcessId );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetProcessVersion( DWORD ProcessId )
{
    static FN_GetProcessVersion *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProcessVersion", &g_Kernel32);
    return pfn( ProcessId );
}

typedef VOID WINAPI FN_FatalAppExitA( UINT uAction, LPCSTR lpMessageText );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_FatalAppExitA( UINT uAction, LPCSTR lpMessageText )
{
    static FN_FatalAppExitA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FatalAppExitA", &g_Kernel32);
    pfn( uAction, lpMessageText );
}

typedef VOID WINAPI FN_FatalAppExitW( UINT uAction, LPCWSTR lpMessageText );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_FatalAppExitW( UINT uAction, LPCWSTR lpMessageText )
{
    static FN_FatalAppExitW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FatalAppExitW", &g_Kernel32);
    pfn( uAction, lpMessageText );
}

typedef VOID WINAPI FN_GetStartupInfoA( LPSTARTUPINFOA lpStartupInfo );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GetStartupInfoA( LPSTARTUPINFOA lpStartupInfo )
{
    static FN_GetStartupInfoA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetStartupInfoA", &g_Kernel32);
    pfn( lpStartupInfo );
}

typedef VOID WINAPI FN_GetStartupInfoW( LPSTARTUPINFOW lpStartupInfo );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_GetStartupInfoW( LPSTARTUPINFOW lpStartupInfo )
{
    static FN_GetStartupInfoW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetStartupInfoW", &g_Kernel32);
    pfn( lpStartupInfo );
}

typedef LPSTR WINAPI FN_GetCommandLineA( VOID );
__declspec(dllexport) LPSTR WINAPI kPrf2Wrap_GetCommandLineA( VOID )
{
    static FN_GetCommandLineA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCommandLineA", &g_Kernel32);
    return pfn ();
}

typedef LPWSTR WINAPI FN_GetCommandLineW( VOID );
__declspec(dllexport) LPWSTR WINAPI kPrf2Wrap_GetCommandLineW( VOID )
{
    static FN_GetCommandLineW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCommandLineW", &g_Kernel32);
    return pfn ();
}

typedef DWORD WINAPI FN_GetEnvironmentVariableA( LPCSTR lpName, LPSTR lpBuffer, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetEnvironmentVariableA( LPCSTR lpName, LPSTR lpBuffer, DWORD nSize )
{
    static FN_GetEnvironmentVariableA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetEnvironmentVariableA", &g_Kernel32);
    return pfn( lpName, lpBuffer, nSize );
}

typedef DWORD WINAPI FN_GetEnvironmentVariableW( LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetEnvironmentVariableW( LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize )
{
    static FN_GetEnvironmentVariableW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetEnvironmentVariableW", &g_Kernel32);
    return pfn( lpName, lpBuffer, nSize );
}

typedef BOOL WINAPI FN_SetEnvironmentVariableA( LPCSTR lpName, LPCSTR lpValue );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetEnvironmentVariableA( LPCSTR lpName, LPCSTR lpValue )
{
    static FN_SetEnvironmentVariableA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetEnvironmentVariableA", &g_Kernel32);
    return pfn( lpName, lpValue );
}

typedef BOOL WINAPI FN_SetEnvironmentVariableW( LPCWSTR lpName, LPCWSTR lpValue );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetEnvironmentVariableW( LPCWSTR lpName, LPCWSTR lpValue )
{
    static FN_SetEnvironmentVariableW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetEnvironmentVariableW", &g_Kernel32);
    return pfn( lpName, lpValue );
}

typedef DWORD WINAPI FN_ExpandEnvironmentStringsA( LPCSTR lpSrc, LPSTR lpDst, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_ExpandEnvironmentStringsA( LPCSTR lpSrc, LPSTR lpDst, DWORD nSize )
{
    static FN_ExpandEnvironmentStringsA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ExpandEnvironmentStringsA", &g_Kernel32);
    return pfn( lpSrc, lpDst, nSize );
}

typedef DWORD WINAPI FN_ExpandEnvironmentStringsW( LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_ExpandEnvironmentStringsW( LPCWSTR lpSrc, LPWSTR lpDst, DWORD nSize )
{
    static FN_ExpandEnvironmentStringsW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ExpandEnvironmentStringsW", &g_Kernel32);
    return pfn( lpSrc, lpDst, nSize );
}

typedef DWORD WINAPI FN_GetFirmwareEnvironmentVariableA( LPCSTR lpName, LPCSTR lpGuid, PVOID pBuffer, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetFirmwareEnvironmentVariableA( LPCSTR lpName, LPCSTR lpGuid, PVOID pBuffer, DWORD nSize )
{
    static FN_GetFirmwareEnvironmentVariableA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFirmwareEnvironmentVariableA", &g_Kernel32);
    return pfn( lpName, lpGuid, pBuffer, nSize );
}

typedef DWORD WINAPI FN_GetFirmwareEnvironmentVariableW( LPCWSTR lpName, LPCWSTR lpGuid, PVOID pBuffer, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetFirmwareEnvironmentVariableW( LPCWSTR lpName, LPCWSTR lpGuid, PVOID pBuffer, DWORD nSize )
{
    static FN_GetFirmwareEnvironmentVariableW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFirmwareEnvironmentVariableW", &g_Kernel32);
    return pfn( lpName, lpGuid, pBuffer, nSize );
}

typedef BOOL WINAPI FN_SetFirmwareEnvironmentVariableA( LPCSTR lpName, LPCSTR lpGuid, PVOID pValue, DWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFirmwareEnvironmentVariableA( LPCSTR lpName, LPCSTR lpGuid, PVOID pValue, DWORD nSize )
{
    static FN_SetFirmwareEnvironmentVariableA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFirmwareEnvironmentVariableA", &g_Kernel32);
    return pfn( lpName, lpGuid, pValue, nSize );
}

typedef BOOL WINAPI FN_SetFirmwareEnvironmentVariableW( LPCWSTR lpName, LPCWSTR lpGuid, PVOID pValue, DWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFirmwareEnvironmentVariableW( LPCWSTR lpName, LPCWSTR lpGuid, PVOID pValue, DWORD nSize )
{
    static FN_SetFirmwareEnvironmentVariableW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFirmwareEnvironmentVariableW", &g_Kernel32);
    return pfn( lpName, lpGuid, pValue, nSize );
}

typedef VOID WINAPI FN_OutputDebugStringA( LPCSTR lpOutputString );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_OutputDebugStringA( LPCSTR lpOutputString )
{
    static FN_OutputDebugStringA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OutputDebugStringA", &g_Kernel32);
    pfn( lpOutputString );
}

typedef VOID WINAPI FN_OutputDebugStringW( LPCWSTR lpOutputString );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_OutputDebugStringW( LPCWSTR lpOutputString )
{
    static FN_OutputDebugStringW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OutputDebugStringW", &g_Kernel32);
    pfn( lpOutputString );
}

typedef HRSRC WINAPI FN_FindResourceA( HMODULE hModule, LPCSTR lpName, LPCSTR lpType );
__declspec(dllexport) HRSRC WINAPI kPrf2Wrap_FindResourceA( HMODULE hModule, LPCSTR lpName, LPCSTR lpType )
{
    static FN_FindResourceA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindResourceA", &g_Kernel32);
    return pfn( hModule, lpName, lpType );
}

typedef HRSRC WINAPI FN_FindResourceW( HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType );
__declspec(dllexport) HRSRC WINAPI kPrf2Wrap_FindResourceW( HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType )
{
    static FN_FindResourceW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindResourceW", &g_Kernel32);
    return pfn( hModule, lpName, lpType );
}

typedef HRSRC WINAPI FN_FindResourceExA( HMODULE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLanguage );
__declspec(dllexport) HRSRC WINAPI kPrf2Wrap_FindResourceExA( HMODULE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLanguage )
{
    static FN_FindResourceExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindResourceExA", &g_Kernel32);
    return pfn( hModule, lpType, lpName, wLanguage );
}

typedef HRSRC WINAPI FN_FindResourceExW( HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage );
__declspec(dllexport) HRSRC WINAPI kPrf2Wrap_FindResourceExW( HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage )
{
    static FN_FindResourceExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindResourceExW", &g_Kernel32);
    return pfn( hModule, lpType, lpName, wLanguage );
}

typedef BOOL WINAPI FN_EnumResourceTypesA( HMODULE hModule, ENUMRESTYPEPROCA lpEnumFunc, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumResourceTypesA( HMODULE hModule, ENUMRESTYPEPROCA lpEnumFunc, LONG_PTR lParam )
{
    static FN_EnumResourceTypesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumResourceTypesA", &g_Kernel32);
    return pfn( hModule, lpEnumFunc, lParam );
}

typedef BOOL WINAPI FN_EnumResourceTypesW( HMODULE hModule, ENUMRESTYPEPROCW lpEnumFunc, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumResourceTypesW( HMODULE hModule, ENUMRESTYPEPROCW lpEnumFunc, LONG_PTR lParam )
{
    static FN_EnumResourceTypesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumResourceTypesW", &g_Kernel32);
    return pfn( hModule, lpEnumFunc, lParam );
}

typedef BOOL WINAPI FN_EnumResourceNamesA( HMODULE hModule, LPCSTR lpType, ENUMRESNAMEPROCA lpEnumFunc, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumResourceNamesA( HMODULE hModule, LPCSTR lpType, ENUMRESNAMEPROCA lpEnumFunc, LONG_PTR lParam )
{
    static FN_EnumResourceNamesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumResourceNamesA", &g_Kernel32);
    return pfn( hModule, lpType, lpEnumFunc, lParam );
}

typedef BOOL WINAPI FN_EnumResourceNamesW( HMODULE hModule, LPCWSTR lpType, ENUMRESNAMEPROCW lpEnumFunc, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumResourceNamesW( HMODULE hModule, LPCWSTR lpType, ENUMRESNAMEPROCW lpEnumFunc, LONG_PTR lParam )
{
    static FN_EnumResourceNamesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumResourceNamesW", &g_Kernel32);
    return pfn( hModule, lpType, lpEnumFunc, lParam );
}

typedef BOOL WINAPI FN_EnumResourceLanguagesA( HMODULE hModule, LPCSTR lpType, LPCSTR lpName, ENUMRESLANGPROCA lpEnumFunc, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumResourceLanguagesA( HMODULE hModule, LPCSTR lpType, LPCSTR lpName, ENUMRESLANGPROCA lpEnumFunc, LONG_PTR lParam )
{
    static FN_EnumResourceLanguagesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumResourceLanguagesA", &g_Kernel32);
    return pfn( hModule, lpType, lpName, lpEnumFunc, lParam );
}

typedef BOOL WINAPI FN_EnumResourceLanguagesW( HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, ENUMRESLANGPROCW lpEnumFunc, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumResourceLanguagesW( HMODULE hModule, LPCWSTR lpType, LPCWSTR lpName, ENUMRESLANGPROCW lpEnumFunc, LONG_PTR lParam )
{
    static FN_EnumResourceLanguagesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumResourceLanguagesW", &g_Kernel32);
    return pfn( hModule, lpType, lpName, lpEnumFunc, lParam );
}

typedef HANDLE WINAPI FN_BeginUpdateResourceA( LPCSTR pFileName, BOOL bDeleteExistingResources );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_BeginUpdateResourceA( LPCSTR pFileName, BOOL bDeleteExistingResources )
{
    static FN_BeginUpdateResourceA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BeginUpdateResourceA", &g_Kernel32);
    return pfn( pFileName, bDeleteExistingResources );
}

typedef HANDLE WINAPI FN_BeginUpdateResourceW( LPCWSTR pFileName, BOOL bDeleteExistingResources );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_BeginUpdateResourceW( LPCWSTR pFileName, BOOL bDeleteExistingResources )
{
    static FN_BeginUpdateResourceW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BeginUpdateResourceW", &g_Kernel32);
    return pfn( pFileName, bDeleteExistingResources );
}

typedef BOOL WINAPI FN_UpdateResourceA( HANDLE hUpdate, LPCSTR lpType, LPCSTR lpName, WORD wLanguage, LPVOID lpData, DWORD cb );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_UpdateResourceA( HANDLE hUpdate, LPCSTR lpType, LPCSTR lpName, WORD wLanguage, LPVOID lpData, DWORD cb )
{
    static FN_UpdateResourceA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "UpdateResourceA", &g_Kernel32);
    return pfn( hUpdate, lpType, lpName, wLanguage, lpData, cb );
}

typedef BOOL WINAPI FN_UpdateResourceW( HANDLE hUpdate, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LPVOID lpData, DWORD cb );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_UpdateResourceW( HANDLE hUpdate, LPCWSTR lpType, LPCWSTR lpName, WORD wLanguage, LPVOID lpData, DWORD cb )
{
    static FN_UpdateResourceW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "UpdateResourceW", &g_Kernel32);
    return pfn( hUpdate, lpType, lpName, wLanguage, lpData, cb );
}

typedef BOOL WINAPI FN_EndUpdateResourceA( HANDLE hUpdate, BOOL fDiscard );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EndUpdateResourceA( HANDLE hUpdate, BOOL fDiscard )
{
    static FN_EndUpdateResourceA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EndUpdateResourceA", &g_Kernel32);
    return pfn( hUpdate, fDiscard );
}

typedef BOOL WINAPI FN_EndUpdateResourceW( HANDLE hUpdate, BOOL fDiscard );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EndUpdateResourceW( HANDLE hUpdate, BOOL fDiscard )
{
    static FN_EndUpdateResourceW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EndUpdateResourceW", &g_Kernel32);
    return pfn( hUpdate, fDiscard );
}

typedef ATOM WINAPI FN_GlobalAddAtomA( LPCSTR lpString );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_GlobalAddAtomA( LPCSTR lpString )
{
    static FN_GlobalAddAtomA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalAddAtomA", &g_Kernel32);
    return pfn( lpString );
}

typedef ATOM WINAPI FN_GlobalAddAtomW( LPCWSTR lpString );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_GlobalAddAtomW( LPCWSTR lpString )
{
    static FN_GlobalAddAtomW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalAddAtomW", &g_Kernel32);
    return pfn( lpString );
}

typedef ATOM WINAPI FN_GlobalFindAtomA( LPCSTR lpString );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_GlobalFindAtomA( LPCSTR lpString )
{
    static FN_GlobalFindAtomA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalFindAtomA", &g_Kernel32);
    return pfn( lpString );
}

typedef ATOM WINAPI FN_GlobalFindAtomW( LPCWSTR lpString );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_GlobalFindAtomW( LPCWSTR lpString )
{
    static FN_GlobalFindAtomW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalFindAtomW", &g_Kernel32);
    return pfn( lpString );
}

typedef UINT WINAPI FN_GlobalGetAtomNameA( ATOM nAtom, LPSTR lpBuffer, int nSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GlobalGetAtomNameA( ATOM nAtom, LPSTR lpBuffer, int nSize )
{
    static FN_GlobalGetAtomNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalGetAtomNameA", &g_Kernel32);
    return pfn( nAtom, lpBuffer, nSize );
}

typedef UINT WINAPI FN_GlobalGetAtomNameW( ATOM nAtom, LPWSTR lpBuffer, int nSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GlobalGetAtomNameW( ATOM nAtom, LPWSTR lpBuffer, int nSize )
{
    static FN_GlobalGetAtomNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GlobalGetAtomNameW", &g_Kernel32);
    return pfn( nAtom, lpBuffer, nSize );
}

typedef ATOM WINAPI FN_AddAtomA( LPCSTR lpString );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_AddAtomA( LPCSTR lpString )
{
    static FN_AddAtomA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAtomA", &g_Kernel32);
    return pfn( lpString );
}

typedef ATOM WINAPI FN_AddAtomW( LPCWSTR lpString );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_AddAtomW( LPCWSTR lpString )
{
    static FN_AddAtomW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAtomW", &g_Kernel32);
    return pfn( lpString );
}

typedef ATOM WINAPI FN_FindAtomA( LPCSTR lpString );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_FindAtomA( LPCSTR lpString )
{
    static FN_FindAtomA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindAtomA", &g_Kernel32);
    return pfn( lpString );
}

typedef ATOM WINAPI FN_FindAtomW( LPCWSTR lpString );
__declspec(dllexport) ATOM WINAPI kPrf2Wrap_FindAtomW( LPCWSTR lpString )
{
    static FN_FindAtomW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindAtomW", &g_Kernel32);
    return pfn( lpString );
}

typedef UINT WINAPI FN_GetAtomNameA( ATOM nAtom, LPSTR lpBuffer, int nSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetAtomNameA( ATOM nAtom, LPSTR lpBuffer, int nSize )
{
    static FN_GetAtomNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetAtomNameA", &g_Kernel32);
    return pfn( nAtom, lpBuffer, nSize );
}

typedef UINT WINAPI FN_GetAtomNameW( ATOM nAtom, LPWSTR lpBuffer, int nSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetAtomNameW( ATOM nAtom, LPWSTR lpBuffer, int nSize )
{
    static FN_GetAtomNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetAtomNameW", &g_Kernel32);
    return pfn( nAtom, lpBuffer, nSize );
}

typedef UINT WINAPI FN_GetProfileIntA( LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetProfileIntA( LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault )
{
    static FN_GetProfileIntA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProfileIntA", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, nDefault );
}

typedef UINT WINAPI FN_GetProfileIntW( LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetProfileIntW( LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault )
{
    static FN_GetProfileIntW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProfileIntW", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, nDefault );
}

typedef DWORD WINAPI FN_GetProfileStringA( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetProfileStringA( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize )
{
    static FN_GetProfileStringA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProfileStringA", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize );
}

typedef DWORD WINAPI FN_GetProfileStringW( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetProfileStringW( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize )
{
    static FN_GetProfileStringW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProfileStringW", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize );
}

typedef BOOL WINAPI FN_WriteProfileStringA( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteProfileStringA( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString )
{
    static FN_WriteProfileStringA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteProfileStringA", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, lpString );
}

typedef BOOL WINAPI FN_WriteProfileStringW( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteProfileStringW( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString )
{
    static FN_WriteProfileStringW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteProfileStringW", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, lpString );
}

typedef DWORD WINAPI FN_GetProfileSectionA( LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetProfileSectionA( LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize )
{
    static FN_GetProfileSectionA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProfileSectionA", &g_Kernel32);
    return pfn( lpAppName, lpReturnedString, nSize );
}

typedef DWORD WINAPI FN_GetProfileSectionW( LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetProfileSectionW( LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize )
{
    static FN_GetProfileSectionW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetProfileSectionW", &g_Kernel32);
    return pfn( lpAppName, lpReturnedString, nSize );
}

typedef BOOL WINAPI FN_WriteProfileSectionA( LPCSTR lpAppName, LPCSTR lpString );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteProfileSectionA( LPCSTR lpAppName, LPCSTR lpString )
{
    static FN_WriteProfileSectionA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteProfileSectionA", &g_Kernel32);
    return pfn( lpAppName, lpString );
}

typedef BOOL WINAPI FN_WriteProfileSectionW( LPCWSTR lpAppName, LPCWSTR lpString );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteProfileSectionW( LPCWSTR lpAppName, LPCWSTR lpString )
{
    static FN_WriteProfileSectionW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteProfileSectionW", &g_Kernel32);
    return pfn( lpAppName, lpString );
}

typedef UINT WINAPI FN_GetPrivateProfileIntA( LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetPrivateProfileIntA( LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName )
{
    static FN_GetPrivateProfileIntA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileIntA", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, nDefault, lpFileName );
}

typedef UINT WINAPI FN_GetPrivateProfileIntW( LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetPrivateProfileIntW( LPCWSTR lpAppName, LPCWSTR lpKeyName, INT nDefault, LPCWSTR lpFileName )
{
    static FN_GetPrivateProfileIntW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileIntW", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, nDefault, lpFileName );
}

typedef DWORD WINAPI FN_GetPrivateProfileStringA( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetPrivateProfileStringA( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName )
{
    static FN_GetPrivateProfileStringA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileStringA", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName );
}

typedef DWORD WINAPI FN_GetPrivateProfileStringW( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetPrivateProfileStringW( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpDefault, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName )
{
    static FN_GetPrivateProfileStringW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileStringW", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName );
}

typedef BOOL WINAPI FN_WritePrivateProfileStringA( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WritePrivateProfileStringA( LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName )
{
    static FN_WritePrivateProfileStringA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WritePrivateProfileStringA", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, lpString, lpFileName );
}

typedef BOOL WINAPI FN_WritePrivateProfileStringW( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WritePrivateProfileStringW( LPCWSTR lpAppName, LPCWSTR lpKeyName, LPCWSTR lpString, LPCWSTR lpFileName )
{
    static FN_WritePrivateProfileStringW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WritePrivateProfileStringW", &g_Kernel32);
    return pfn( lpAppName, lpKeyName, lpString, lpFileName );
}

typedef DWORD WINAPI FN_GetPrivateProfileSectionA( LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetPrivateProfileSectionA( LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName )
{
    static FN_GetPrivateProfileSectionA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileSectionA", &g_Kernel32);
    return pfn( lpAppName, lpReturnedString, nSize, lpFileName );
}

typedef DWORD WINAPI FN_GetPrivateProfileSectionW( LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetPrivateProfileSectionW( LPCWSTR lpAppName, LPWSTR lpReturnedString, DWORD nSize, LPCWSTR lpFileName )
{
    static FN_GetPrivateProfileSectionW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileSectionW", &g_Kernel32);
    return pfn( lpAppName, lpReturnedString, nSize, lpFileName );
}

typedef BOOL WINAPI FN_WritePrivateProfileSectionA( LPCSTR lpAppName, LPCSTR lpString, LPCSTR lpFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WritePrivateProfileSectionA( LPCSTR lpAppName, LPCSTR lpString, LPCSTR lpFileName )
{
    static FN_WritePrivateProfileSectionA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WritePrivateProfileSectionA", &g_Kernel32);
    return pfn( lpAppName, lpString, lpFileName );
}

typedef BOOL WINAPI FN_WritePrivateProfileSectionW( LPCWSTR lpAppName, LPCWSTR lpString, LPCWSTR lpFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WritePrivateProfileSectionW( LPCWSTR lpAppName, LPCWSTR lpString, LPCWSTR lpFileName )
{
    static FN_WritePrivateProfileSectionW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WritePrivateProfileSectionW", &g_Kernel32);
    return pfn( lpAppName, lpString, lpFileName );
}

typedef DWORD WINAPI FN_GetPrivateProfileSectionNamesA( LPSTR lpszReturnBuffer, DWORD nSize, LPCSTR lpFileName );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetPrivateProfileSectionNamesA( LPSTR lpszReturnBuffer, DWORD nSize, LPCSTR lpFileName )
{
    static FN_GetPrivateProfileSectionNamesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileSectionNamesA", &g_Kernel32);
    return pfn( lpszReturnBuffer, nSize, lpFileName );
}

typedef DWORD WINAPI FN_GetPrivateProfileSectionNamesW( LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetPrivateProfileSectionNamesW( LPWSTR lpszReturnBuffer, DWORD nSize, LPCWSTR lpFileName )
{
    static FN_GetPrivateProfileSectionNamesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileSectionNamesW", &g_Kernel32);
    return pfn( lpszReturnBuffer, nSize, lpFileName );
}

typedef BOOL WINAPI FN_GetPrivateProfileStructA( LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCSTR szFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetPrivateProfileStructA( LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCSTR szFile )
{
    static FN_GetPrivateProfileStructA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileStructA", &g_Kernel32);
    return pfn( lpszSection, lpszKey, lpStruct, uSizeStruct, szFile );
}

typedef BOOL WINAPI FN_GetPrivateProfileStructW( LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR szFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetPrivateProfileStructW( LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR szFile )
{
    static FN_GetPrivateProfileStructW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateProfileStructW", &g_Kernel32);
    return pfn( lpszSection, lpszKey, lpStruct, uSizeStruct, szFile );
}

typedef BOOL WINAPI FN_WritePrivateProfileStructA( LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCSTR szFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WritePrivateProfileStructA( LPCSTR lpszSection, LPCSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCSTR szFile )
{
    static FN_WritePrivateProfileStructA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WritePrivateProfileStructA", &g_Kernel32);
    return pfn( lpszSection, lpszKey, lpStruct, uSizeStruct, szFile );
}

typedef BOOL WINAPI FN_WritePrivateProfileStructW( LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR szFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WritePrivateProfileStructW( LPCWSTR lpszSection, LPCWSTR lpszKey, LPVOID lpStruct, UINT uSizeStruct, LPCWSTR szFile )
{
    static FN_WritePrivateProfileStructW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WritePrivateProfileStructW", &g_Kernel32);
    return pfn( lpszSection, lpszKey, lpStruct, uSizeStruct, szFile );
}

typedef UINT WINAPI FN_GetDriveTypeA( LPCSTR lpRootPathName );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetDriveTypeA( LPCSTR lpRootPathName )
{
    static FN_GetDriveTypeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDriveTypeA", &g_Kernel32);
    return pfn( lpRootPathName );
}

typedef UINT WINAPI FN_GetDriveTypeW( LPCWSTR lpRootPathName );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetDriveTypeW( LPCWSTR lpRootPathName )
{
    static FN_GetDriveTypeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDriveTypeW", &g_Kernel32);
    return pfn( lpRootPathName );
}

typedef UINT WINAPI FN_GetSystemDirectoryA( LPSTR lpBuffer, UINT uSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetSystemDirectoryA( LPSTR lpBuffer, UINT uSize )
{
    static FN_GetSystemDirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemDirectoryA", &g_Kernel32);
    return pfn( lpBuffer, uSize );
}

typedef UINT WINAPI FN_GetSystemDirectoryW( LPWSTR lpBuffer, UINT uSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetSystemDirectoryW( LPWSTR lpBuffer, UINT uSize )
{
    static FN_GetSystemDirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemDirectoryW", &g_Kernel32);
    return pfn( lpBuffer, uSize );
}

typedef DWORD WINAPI FN_GetTempPathA( DWORD nBufferLength, LPSTR lpBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetTempPathA( DWORD nBufferLength, LPSTR lpBuffer )
{
    static FN_GetTempPathA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTempPathA", &g_Kernel32);
    return pfn( nBufferLength, lpBuffer );
}

typedef DWORD WINAPI FN_GetTempPathW( DWORD nBufferLength, LPWSTR lpBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetTempPathW( DWORD nBufferLength, LPWSTR lpBuffer )
{
    static FN_GetTempPathW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTempPathW", &g_Kernel32);
    return pfn( nBufferLength, lpBuffer );
}

typedef UINT WINAPI FN_GetTempFileNameA( LPCSTR lpPathName, LPCSTR lpPrefixString, UINT uUnique, LPSTR lpTempFileName );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetTempFileNameA( LPCSTR lpPathName, LPCSTR lpPrefixString, UINT uUnique, LPSTR lpTempFileName )
{
    static FN_GetTempFileNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTempFileNameA", &g_Kernel32);
    return pfn( lpPathName, lpPrefixString, uUnique, lpTempFileName );
}

typedef UINT WINAPI FN_GetTempFileNameW( LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetTempFileNameW( LPCWSTR lpPathName, LPCWSTR lpPrefixString, UINT uUnique, LPWSTR lpTempFileName )
{
    static FN_GetTempFileNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTempFileNameW", &g_Kernel32);
    return pfn( lpPathName, lpPrefixString, uUnique, lpTempFileName );
}

typedef UINT WINAPI FN_GetWindowsDirectoryA( LPSTR lpBuffer, UINT uSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetWindowsDirectoryA( LPSTR lpBuffer, UINT uSize )
{
    static FN_GetWindowsDirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetWindowsDirectoryA", &g_Kernel32);
    return pfn( lpBuffer, uSize );
}

typedef UINT WINAPI FN_GetWindowsDirectoryW( LPWSTR lpBuffer, UINT uSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetWindowsDirectoryW( LPWSTR lpBuffer, UINT uSize )
{
    static FN_GetWindowsDirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetWindowsDirectoryW", &g_Kernel32);
    return pfn( lpBuffer, uSize );
}

typedef UINT WINAPI FN_GetSystemWindowsDirectoryA( LPSTR lpBuffer, UINT uSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetSystemWindowsDirectoryA( LPSTR lpBuffer, UINT uSize )
{
    static FN_GetSystemWindowsDirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemWindowsDirectoryA", &g_Kernel32);
    return pfn( lpBuffer, uSize );
}

typedef UINT WINAPI FN_GetSystemWindowsDirectoryW( LPWSTR lpBuffer, UINT uSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetSystemWindowsDirectoryW( LPWSTR lpBuffer, UINT uSize )
{
    static FN_GetSystemWindowsDirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemWindowsDirectoryW", &g_Kernel32);
    return pfn( lpBuffer, uSize );
}

typedef UINT WINAPI FN_GetSystemWow64DirectoryA( LPSTR lpBuffer, UINT uSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetSystemWow64DirectoryA( LPSTR lpBuffer, UINT uSize )
{
    static FN_GetSystemWow64DirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemWow64DirectoryA", &g_Kernel32);
    return pfn( lpBuffer, uSize );
}

typedef UINT WINAPI FN_GetSystemWow64DirectoryW( LPWSTR lpBuffer, UINT uSize );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetSystemWow64DirectoryW( LPWSTR lpBuffer, UINT uSize )
{
    static FN_GetSystemWow64DirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemWow64DirectoryW", &g_Kernel32);
    return pfn( lpBuffer, uSize );
}

typedef BOOLEAN WINAPI FN_Wow64EnableWow64FsRedirection( BOOLEAN Wow64FsEnableRedirection );
__declspec(dllexport) BOOLEAN WINAPI kPrf2Wrap_Wow64EnableWow64FsRedirection( BOOLEAN Wow64FsEnableRedirection )
{
    static FN_Wow64EnableWow64FsRedirection *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Wow64EnableWow64FsRedirection", &g_Kernel32);
    return pfn( Wow64FsEnableRedirection );
}

typedef BOOL WINAPI FN_Wow64DisableWow64FsRedirection( PVOID * OldValue );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Wow64DisableWow64FsRedirection( PVOID * OldValue )
{
    static FN_Wow64DisableWow64FsRedirection *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Wow64DisableWow64FsRedirection", &g_Kernel32);
    return pfn( OldValue );
}

typedef BOOL WINAPI FN_Wow64RevertWow64FsRedirection( PVOID OlValue );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Wow64RevertWow64FsRedirection( PVOID OlValue )
{
    static FN_Wow64RevertWow64FsRedirection *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Wow64RevertWow64FsRedirection", &g_Kernel32);
    return pfn( OlValue );
}

typedef BOOL WINAPI FN_SetCurrentDirectoryA( LPCSTR lpPathName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetCurrentDirectoryA( LPCSTR lpPathName )
{
    static FN_SetCurrentDirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCurrentDirectoryA", &g_Kernel32);
    return pfn( lpPathName );
}

typedef BOOL WINAPI FN_SetCurrentDirectoryW( LPCWSTR lpPathName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetCurrentDirectoryW( LPCWSTR lpPathName )
{
    static FN_SetCurrentDirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCurrentDirectoryW", &g_Kernel32);
    return pfn( lpPathName );
}

typedef DWORD WINAPI FN_GetCurrentDirectoryA( DWORD nBufferLength, LPSTR lpBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetCurrentDirectoryA( DWORD nBufferLength, LPSTR lpBuffer )
{
    static FN_GetCurrentDirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentDirectoryA", &g_Kernel32);
    return pfn( nBufferLength, lpBuffer );
}

typedef DWORD WINAPI FN_GetCurrentDirectoryW( DWORD nBufferLength, LPWSTR lpBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetCurrentDirectoryW( DWORD nBufferLength, LPWSTR lpBuffer )
{
    static FN_GetCurrentDirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentDirectoryW", &g_Kernel32);
    return pfn( nBufferLength, lpBuffer );
}

typedef BOOL WINAPI FN_SetDllDirectoryA( LPCSTR lpPathName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetDllDirectoryA( LPCSTR lpPathName )
{
    static FN_SetDllDirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetDllDirectoryA", &g_Kernel32);
    return pfn( lpPathName );
}

typedef BOOL WINAPI FN_SetDllDirectoryW( LPCWSTR lpPathName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetDllDirectoryW( LPCWSTR lpPathName )
{
    static FN_SetDllDirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetDllDirectoryW", &g_Kernel32);
    return pfn( lpPathName );
}

typedef DWORD WINAPI FN_GetDllDirectoryA( DWORD nBufferLength, LPSTR lpBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetDllDirectoryA( DWORD nBufferLength, LPSTR lpBuffer )
{
    static FN_GetDllDirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDllDirectoryA", &g_Kernel32);
    return pfn( nBufferLength, lpBuffer );
}

typedef DWORD WINAPI FN_GetDllDirectoryW( DWORD nBufferLength, LPWSTR lpBuffer );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetDllDirectoryW( DWORD nBufferLength, LPWSTR lpBuffer )
{
    static FN_GetDllDirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDllDirectoryW", &g_Kernel32);
    return pfn( nBufferLength, lpBuffer );
}

typedef BOOL WINAPI FN_GetDiskFreeSpaceA( LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetDiskFreeSpaceA( LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters )
{
    static FN_GetDiskFreeSpaceA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDiskFreeSpaceA", &g_Kernel32);
    return pfn( lpRootPathName, lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters );
}

typedef BOOL WINAPI FN_GetDiskFreeSpaceW( LPCWSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetDiskFreeSpaceW( LPCWSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters )
{
    static FN_GetDiskFreeSpaceW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDiskFreeSpaceW", &g_Kernel32);
    return pfn( lpRootPathName, lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters );
}

typedef BOOL WINAPI FN_GetDiskFreeSpaceExA( LPCSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetDiskFreeSpaceExA( LPCSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes )
{
    static FN_GetDiskFreeSpaceExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDiskFreeSpaceExA", &g_Kernel32);
    return pfn( lpDirectoryName, lpFreeBytesAvailableToCaller, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes );
}

typedef BOOL WINAPI FN_GetDiskFreeSpaceExW( LPCWSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetDiskFreeSpaceExW( LPCWSTR lpDirectoryName, PULARGE_INTEGER lpFreeBytesAvailableToCaller, PULARGE_INTEGER lpTotalNumberOfBytes, PULARGE_INTEGER lpTotalNumberOfFreeBytes )
{
    static FN_GetDiskFreeSpaceExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDiskFreeSpaceExW", &g_Kernel32);
    return pfn( lpDirectoryName, lpFreeBytesAvailableToCaller, lpTotalNumberOfBytes, lpTotalNumberOfFreeBytes );
}

typedef BOOL WINAPI FN_CreateDirectoryA( LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateDirectoryA( LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateDirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateDirectoryA", &g_Kernel32);
    return pfn( lpPathName, lpSecurityAttributes );
}

typedef BOOL WINAPI FN_CreateDirectoryW( LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateDirectoryW( LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateDirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateDirectoryW", &g_Kernel32);
    return pfn( lpPathName, lpSecurityAttributes );
}

typedef BOOL WINAPI FN_CreateDirectoryExA( LPCSTR lpTemplateDirectory, LPCSTR lpNewDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateDirectoryExA( LPCSTR lpTemplateDirectory, LPCSTR lpNewDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateDirectoryExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateDirectoryExA", &g_Kernel32);
    return pfn( lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes );
}

typedef BOOL WINAPI FN_CreateDirectoryExW( LPCWSTR lpTemplateDirectory, LPCWSTR lpNewDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateDirectoryExW( LPCWSTR lpTemplateDirectory, LPCWSTR lpNewDirectory, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateDirectoryExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateDirectoryExW", &g_Kernel32);
    return pfn( lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes );
}

typedef BOOL WINAPI FN_RemoveDirectoryA( LPCSTR lpPathName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_RemoveDirectoryA( LPCSTR lpPathName )
{
    static FN_RemoveDirectoryA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RemoveDirectoryA", &g_Kernel32);
    return pfn( lpPathName );
}

typedef BOOL WINAPI FN_RemoveDirectoryW( LPCWSTR lpPathName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_RemoveDirectoryW( LPCWSTR lpPathName )
{
    static FN_RemoveDirectoryW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RemoveDirectoryW", &g_Kernel32);
    return pfn( lpPathName );
}

typedef DWORD WINAPI FN_GetFullPathNameA( LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR * lpFilePart );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetFullPathNameA( LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR * lpFilePart )
{
    static FN_GetFullPathNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFullPathNameA", &g_Kernel32);
    return pfn( lpFileName, nBufferLength, lpBuffer, lpFilePart );
}

typedef DWORD WINAPI FN_GetFullPathNameW( LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR * lpFilePart );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetFullPathNameW( LPCWSTR lpFileName, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR * lpFilePart )
{
    static FN_GetFullPathNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFullPathNameW", &g_Kernel32);
    return pfn( lpFileName, nBufferLength, lpBuffer, lpFilePart );
}

typedef BOOL WINAPI FN_DefineDosDeviceA( DWORD dwFlags, LPCSTR lpDeviceName, LPCSTR lpTargetPath );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DefineDosDeviceA( DWORD dwFlags, LPCSTR lpDeviceName, LPCSTR lpTargetPath )
{
    static FN_DefineDosDeviceA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DefineDosDeviceA", &g_Kernel32);
    return pfn( dwFlags, lpDeviceName, lpTargetPath );
}

typedef BOOL WINAPI FN_DefineDosDeviceW( DWORD dwFlags, LPCWSTR lpDeviceName, LPCWSTR lpTargetPath );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DefineDosDeviceW( DWORD dwFlags, LPCWSTR lpDeviceName, LPCWSTR lpTargetPath )
{
    static FN_DefineDosDeviceW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DefineDosDeviceW", &g_Kernel32);
    return pfn( dwFlags, lpDeviceName, lpTargetPath );
}

typedef DWORD WINAPI FN_QueryDosDeviceA( LPCSTR lpDeviceName, LPSTR lpTargetPath, DWORD ucchMax );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_QueryDosDeviceA( LPCSTR lpDeviceName, LPSTR lpTargetPath, DWORD ucchMax )
{
    static FN_QueryDosDeviceA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueryDosDeviceA", &g_Kernel32);
    return pfn( lpDeviceName, lpTargetPath, ucchMax );
}

typedef DWORD WINAPI FN_QueryDosDeviceW( LPCWSTR lpDeviceName, LPWSTR lpTargetPath, DWORD ucchMax );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_QueryDosDeviceW( LPCWSTR lpDeviceName, LPWSTR lpTargetPath, DWORD ucchMax )
{
    static FN_QueryDosDeviceW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueryDosDeviceW", &g_Kernel32);
    return pfn( lpDeviceName, lpTargetPath, ucchMax );
}

typedef HANDLE WINAPI FN_CreateFileA( LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateFileA( LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile )
{
    static FN_CreateFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateFileA", &g_Kernel32);
    return pfn( lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
}

typedef HANDLE WINAPI FN_CreateFileW( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateFileW( LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile )
{
    static FN_CreateFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateFileW", &g_Kernel32);
    return pfn( lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile );
}

typedef HANDLE WINAPI FN_ReOpenFile( HANDLE hOriginalFile, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwFlagsAndAttributes );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_ReOpenFile( HANDLE hOriginalFile, DWORD dwDesiredAccess, DWORD dwShareMode, DWORD dwFlagsAndAttributes )
{
    static FN_ReOpenFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReOpenFile", &g_Kernel32);
    return pfn( hOriginalFile, dwDesiredAccess, dwShareMode, dwFlagsAndAttributes );
}

typedef BOOL WINAPI FN_SetFileAttributesA( LPCSTR lpFileName, DWORD dwFileAttributes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFileAttributesA( LPCSTR lpFileName, DWORD dwFileAttributes )
{
    static FN_SetFileAttributesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileAttributesA", &g_Kernel32);
    return pfn( lpFileName, dwFileAttributes );
}

typedef BOOL WINAPI FN_SetFileAttributesW( LPCWSTR lpFileName, DWORD dwFileAttributes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFileAttributesW( LPCWSTR lpFileName, DWORD dwFileAttributes )
{
    static FN_SetFileAttributesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileAttributesW", &g_Kernel32);
    return pfn( lpFileName, dwFileAttributes );
}

typedef DWORD WINAPI FN_GetFileAttributesA( LPCSTR lpFileName );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetFileAttributesA( LPCSTR lpFileName )
{
    static FN_GetFileAttributesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileAttributesA", &g_Kernel32);
    return pfn( lpFileName );
}

typedef DWORD WINAPI FN_GetFileAttributesW( LPCWSTR lpFileName );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetFileAttributesW( LPCWSTR lpFileName )
{
    static FN_GetFileAttributesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileAttributesW", &g_Kernel32);
    return pfn( lpFileName );
}

typedef BOOL WINAPI FN_GetFileAttributesExA( LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetFileAttributesExA( LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation )
{
    static FN_GetFileAttributesExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileAttributesExA", &g_Kernel32);
    return pfn( lpFileName, fInfoLevelId, lpFileInformation );
}

typedef BOOL WINAPI FN_GetFileAttributesExW( LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetFileAttributesExW( LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation )
{
    static FN_GetFileAttributesExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileAttributesExW", &g_Kernel32);
    return pfn( lpFileName, fInfoLevelId, lpFileInformation );
}

typedef DWORD WINAPI FN_GetCompressedFileSizeA( LPCSTR lpFileName, LPDWORD lpFileSizeHigh );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetCompressedFileSizeA( LPCSTR lpFileName, LPDWORD lpFileSizeHigh )
{
    static FN_GetCompressedFileSizeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCompressedFileSizeA", &g_Kernel32);
    return pfn( lpFileName, lpFileSizeHigh );
}

typedef DWORD WINAPI FN_GetCompressedFileSizeW( LPCWSTR lpFileName, LPDWORD lpFileSizeHigh );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetCompressedFileSizeW( LPCWSTR lpFileName, LPDWORD lpFileSizeHigh )
{
    static FN_GetCompressedFileSizeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCompressedFileSizeW", &g_Kernel32);
    return pfn( lpFileName, lpFileSizeHigh );
}

typedef BOOL WINAPI FN_DeleteFileA( LPCSTR lpFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeleteFileA( LPCSTR lpFileName )
{
    static FN_DeleteFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteFileA", &g_Kernel32);
    return pfn( lpFileName );
}

typedef BOOL WINAPI FN_DeleteFileW( LPCWSTR lpFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeleteFileW( LPCWSTR lpFileName )
{
    static FN_DeleteFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteFileW", &g_Kernel32);
    return pfn( lpFileName );
}

typedef BOOL WINAPI FN_CheckNameLegalDOS8Dot3A( LPCSTR lpName, LPSTR lpOemName, DWORD OemNameSize, PBOOL pbNameContainsSpaces , PBOOL pbNameLegal );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CheckNameLegalDOS8Dot3A( LPCSTR lpName, LPSTR lpOemName, DWORD OemNameSize, PBOOL pbNameContainsSpaces , PBOOL pbNameLegal )
{
    static FN_CheckNameLegalDOS8Dot3A *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CheckNameLegalDOS8Dot3A", &g_Kernel32);
    return pfn( lpName, lpOemName, OemNameSize, pbNameContainsSpaces , pbNameLegal );
}

typedef BOOL WINAPI FN_CheckNameLegalDOS8Dot3W( LPCWSTR lpName, LPSTR lpOemName, DWORD OemNameSize, PBOOL pbNameContainsSpaces , PBOOL pbNameLegal );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CheckNameLegalDOS8Dot3W( LPCWSTR lpName, LPSTR lpOemName, DWORD OemNameSize, PBOOL pbNameContainsSpaces , PBOOL pbNameLegal )
{
    static FN_CheckNameLegalDOS8Dot3W *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CheckNameLegalDOS8Dot3W", &g_Kernel32);
    return pfn( lpName, lpOemName, OemNameSize, pbNameContainsSpaces , pbNameLegal );
}

typedef HANDLE WINAPI FN_FindFirstFileExA( LPCSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstFileExA( LPCSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags )
{
    static FN_FindFirstFileExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstFileExA", &g_Kernel32);
    return pfn( lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags );
}

typedef HANDLE WINAPI FN_FindFirstFileExW( LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstFileExW( LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags )
{
    static FN_FindFirstFileExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstFileExW", &g_Kernel32);
    return pfn( lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags );
}

typedef HANDLE WINAPI FN_FindFirstFileA( LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstFileA( LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData )
{
    static FN_FindFirstFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstFileA", &g_Kernel32);
    return pfn( lpFileName, lpFindFileData );
}

typedef HANDLE WINAPI FN_FindFirstFileW( LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstFileW( LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData )
{
    static FN_FindFirstFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstFileW", &g_Kernel32);
    return pfn( lpFileName, lpFindFileData );
}

typedef BOOL WINAPI FN_FindNextFileA( HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindNextFileA( HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData )
{
    static FN_FindNextFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindNextFileA", &g_Kernel32);
    return pfn( hFindFile, lpFindFileData );
}

typedef BOOL WINAPI FN_FindNextFileW( HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindNextFileW( HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData )
{
    static FN_FindNextFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindNextFileW", &g_Kernel32);
    return pfn( hFindFile, lpFindFileData );
}

typedef DWORD WINAPI FN_SearchPathA( LPCSTR lpPath, LPCSTR lpFileName, LPCSTR lpExtension, DWORD nBufferLength, LPSTR lpBuffer, LPSTR * lpFilePart );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SearchPathA( LPCSTR lpPath, LPCSTR lpFileName, LPCSTR lpExtension, DWORD nBufferLength, LPSTR lpBuffer, LPSTR * lpFilePart )
{
    static FN_SearchPathA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SearchPathA", &g_Kernel32);
    return pfn( lpPath, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart );
}

typedef DWORD WINAPI FN_SearchPathW( LPCWSTR lpPath, LPCWSTR lpFileName, LPCWSTR lpExtension, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR * lpFilePart );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SearchPathW( LPCWSTR lpPath, LPCWSTR lpFileName, LPCWSTR lpExtension, DWORD nBufferLength, LPWSTR lpBuffer, LPWSTR * lpFilePart )
{
    static FN_SearchPathW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SearchPathW", &g_Kernel32);
    return pfn( lpPath, lpFileName, lpExtension, nBufferLength, lpBuffer, lpFilePart );
}

typedef BOOL WINAPI FN_CopyFileA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CopyFileA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists )
{
    static FN_CopyFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CopyFileA", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName, bFailIfExists );
}

typedef BOOL WINAPI FN_CopyFileW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CopyFileW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, BOOL bFailIfExists )
{
    static FN_CopyFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CopyFileW", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName, bFailIfExists );
}

typedef BOOL WINAPI FN_CopyFileExA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CopyFileExA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags )
{
    static FN_CopyFileExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CopyFileExA", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags );
}

typedef BOOL WINAPI FN_CopyFileExW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CopyFileExW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, LPBOOL pbCancel, DWORD dwCopyFlags )
{
    static FN_CopyFileExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CopyFileExW", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, pbCancel, dwCopyFlags );
}

typedef BOOL WINAPI FN_MoveFileA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MoveFileA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName )
{
    static FN_MoveFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MoveFileA", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName );
}

typedef BOOL WINAPI FN_MoveFileW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MoveFileW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName )
{
    static FN_MoveFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MoveFileW", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName );
}

typedef BOOL WINAPI FN_MoveFileExA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MoveFileExA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD dwFlags )
{
    static FN_MoveFileExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MoveFileExA", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName, dwFlags );
}

typedef BOOL WINAPI FN_MoveFileExW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MoveFileExW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, DWORD dwFlags )
{
    static FN_MoveFileExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MoveFileExW", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName, dwFlags );
}

typedef BOOL WINAPI FN_MoveFileWithProgressA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MoveFileWithProgressA( LPCSTR lpExistingFileName, LPCSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags )
{
    static FN_MoveFileWithProgressA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MoveFileWithProgressA", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, dwFlags );
}

typedef BOOL WINAPI FN_MoveFileWithProgressW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MoveFileWithProgressW( LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName, LPPROGRESS_ROUTINE lpProgressRoutine, LPVOID lpData, DWORD dwFlags )
{
    static FN_MoveFileWithProgressW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MoveFileWithProgressW", &g_Kernel32);
    return pfn( lpExistingFileName, lpNewFileName, lpProgressRoutine, lpData, dwFlags );
}

typedef BOOL WINAPI FN_ReplaceFileA( LPCSTR lpReplacedFileName, LPCSTR lpReplacementFileName, LPCSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReplaceFileA( LPCSTR lpReplacedFileName, LPCSTR lpReplacementFileName, LPCSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved )
{
    static FN_ReplaceFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReplaceFileA", &g_Kernel32);
    return pfn( lpReplacedFileName, lpReplacementFileName, lpBackupFileName, dwReplaceFlags, lpExclude, lpReserved );
}

typedef BOOL WINAPI FN_ReplaceFileW( LPCWSTR lpReplacedFileName, LPCWSTR lpReplacementFileName, LPCWSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReplaceFileW( LPCWSTR lpReplacedFileName, LPCWSTR lpReplacementFileName, LPCWSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved )
{
    static FN_ReplaceFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReplaceFileW", &g_Kernel32);
    return pfn( lpReplacedFileName, lpReplacementFileName, lpBackupFileName, dwReplaceFlags, lpExclude, lpReserved );
}

typedef BOOL WINAPI FN_CreateHardLinkA( LPCSTR lpFileName, LPCSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateHardLinkA( LPCSTR lpFileName, LPCSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateHardLinkA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateHardLinkA", &g_Kernel32);
    return pfn( lpFileName, lpExistingFileName, lpSecurityAttributes );
}

typedef BOOL WINAPI FN_CreateHardLinkW( LPCWSTR lpFileName, LPCWSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateHardLinkW( LPCWSTR lpFileName, LPCWSTR lpExistingFileName, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateHardLinkW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateHardLinkW", &g_Kernel32);
    return pfn( lpFileName, lpExistingFileName, lpSecurityAttributes );
}

typedef HANDLE WINAPI FN_FindFirstStreamW( LPCWSTR lpFileName, STREAM_INFO_LEVELS InfoLevel, LPVOID lpFindStreamData, DWORD dwFlags );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstStreamW( LPCWSTR lpFileName, STREAM_INFO_LEVELS InfoLevel, LPVOID lpFindStreamData, DWORD dwFlags )
{
    static FN_FindFirstStreamW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstStreamW", &g_Kernel32);
    return pfn( lpFileName, InfoLevel, lpFindStreamData, dwFlags );
}

typedef BOOL APIENTRY FN_FindNextStreamW( HANDLE hFindStream, LPVOID lpFindStreamData );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_FindNextStreamW( HANDLE hFindStream, LPVOID lpFindStreamData )
{
    static FN_FindNextStreamW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindNextStreamW", &g_Kernel32);
    return pfn( hFindStream, lpFindStreamData );
}

typedef HANDLE WINAPI FN_CreateNamedPipeA( LPCSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateNamedPipeA( LPCSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateNamedPipeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateNamedPipeA", &g_Kernel32);
    return pfn( lpName, dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes );
}

typedef HANDLE WINAPI FN_CreateNamedPipeW( LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES lpSecurityAttributes );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateNamedPipeW( LPCWSTR lpName, DWORD dwOpenMode, DWORD dwPipeMode, DWORD nMaxInstances, DWORD nOutBufferSize, DWORD nInBufferSize, DWORD nDefaultTimeOut, LPSECURITY_ATTRIBUTES lpSecurityAttributes )
{
    static FN_CreateNamedPipeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateNamedPipeW", &g_Kernel32);
    return pfn( lpName, dwOpenMode, dwPipeMode, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, lpSecurityAttributes );
}

typedef BOOL WINAPI FN_GetNamedPipeHandleStateA( HANDLE hNamedPipe, LPDWORD lpState, LPDWORD lpCurInstances, LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout, LPSTR lpUserName, DWORD nMaxUserNameSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNamedPipeHandleStateA( HANDLE hNamedPipe, LPDWORD lpState, LPDWORD lpCurInstances, LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout, LPSTR lpUserName, DWORD nMaxUserNameSize )
{
    static FN_GetNamedPipeHandleStateA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNamedPipeHandleStateA", &g_Kernel32);
    return pfn( hNamedPipe, lpState, lpCurInstances, lpMaxCollectionCount, lpCollectDataTimeout, lpUserName, nMaxUserNameSize );
}

typedef BOOL WINAPI FN_GetNamedPipeHandleStateW( HANDLE hNamedPipe, LPDWORD lpState, LPDWORD lpCurInstances, LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout, LPWSTR lpUserName, DWORD nMaxUserNameSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNamedPipeHandleStateW( HANDLE hNamedPipe, LPDWORD lpState, LPDWORD lpCurInstances, LPDWORD lpMaxCollectionCount, LPDWORD lpCollectDataTimeout, LPWSTR lpUserName, DWORD nMaxUserNameSize )
{
    static FN_GetNamedPipeHandleStateW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNamedPipeHandleStateW", &g_Kernel32);
    return pfn( hNamedPipe, lpState, lpCurInstances, lpMaxCollectionCount, lpCollectDataTimeout, lpUserName, nMaxUserNameSize );
}

typedef BOOL WINAPI FN_CallNamedPipeA( LPCSTR lpNamedPipeName, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesRead, DWORD nTimeOut );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CallNamedPipeA( LPCSTR lpNamedPipeName, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesRead, DWORD nTimeOut )
{
    static FN_CallNamedPipeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CallNamedPipeA", &g_Kernel32);
    return pfn( lpNamedPipeName, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesRead, nTimeOut );
}

typedef BOOL WINAPI FN_CallNamedPipeW( LPCWSTR lpNamedPipeName, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesRead, DWORD nTimeOut );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CallNamedPipeW( LPCWSTR lpNamedPipeName, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesRead, DWORD nTimeOut )
{
    static FN_CallNamedPipeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CallNamedPipeW", &g_Kernel32);
    return pfn( lpNamedPipeName, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesRead, nTimeOut );
}

typedef BOOL WINAPI FN_WaitNamedPipeA( LPCSTR lpNamedPipeName, DWORD nTimeOut );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WaitNamedPipeA( LPCSTR lpNamedPipeName, DWORD nTimeOut )
{
    static FN_WaitNamedPipeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WaitNamedPipeA", &g_Kernel32);
    return pfn( lpNamedPipeName, nTimeOut );
}

typedef BOOL WINAPI FN_WaitNamedPipeW( LPCWSTR lpNamedPipeName, DWORD nTimeOut );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WaitNamedPipeW( LPCWSTR lpNamedPipeName, DWORD nTimeOut )
{
    static FN_WaitNamedPipeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WaitNamedPipeW", &g_Kernel32);
    return pfn( lpNamedPipeName, nTimeOut );
}

typedef BOOL WINAPI FN_SetVolumeLabelA( LPCSTR lpRootPathName, LPCSTR lpVolumeName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetVolumeLabelA( LPCSTR lpRootPathName, LPCSTR lpVolumeName )
{
    static FN_SetVolumeLabelA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetVolumeLabelA", &g_Kernel32);
    return pfn( lpRootPathName, lpVolumeName );
}

typedef BOOL WINAPI FN_SetVolumeLabelW( LPCWSTR lpRootPathName, LPCWSTR lpVolumeName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetVolumeLabelW( LPCWSTR lpRootPathName, LPCWSTR lpVolumeName )
{
    static FN_SetVolumeLabelW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetVolumeLabelW", &g_Kernel32);
    return pfn( lpRootPathName, lpVolumeName );
}

typedef VOID WINAPI FN_SetFileApisToOEM( VOID );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_SetFileApisToOEM( VOID )
{
    static FN_SetFileApisToOEM *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileApisToOEM", &g_Kernel32);
    pfn ();
}

typedef VOID WINAPI FN_SetFileApisToANSI( VOID );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_SetFileApisToANSI( VOID )
{
    static FN_SetFileApisToANSI *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileApisToANSI", &g_Kernel32);
    pfn ();
}

typedef BOOL WINAPI FN_AreFileApisANSI( VOID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AreFileApisANSI( VOID )
{
    static FN_AreFileApisANSI *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AreFileApisANSI", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_GetVolumeInformationA( LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVolumeInformationA( LPCSTR lpRootPathName, LPSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize )
{
    static FN_GetVolumeInformationA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVolumeInformationA", &g_Kernel32);
    return pfn( lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber, lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize );
}

typedef BOOL WINAPI FN_GetVolumeInformationW( LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVolumeInformationW( LPCWSTR lpRootPathName, LPWSTR lpVolumeNameBuffer, DWORD nVolumeNameSize, LPDWORD lpVolumeSerialNumber, LPDWORD lpMaximumComponentLength, LPDWORD lpFileSystemFlags, LPWSTR lpFileSystemNameBuffer, DWORD nFileSystemNameSize )
{
    static FN_GetVolumeInformationW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVolumeInformationW", &g_Kernel32);
    return pfn( lpRootPathName, lpVolumeNameBuffer, nVolumeNameSize, lpVolumeSerialNumber, lpMaximumComponentLength, lpFileSystemFlags, lpFileSystemNameBuffer, nFileSystemNameSize );
}

typedef BOOL WINAPI FN_CancelIo( HANDLE hFile );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CancelIo( HANDLE hFile )
{
    static FN_CancelIo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CancelIo", &g_Kernel32);
    return pfn( hFile );
}

typedef BOOL WINAPI FN_ClearEventLogA( HANDLE hEventLog, LPCSTR lpBackupFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ClearEventLogA( HANDLE hEventLog, LPCSTR lpBackupFileName )
{
    static FN_ClearEventLogA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ClearEventLogA", &g_Kernel32);
    return pfn( hEventLog, lpBackupFileName );
}

typedef BOOL WINAPI FN_ClearEventLogW( HANDLE hEventLog, LPCWSTR lpBackupFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ClearEventLogW( HANDLE hEventLog, LPCWSTR lpBackupFileName )
{
    static FN_ClearEventLogW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ClearEventLogW", &g_Kernel32);
    return pfn( hEventLog, lpBackupFileName );
}

typedef BOOL WINAPI FN_BackupEventLogA( HANDLE hEventLog, LPCSTR lpBackupFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BackupEventLogA( HANDLE hEventLog, LPCSTR lpBackupFileName )
{
    static FN_BackupEventLogA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BackupEventLogA", &g_Kernel32);
    return pfn( hEventLog, lpBackupFileName );
}

typedef BOOL WINAPI FN_BackupEventLogW( HANDLE hEventLog, LPCWSTR lpBackupFileName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BackupEventLogW( HANDLE hEventLog, LPCWSTR lpBackupFileName )
{
    static FN_BackupEventLogW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BackupEventLogW", &g_Kernel32);
    return pfn( hEventLog, lpBackupFileName );
}

typedef BOOL WINAPI FN_CloseEventLog( HANDLE hEventLog );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CloseEventLog( HANDLE hEventLog )
{
    static FN_CloseEventLog *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CloseEventLog", &g_Kernel32);
    return pfn( hEventLog );
}

typedef BOOL WINAPI FN_DeregisterEventSource( HANDLE hEventLog );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeregisterEventSource( HANDLE hEventLog )
{
    static FN_DeregisterEventSource *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeregisterEventSource", &g_Kernel32);
    return pfn( hEventLog );
}

typedef BOOL WINAPI FN_NotifyChangeEventLog( HANDLE hEventLog, HANDLE hEvent );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_NotifyChangeEventLog( HANDLE hEventLog, HANDLE hEvent )
{
    static FN_NotifyChangeEventLog *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "NotifyChangeEventLog", &g_Kernel32);
    return pfn( hEventLog, hEvent );
}

typedef BOOL WINAPI FN_GetNumberOfEventLogRecords( HANDLE hEventLog, PDWORD NumberOfRecords );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNumberOfEventLogRecords( HANDLE hEventLog, PDWORD NumberOfRecords )
{
    static FN_GetNumberOfEventLogRecords *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNumberOfEventLogRecords", &g_Kernel32);
    return pfn( hEventLog, NumberOfRecords );
}

typedef BOOL WINAPI FN_GetOldestEventLogRecord( HANDLE hEventLog, PDWORD OldestRecord );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetOldestEventLogRecord( HANDLE hEventLog, PDWORD OldestRecord )
{
    static FN_GetOldestEventLogRecord *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetOldestEventLogRecord", &g_Kernel32);
    return pfn( hEventLog, OldestRecord );
}

typedef HANDLE WINAPI FN_OpenEventLogA( LPCSTR lpUNCServerName, LPCSTR lpSourceName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenEventLogA( LPCSTR lpUNCServerName, LPCSTR lpSourceName )
{
    static FN_OpenEventLogA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenEventLogA", &g_Kernel32);
    return pfn( lpUNCServerName, lpSourceName );
}

typedef HANDLE WINAPI FN_OpenEventLogW( LPCWSTR lpUNCServerName, LPCWSTR lpSourceName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenEventLogW( LPCWSTR lpUNCServerName, LPCWSTR lpSourceName )
{
    static FN_OpenEventLogW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenEventLogW", &g_Kernel32);
    return pfn( lpUNCServerName, lpSourceName );
}

typedef HANDLE WINAPI FN_RegisterEventSourceA( LPCSTR lpUNCServerName, LPCSTR lpSourceName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_RegisterEventSourceA( LPCSTR lpUNCServerName, LPCSTR lpSourceName )
{
    static FN_RegisterEventSourceA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RegisterEventSourceA", &g_Kernel32);
    return pfn( lpUNCServerName, lpSourceName );
}

typedef HANDLE WINAPI FN_RegisterEventSourceW( LPCWSTR lpUNCServerName, LPCWSTR lpSourceName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_RegisterEventSourceW( LPCWSTR lpUNCServerName, LPCWSTR lpSourceName )
{
    static FN_RegisterEventSourceW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RegisterEventSourceW", &g_Kernel32);
    return pfn( lpUNCServerName, lpSourceName );
}

typedef HANDLE WINAPI FN_OpenBackupEventLogA( LPCSTR lpUNCServerName, LPCSTR lpFileName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenBackupEventLogA( LPCSTR lpUNCServerName, LPCSTR lpFileName )
{
    static FN_OpenBackupEventLogA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenBackupEventLogA", &g_Kernel32);
    return pfn( lpUNCServerName, lpFileName );
}

typedef HANDLE WINAPI FN_OpenBackupEventLogW( LPCWSTR lpUNCServerName, LPCWSTR lpFileName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenBackupEventLogW( LPCWSTR lpUNCServerName, LPCWSTR lpFileName )
{
    static FN_OpenBackupEventLogW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenBackupEventLogW", &g_Kernel32);
    return pfn( lpUNCServerName, lpFileName );
}

typedef BOOL WINAPI FN_ReadEventLogA( HANDLE hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD * pnBytesRead, DWORD * pnMinNumberOfBytesNeeded );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadEventLogA( HANDLE hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD * pnBytesRead, DWORD * pnMinNumberOfBytesNeeded )
{
    static FN_ReadEventLogA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadEventLogA", &g_Kernel32);
    return pfn( hEventLog, dwReadFlags, dwRecordOffset, lpBuffer, nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded );
}

typedef BOOL WINAPI FN_ReadEventLogW( HANDLE hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD * pnBytesRead, DWORD * pnMinNumberOfBytesNeeded );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadEventLogW( HANDLE hEventLog, DWORD dwReadFlags, DWORD dwRecordOffset, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, DWORD * pnBytesRead, DWORD * pnMinNumberOfBytesNeeded )
{
    static FN_ReadEventLogW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadEventLogW", &g_Kernel32);
    return pfn( hEventLog, dwReadFlags, dwRecordOffset, lpBuffer, nNumberOfBytesToRead, pnBytesRead, pnMinNumberOfBytesNeeded );
}

typedef BOOL WINAPI FN_ReportEventA( HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCSTR * lpStrings, LPVOID lpRawData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReportEventA( HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCSTR * lpStrings, LPVOID lpRawData )
{
    static FN_ReportEventA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReportEventA", &g_Kernel32);
    return pfn( hEventLog, wType, wCategory, dwEventID, lpUserSid, wNumStrings, dwDataSize, lpStrings, lpRawData );
}

typedef BOOL WINAPI FN_ReportEventW( HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCWSTR * lpStrings, LPVOID lpRawData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReportEventW( HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCWSTR * lpStrings, LPVOID lpRawData )
{
    static FN_ReportEventW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReportEventW", &g_Kernel32);
    return pfn( hEventLog, wType, wCategory, dwEventID, lpUserSid, wNumStrings, dwDataSize, lpStrings, lpRawData );
}

typedef BOOL WINAPI FN_GetEventLogInformation( HANDLE hEventLog, DWORD dwInfoLevel, LPVOID lpBuffer, DWORD cbBufSize, LPDWORD pcbBytesNeeded );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetEventLogInformation( HANDLE hEventLog, DWORD dwInfoLevel, LPVOID lpBuffer, DWORD cbBufSize, LPDWORD pcbBytesNeeded )
{
    static FN_GetEventLogInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetEventLogInformation", &g_Kernel32);
    return pfn( hEventLog, dwInfoLevel, lpBuffer, cbBufSize, pcbBytesNeeded );
}

typedef BOOL WINAPI FN_DuplicateToken( HANDLE ExistingTokenHandle, SECURITY_IMPERSONATION_LEVEL ImpersonationLevel, PHANDLE DuplicateTokenHandle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DuplicateToken( HANDLE ExistingTokenHandle, SECURITY_IMPERSONATION_LEVEL ImpersonationLevel, PHANDLE DuplicateTokenHandle )
{
    static FN_DuplicateToken *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DuplicateToken", &g_Kernel32);
    return pfn( ExistingTokenHandle, ImpersonationLevel, DuplicateTokenHandle );
}

typedef BOOL WINAPI FN_GetKernelObjectSecurity( HANDLE Handle, SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetKernelObjectSecurity( HANDLE Handle, SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded )
{
    static FN_GetKernelObjectSecurity *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetKernelObjectSecurity", &g_Kernel32);
    return pfn( Handle, RequestedInformation, pSecurityDescriptor, nLength, lpnLengthNeeded );
}

typedef BOOL WINAPI FN_ImpersonateNamedPipeClient( HANDLE hNamedPipe );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ImpersonateNamedPipeClient( HANDLE hNamedPipe )
{
    static FN_ImpersonateNamedPipeClient *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ImpersonateNamedPipeClient", &g_Kernel32);
    return pfn( hNamedPipe );
}

typedef BOOL WINAPI FN_ImpersonateSelf( SECURITY_IMPERSONATION_LEVEL ImpersonationLevel );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ImpersonateSelf( SECURITY_IMPERSONATION_LEVEL ImpersonationLevel )
{
    static FN_ImpersonateSelf *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ImpersonateSelf", &g_Kernel32);
    return pfn( ImpersonationLevel );
}

typedef BOOL WINAPI FN_RevertToSelf( VOID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_RevertToSelf( VOID )
{
    static FN_RevertToSelf *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RevertToSelf", &g_Kernel32);
    return pfn ();
}

typedef BOOL APIENTRY FN_SetThreadToken( PHANDLE Thread, HANDLE Token );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_SetThreadToken( PHANDLE Thread, HANDLE Token )
{
    static FN_SetThreadToken *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetThreadToken", &g_Kernel32);
    return pfn( Thread, Token );
}

typedef BOOL WINAPI FN_AccessCheck( PSECURITY_DESCRIPTOR pSecurityDescriptor, HANDLE ClientToken, DWORD DesiredAccess, PGENERIC_MAPPING GenericMapping, PPRIVILEGE_SET PrivilegeSet, LPDWORD PrivilegeSetLength, LPDWORD GrantedAccess, LPBOOL AccessStatus );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheck( PSECURITY_DESCRIPTOR pSecurityDescriptor, HANDLE ClientToken, DWORD DesiredAccess, PGENERIC_MAPPING GenericMapping, PPRIVILEGE_SET PrivilegeSet, LPDWORD PrivilegeSetLength, LPDWORD GrantedAccess, LPBOOL AccessStatus )
{
    static FN_AccessCheck *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheck", &g_Kernel32);
    return pfn( pSecurityDescriptor, ClientToken, DesiredAccess, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus );
}

typedef BOOL WINAPI FN_AccessCheckByType( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID PrincipalSelfSid, HANDLE ClientToken, DWORD DesiredAccess, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, PPRIVILEGE_SET PrivilegeSet, LPDWORD PrivilegeSetLength, LPDWORD GrantedAccess, LPBOOL AccessStatus );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckByType( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID PrincipalSelfSid, HANDLE ClientToken, DWORD DesiredAccess, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, PPRIVILEGE_SET PrivilegeSet, LPDWORD PrivilegeSetLength, LPDWORD GrantedAccess, LPBOOL AccessStatus )
{
    static FN_AccessCheckByType *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckByType", &g_Kernel32);
    return pfn( pSecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccess, AccessStatus );
}

typedef BOOL WINAPI FN_AccessCheckByTypeResultList( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID PrincipalSelfSid, HANDLE ClientToken, DWORD DesiredAccess, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, PPRIVILEGE_SET PrivilegeSet, LPDWORD PrivilegeSetLength, LPDWORD GrantedAccessList, LPDWORD AccessStatusList );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckByTypeResultList( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID PrincipalSelfSid, HANDLE ClientToken, DWORD DesiredAccess, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, PPRIVILEGE_SET PrivilegeSet, LPDWORD PrivilegeSetLength, LPDWORD GrantedAccessList, LPDWORD AccessStatusList )
{
    static FN_AccessCheckByTypeResultList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckByTypeResultList", &g_Kernel32);
    return pfn( pSecurityDescriptor, PrincipalSelfSid, ClientToken, DesiredAccess, ObjectTypeList, ObjectTypeListLength, GenericMapping, PrivilegeSet, PrivilegeSetLength, GrantedAccessList, AccessStatusList );
}

typedef BOOL WINAPI FN_OpenProcessToken( HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_OpenProcessToken( HANDLE ProcessHandle, DWORD DesiredAccess, PHANDLE TokenHandle )
{
    static FN_OpenProcessToken *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenProcessToken", &g_Kernel32);
    return pfn( ProcessHandle, DesiredAccess, TokenHandle );
}

typedef BOOL WINAPI FN_OpenThreadToken( HANDLE ThreadHandle, DWORD DesiredAccess, BOOL OpenAsSelf, PHANDLE TokenHandle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_OpenThreadToken( HANDLE ThreadHandle, DWORD DesiredAccess, BOOL OpenAsSelf, PHANDLE TokenHandle )
{
    static FN_OpenThreadToken *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenThreadToken", &g_Kernel32);
    return pfn( ThreadHandle, DesiredAccess, OpenAsSelf, TokenHandle );
}

typedef BOOL WINAPI FN_GetTokenInformation( HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetTokenInformation( HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength, PDWORD ReturnLength )
{
    static FN_GetTokenInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTokenInformation", &g_Kernel32);
    return pfn( TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength, ReturnLength );
}

typedef BOOL WINAPI FN_SetTokenInformation( HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetTokenInformation( HANDLE TokenHandle, TOKEN_INFORMATION_CLASS TokenInformationClass, LPVOID TokenInformation, DWORD TokenInformationLength )
{
    static FN_SetTokenInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetTokenInformation", &g_Kernel32);
    return pfn( TokenHandle, TokenInformationClass, TokenInformation, TokenInformationLength );
}

typedef BOOL WINAPI FN_AdjustTokenPrivileges( HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AdjustTokenPrivileges( HANDLE TokenHandle, BOOL DisableAllPrivileges, PTOKEN_PRIVILEGES NewState, DWORD BufferLength, PTOKEN_PRIVILEGES PreviousState, PDWORD ReturnLength )
{
    static FN_AdjustTokenPrivileges *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AdjustTokenPrivileges", &g_Kernel32);
    return pfn( TokenHandle, DisableAllPrivileges, NewState, BufferLength, PreviousState, ReturnLength );
}

typedef BOOL WINAPI FN_AdjustTokenGroups( HANDLE TokenHandle, BOOL ResetToDefault, PTOKEN_GROUPS NewState, DWORD BufferLength, PTOKEN_GROUPS PreviousState, PDWORD ReturnLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AdjustTokenGroups( HANDLE TokenHandle, BOOL ResetToDefault, PTOKEN_GROUPS NewState, DWORD BufferLength, PTOKEN_GROUPS PreviousState, PDWORD ReturnLength )
{
    static FN_AdjustTokenGroups *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AdjustTokenGroups", &g_Kernel32);
    return pfn( TokenHandle, ResetToDefault, NewState, BufferLength, PreviousState, ReturnLength );
}

typedef BOOL WINAPI FN_PrivilegeCheck( HANDLE ClientToken, PPRIVILEGE_SET RequiredPrivileges, LPBOOL pfResult );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_PrivilegeCheck( HANDLE ClientToken, PPRIVILEGE_SET RequiredPrivileges, LPBOOL pfResult )
{
    static FN_PrivilegeCheck *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PrivilegeCheck", &g_Kernel32);
    return pfn( ClientToken, RequiredPrivileges, pfResult );
}

typedef BOOL WINAPI FN_AccessCheckAndAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, LPSTR ObjectTypeName, LPSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD DesiredAccess, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPBOOL AccessStatus, LPBOOL pfGenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckAndAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, LPSTR ObjectTypeName, LPSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD DesiredAccess, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPBOOL AccessStatus, LPBOOL pfGenerateOnClose )
{
    static FN_AccessCheckAndAuditAlarmA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckAndAuditAlarmA", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, pfGenerateOnClose );
}

typedef BOOL WINAPI FN_AccessCheckAndAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, LPWSTR ObjectTypeName, LPWSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD DesiredAccess, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPBOOL AccessStatus, LPBOOL pfGenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckAndAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, LPWSTR ObjectTypeName, LPWSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, DWORD DesiredAccess, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPBOOL AccessStatus, LPBOOL pfGenerateOnClose )
{
    static FN_AccessCheckAndAuditAlarmW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckAndAuditAlarmW", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, DesiredAccess, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, pfGenerateOnClose );
}

typedef BOOL WINAPI FN_AccessCheckByTypeAndAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, LPCSTR ObjectTypeName, LPCSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPBOOL AccessStatus, LPBOOL pfGenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckByTypeAndAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, LPCSTR ObjectTypeName, LPCSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPBOOL AccessStatus, LPBOOL pfGenerateOnClose )
{
    static FN_AccessCheckByTypeAndAuditAlarmA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckByTypeAndAuditAlarmA", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, pfGenerateOnClose );
}

typedef BOOL WINAPI FN_AccessCheckByTypeAndAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, LPCWSTR ObjectTypeName, LPCWSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPBOOL AccessStatus, LPBOOL pfGenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckByTypeAndAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, LPCWSTR ObjectTypeName, LPCWSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPBOOL AccessStatus, LPBOOL pfGenerateOnClose )
{
    static FN_AccessCheckByTypeAndAuditAlarmW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckByTypeAndAuditAlarmW", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatus, pfGenerateOnClose );
}

typedef BOOL WINAPI FN_AccessCheckByTypeResultListAndAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, LPCSTR ObjectTypeName, LPCSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPDWORD AccessStatusList, LPBOOL pfGenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckByTypeResultListAndAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, LPCSTR ObjectTypeName, LPCSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPDWORD AccessStatusList, LPBOOL pfGenerateOnClose )
{
    static FN_AccessCheckByTypeResultListAndAuditAlarmA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckByTypeResultListAndAuditAlarmA", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatusList, pfGenerateOnClose );
}

typedef BOOL WINAPI FN_AccessCheckByTypeResultListAndAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, LPCWSTR ObjectTypeName, LPCWSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPDWORD AccessStatusList, LPBOOL pfGenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckByTypeResultListAndAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, LPCWSTR ObjectTypeName, LPCWSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPDWORD AccessStatusList, LPBOOL pfGenerateOnClose )
{
    static FN_AccessCheckByTypeResultListAndAuditAlarmW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckByTypeResultListAndAuditAlarmW", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatusList, pfGenerateOnClose );
}

typedef BOOL WINAPI FN_AccessCheckByTypeResultListAndAuditAlarmByHandleA( LPCSTR SubsystemName, LPVOID HandleId, HANDLE ClientToken, LPCSTR ObjectTypeName, LPCSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPDWORD AccessStatusList, LPBOOL pfGenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckByTypeResultListAndAuditAlarmByHandleA( LPCSTR SubsystemName, LPVOID HandleId, HANDLE ClientToken, LPCSTR ObjectTypeName, LPCSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPDWORD AccessStatusList, LPBOOL pfGenerateOnClose )
{
    static FN_AccessCheckByTypeResultListAndAuditAlarmByHandleA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckByTypeResultListAndAuditAlarmByHandleA", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatusList, pfGenerateOnClose );
}

typedef BOOL WINAPI FN_AccessCheckByTypeResultListAndAuditAlarmByHandleW( LPCWSTR SubsystemName, LPVOID HandleId, HANDLE ClientToken, LPCWSTR ObjectTypeName, LPCWSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPDWORD AccessStatusList, LPBOOL pfGenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AccessCheckByTypeResultListAndAuditAlarmByHandleW( LPCWSTR SubsystemName, LPVOID HandleId, HANDLE ClientToken, LPCWSTR ObjectTypeName, LPCWSTR ObjectName, PSECURITY_DESCRIPTOR SecurityDescriptor, PSID PrincipalSelfSid, DWORD DesiredAccess, AUDIT_EVENT_TYPE AuditType, DWORD Flags, POBJECT_TYPE_LIST ObjectTypeList, DWORD ObjectTypeListLength, PGENERIC_MAPPING GenericMapping, BOOL ObjectCreation, LPDWORD GrantedAccess, LPDWORD AccessStatusList, LPBOOL pfGenerateOnClose )
{
    static FN_AccessCheckByTypeResultListAndAuditAlarmByHandleW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AccessCheckByTypeResultListAndAuditAlarmByHandleW", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ClientToken, ObjectTypeName, ObjectName, SecurityDescriptor, PrincipalSelfSid, DesiredAccess, AuditType, Flags, ObjectTypeList, ObjectTypeListLength, GenericMapping, ObjectCreation, GrantedAccess, AccessStatusList, pfGenerateOnClose );
}

typedef BOOL WINAPI FN_ObjectOpenAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, LPSTR ObjectTypeName, LPSTR ObjectName, PSECURITY_DESCRIPTOR pSecurityDescriptor, HANDLE ClientToken, DWORD DesiredAccess, DWORD GrantedAccess, PPRIVILEGE_SET Privileges, BOOL ObjectCreation, BOOL AccessGranted, LPBOOL GenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ObjectOpenAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, LPSTR ObjectTypeName, LPSTR ObjectName, PSECURITY_DESCRIPTOR pSecurityDescriptor, HANDLE ClientToken, DWORD DesiredAccess, DWORD GrantedAccess, PPRIVILEGE_SET Privileges, BOOL ObjectCreation, BOOL AccessGranted, LPBOOL GenerateOnClose )
{
    static FN_ObjectOpenAuditAlarmA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ObjectOpenAuditAlarmA", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ObjectTypeName, ObjectName, pSecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose );
}

typedef BOOL WINAPI FN_ObjectOpenAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, LPWSTR ObjectTypeName, LPWSTR ObjectName, PSECURITY_DESCRIPTOR pSecurityDescriptor, HANDLE ClientToken, DWORD DesiredAccess, DWORD GrantedAccess, PPRIVILEGE_SET Privileges, BOOL ObjectCreation, BOOL AccessGranted, LPBOOL GenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ObjectOpenAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, LPWSTR ObjectTypeName, LPWSTR ObjectName, PSECURITY_DESCRIPTOR pSecurityDescriptor, HANDLE ClientToken, DWORD DesiredAccess, DWORD GrantedAccess, PPRIVILEGE_SET Privileges, BOOL ObjectCreation, BOOL AccessGranted, LPBOOL GenerateOnClose )
{
    static FN_ObjectOpenAuditAlarmW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ObjectOpenAuditAlarmW", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ObjectTypeName, ObjectName, pSecurityDescriptor, ClientToken, DesiredAccess, GrantedAccess, Privileges, ObjectCreation, AccessGranted, GenerateOnClose );
}

typedef BOOL WINAPI FN_ObjectPrivilegeAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, HANDLE ClientToken, DWORD DesiredAccess, PPRIVILEGE_SET Privileges, BOOL AccessGranted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ObjectPrivilegeAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, HANDLE ClientToken, DWORD DesiredAccess, PPRIVILEGE_SET Privileges, BOOL AccessGranted )
{
    static FN_ObjectPrivilegeAuditAlarmA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ObjectPrivilegeAuditAlarmA", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted );
}

typedef BOOL WINAPI FN_ObjectPrivilegeAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, HANDLE ClientToken, DWORD DesiredAccess, PPRIVILEGE_SET Privileges, BOOL AccessGranted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ObjectPrivilegeAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, HANDLE ClientToken, DWORD DesiredAccess, PPRIVILEGE_SET Privileges, BOOL AccessGranted )
{
    static FN_ObjectPrivilegeAuditAlarmW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ObjectPrivilegeAuditAlarmW", &g_Kernel32);
    return pfn( SubsystemName, HandleId, ClientToken, DesiredAccess, Privileges, AccessGranted );
}

typedef BOOL WINAPI FN_ObjectCloseAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, BOOL GenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ObjectCloseAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, BOOL GenerateOnClose )
{
    static FN_ObjectCloseAuditAlarmA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ObjectCloseAuditAlarmA", &g_Kernel32);
    return pfn( SubsystemName, HandleId, GenerateOnClose );
}

typedef BOOL WINAPI FN_ObjectCloseAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, BOOL GenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ObjectCloseAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, BOOL GenerateOnClose )
{
    static FN_ObjectCloseAuditAlarmW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ObjectCloseAuditAlarmW", &g_Kernel32);
    return pfn( SubsystemName, HandleId, GenerateOnClose );
}

typedef BOOL WINAPI FN_ObjectDeleteAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, BOOL GenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ObjectDeleteAuditAlarmA( LPCSTR SubsystemName, LPVOID HandleId, BOOL GenerateOnClose )
{
    static FN_ObjectDeleteAuditAlarmA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ObjectDeleteAuditAlarmA", &g_Kernel32);
    return pfn( SubsystemName, HandleId, GenerateOnClose );
}

typedef BOOL WINAPI FN_ObjectDeleteAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, BOOL GenerateOnClose );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ObjectDeleteAuditAlarmW( LPCWSTR SubsystemName, LPVOID HandleId, BOOL GenerateOnClose )
{
    static FN_ObjectDeleteAuditAlarmW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ObjectDeleteAuditAlarmW", &g_Kernel32);
    return pfn( SubsystemName, HandleId, GenerateOnClose );
}

typedef BOOL WINAPI FN_PrivilegedServiceAuditAlarmA( LPCSTR SubsystemName, LPCSTR ServiceName, HANDLE ClientToken, PPRIVILEGE_SET Privileges, BOOL AccessGranted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_PrivilegedServiceAuditAlarmA( LPCSTR SubsystemName, LPCSTR ServiceName, HANDLE ClientToken, PPRIVILEGE_SET Privileges, BOOL AccessGranted )
{
    static FN_PrivilegedServiceAuditAlarmA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PrivilegedServiceAuditAlarmA", &g_Kernel32);
    return pfn( SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted );
}

typedef BOOL WINAPI FN_PrivilegedServiceAuditAlarmW( LPCWSTR SubsystemName, LPCWSTR ServiceName, HANDLE ClientToken, PPRIVILEGE_SET Privileges, BOOL AccessGranted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_PrivilegedServiceAuditAlarmW( LPCWSTR SubsystemName, LPCWSTR ServiceName, HANDLE ClientToken, PPRIVILEGE_SET Privileges, BOOL AccessGranted )
{
    static FN_PrivilegedServiceAuditAlarmW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PrivilegedServiceAuditAlarmW", &g_Kernel32);
    return pfn( SubsystemName, ServiceName, ClientToken, Privileges, AccessGranted );
}

typedef BOOL WINAPI FN_IsWellKnownSid( PSID pSid, WELL_KNOWN_SID_TYPE WellKnownSidType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsWellKnownSid( PSID pSid, WELL_KNOWN_SID_TYPE WellKnownSidType )
{
    static FN_IsWellKnownSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsWellKnownSid", &g_Kernel32);
    return pfn( pSid, WellKnownSidType );
}

typedef BOOL WINAPI FN_CreateWellKnownSid( WELL_KNOWN_SID_TYPE WellKnownSidType, PSID DomainSid, PSID pSid, DWORD * cbSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateWellKnownSid( WELL_KNOWN_SID_TYPE WellKnownSidType, PSID DomainSid, PSID pSid, DWORD * cbSid )
{
    static FN_CreateWellKnownSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateWellKnownSid", &g_Kernel32);
    return pfn( WellKnownSidType, DomainSid, pSid, cbSid );
}

typedef BOOL WINAPI FN_EqualDomainSid( PSID pSid1, PSID pSid2, BOOL * pfEqual );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EqualDomainSid( PSID pSid1, PSID pSid2, BOOL * pfEqual )
{
    static FN_EqualDomainSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EqualDomainSid", &g_Kernel32);
    return pfn( pSid1, pSid2, pfEqual );
}

typedef BOOL WINAPI FN_GetWindowsAccountDomainSid( PSID pSid, PSID pDomainSid, DWORD * cbDomainSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetWindowsAccountDomainSid( PSID pSid, PSID pDomainSid, DWORD * cbDomainSid )
{
    static FN_GetWindowsAccountDomainSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetWindowsAccountDomainSid", &g_Kernel32);
    return pfn( pSid, pDomainSid, cbDomainSid );
}

typedef BOOL WINAPI FN_IsValidSid( PSID pSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsValidSid( PSID pSid )
{
    static FN_IsValidSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsValidSid", &g_Kernel32);
    return pfn( pSid );
}

typedef BOOL WINAPI FN_EqualSid( PSID pSid1, PSID pSid2 );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EqualSid( PSID pSid1, PSID pSid2 )
{
    static FN_EqualSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EqualSid", &g_Kernel32);
    return pfn( pSid1, pSid2 );
}

typedef BOOL WINAPI FN_EqualPrefixSid( PSID pSid1, PSID pSid2 );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EqualPrefixSid( PSID pSid1, PSID pSid2 )
{
    static FN_EqualPrefixSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EqualPrefixSid", &g_Kernel32);
    return pfn( pSid1, pSid2 );
}

typedef DWORD WINAPI FN_GetSidLengthRequired( UCHAR nSubAuthorityCount );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetSidLengthRequired( UCHAR nSubAuthorityCount )
{
    static FN_GetSidLengthRequired *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSidLengthRequired", &g_Kernel32);
    return pfn( nSubAuthorityCount );
}

typedef BOOL WINAPI FN_AllocateAndInitializeSid( PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount, DWORD nSubAuthority0, DWORD nSubAuthority1, DWORD nSubAuthority2, DWORD nSubAuthority3, DWORD nSubAuthority4, DWORD nSubAuthority5, DWORD nSubAuthority6, DWORD nSubAuthority7, PSID * pSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AllocateAndInitializeSid( PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount, DWORD nSubAuthority0, DWORD nSubAuthority1, DWORD nSubAuthority2, DWORD nSubAuthority3, DWORD nSubAuthority4, DWORD nSubAuthority5, DWORD nSubAuthority6, DWORD nSubAuthority7, PSID * pSid )
{
    static FN_AllocateAndInitializeSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AllocateAndInitializeSid", &g_Kernel32);
    return pfn( pIdentifierAuthority, nSubAuthorityCount, nSubAuthority0, nSubAuthority1, nSubAuthority2, nSubAuthority3, nSubAuthority4, nSubAuthority5, nSubAuthority6, nSubAuthority7, pSid );
}

typedef PVOID WINAPI FN_FreeSid( PSID pSid );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_FreeSid( PSID pSid )
{
    static FN_FreeSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FreeSid", &g_Kernel32);
    return pfn( pSid );
}

typedef BOOL WINAPI FN_InitializeSid( PSID Sid, PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_InitializeSid( PSID Sid, PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority, BYTE nSubAuthorityCount )
{
    static FN_InitializeSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InitializeSid", &g_Kernel32);
    return pfn( Sid, pIdentifierAuthority, nSubAuthorityCount );
}

typedef PSID_IDENTIFIER_AUTHORITY WINAPI FN_GetSidIdentifierAuthority( PSID pSid );
__declspec(dllexport) PSID_IDENTIFIER_AUTHORITY WINAPI kPrf2Wrap_GetSidIdentifierAuthority( PSID pSid )
{
    static FN_GetSidIdentifierAuthority *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSidIdentifierAuthority", &g_Kernel32);
    return pfn( pSid );
}

typedef PDWORD WINAPI FN_GetSidSubAuthority( PSID pSid, DWORD nSubAuthority );
__declspec(dllexport) PDWORD WINAPI kPrf2Wrap_GetSidSubAuthority( PSID pSid, DWORD nSubAuthority )
{
    static FN_GetSidSubAuthority *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSidSubAuthority", &g_Kernel32);
    return pfn( pSid, nSubAuthority );
}

typedef PUCHAR WINAPI FN_GetSidSubAuthorityCount( PSID pSid );
__declspec(dllexport) PUCHAR WINAPI kPrf2Wrap_GetSidSubAuthorityCount( PSID pSid )
{
    static FN_GetSidSubAuthorityCount *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSidSubAuthorityCount", &g_Kernel32);
    return pfn( pSid );
}

typedef DWORD WINAPI FN_GetLengthSid( PSID pSid );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetLengthSid( PSID pSid )
{
    static FN_GetLengthSid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLengthSid", &g_Kernel32);
    return pfn( pSid );
}

typedef BOOL WINAPI FN_CopySid( DWORD nDestinationSidLength, PSID pDestinationSid, PSID pSourceSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CopySid( DWORD nDestinationSidLength, PSID pDestinationSid, PSID pSourceSid )
{
    static FN_CopySid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CopySid", &g_Kernel32);
    return pfn( nDestinationSidLength, pDestinationSid, pSourceSid );
}

typedef BOOL WINAPI FN_AreAllAccessesGranted( DWORD GrantedAccess, DWORD DesiredAccess );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AreAllAccessesGranted( DWORD GrantedAccess, DWORD DesiredAccess )
{
    static FN_AreAllAccessesGranted *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AreAllAccessesGranted", &g_Kernel32);
    return pfn( GrantedAccess, DesiredAccess );
}

typedef BOOL WINAPI FN_AreAnyAccessesGranted( DWORD GrantedAccess, DWORD DesiredAccess );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AreAnyAccessesGranted( DWORD GrantedAccess, DWORD DesiredAccess )
{
    static FN_AreAnyAccessesGranted *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AreAnyAccessesGranted", &g_Kernel32);
    return pfn( GrantedAccess, DesiredAccess );
}

typedef VOID WINAPI FN_MapGenericMask( PDWORD AccessMask, PGENERIC_MAPPING GenericMapping );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_MapGenericMask( PDWORD AccessMask, PGENERIC_MAPPING GenericMapping )
{
    static FN_MapGenericMask *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MapGenericMask", &g_Kernel32);
    pfn( AccessMask, GenericMapping );
}

typedef BOOL WINAPI FN_IsValidAcl( PACL pAcl );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsValidAcl( PACL pAcl )
{
    static FN_IsValidAcl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsValidAcl", &g_Kernel32);
    return pfn( pAcl );
}

typedef BOOL WINAPI FN_InitializeAcl( PACL pAcl, DWORD nAclLength, DWORD dwAclRevision );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_InitializeAcl( PACL pAcl, DWORD nAclLength, DWORD dwAclRevision )
{
    static FN_InitializeAcl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InitializeAcl", &g_Kernel32);
    return pfn( pAcl, nAclLength, dwAclRevision );
}

typedef BOOL WINAPI FN_GetAclInformation( PACL pAcl, LPVOID pAclInformation, DWORD nAclInformationLength, ACL_INFORMATION_CLASS dwAclInformationClass );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetAclInformation( PACL pAcl, LPVOID pAclInformation, DWORD nAclInformationLength, ACL_INFORMATION_CLASS dwAclInformationClass )
{
    static FN_GetAclInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetAclInformation", &g_Kernel32);
    return pfn( pAcl, pAclInformation, nAclInformationLength, dwAclInformationClass );
}

typedef BOOL WINAPI FN_SetAclInformation( PACL pAcl, LPVOID pAclInformation, DWORD nAclInformationLength, ACL_INFORMATION_CLASS dwAclInformationClass );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetAclInformation( PACL pAcl, LPVOID pAclInformation, DWORD nAclInformationLength, ACL_INFORMATION_CLASS dwAclInformationClass )
{
    static FN_SetAclInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetAclInformation", &g_Kernel32);
    return pfn( pAcl, pAclInformation, nAclInformationLength, dwAclInformationClass );
}

typedef BOOL WINAPI FN_AddAce( PACL pAcl, DWORD dwAceRevision, DWORD dwStartingAceIndex, LPVOID pAceList, DWORD nAceListLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAce( PACL pAcl, DWORD dwAceRevision, DWORD dwStartingAceIndex, LPVOID pAceList, DWORD nAceListLength )
{
    static FN_AddAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAce", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, dwStartingAceIndex, pAceList, nAceListLength );
}

typedef BOOL WINAPI FN_DeleteAce( PACL pAcl, DWORD dwAceIndex );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeleteAce( PACL pAcl, DWORD dwAceIndex )
{
    static FN_DeleteAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteAce", &g_Kernel32);
    return pfn( pAcl, dwAceIndex );
}

typedef BOOL WINAPI FN_GetAce( PACL pAcl, DWORD dwAceIndex, LPVOID * pAce );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetAce( PACL pAcl, DWORD dwAceIndex, LPVOID * pAce )
{
    static FN_GetAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetAce", &g_Kernel32);
    return pfn( pAcl, dwAceIndex, pAce );
}

typedef BOOL WINAPI FN_AddAccessAllowedAce( PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAccessAllowedAce( PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid )
{
    static FN_AddAccessAllowedAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAccessAllowedAce", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, AccessMask, pSid );
}

typedef BOOL WINAPI FN_AddAccessAllowedAceEx( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, PSID pSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAccessAllowedAceEx( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, PSID pSid )
{
    static FN_AddAccessAllowedAceEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAccessAllowedAceEx", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, AceFlags, AccessMask, pSid );
}

typedef BOOL WINAPI FN_AddAccessDeniedAce( PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAccessDeniedAce( PACL pAcl, DWORD dwAceRevision, DWORD AccessMask, PSID pSid )
{
    static FN_AddAccessDeniedAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAccessDeniedAce", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, AccessMask, pSid );
}

typedef BOOL WINAPI FN_AddAccessDeniedAceEx( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, PSID pSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAccessDeniedAceEx( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, PSID pSid )
{
    static FN_AddAccessDeniedAceEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAccessDeniedAceEx", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, AceFlags, AccessMask, pSid );
}

typedef BOOL WINAPI FN_AddAuditAccessAce( PACL pAcl, DWORD dwAceRevision, DWORD dwAccessMask, PSID pSid, BOOL bAuditSuccess, BOOL bAuditFailure );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAuditAccessAce( PACL pAcl, DWORD dwAceRevision, DWORD dwAccessMask, PSID pSid, BOOL bAuditSuccess, BOOL bAuditFailure )
{
    static FN_AddAuditAccessAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAuditAccessAce", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, dwAccessMask, pSid, bAuditSuccess, bAuditFailure );
}

typedef BOOL WINAPI FN_AddAuditAccessAceEx( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD dwAccessMask, PSID pSid, BOOL bAuditSuccess, BOOL bAuditFailure );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAuditAccessAceEx( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD dwAccessMask, PSID pSid, BOOL bAuditSuccess, BOOL bAuditFailure )
{
    static FN_AddAuditAccessAceEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAuditAccessAceEx", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, AceFlags, dwAccessMask, pSid, bAuditSuccess, bAuditFailure );
}

typedef BOOL WINAPI FN_AddAccessAllowedObjectAce( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, GUID * ObjectTypeGuid, GUID * InheritedObjectTypeGuid, PSID pSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAccessAllowedObjectAce( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, GUID * ObjectTypeGuid, GUID * InheritedObjectTypeGuid, PSID pSid )
{
    static FN_AddAccessAllowedObjectAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAccessAllowedObjectAce", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, AceFlags, AccessMask, ObjectTypeGuid, InheritedObjectTypeGuid, pSid );
}

typedef BOOL WINAPI FN_AddAccessDeniedObjectAce( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, GUID * ObjectTypeGuid, GUID * InheritedObjectTypeGuid, PSID pSid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAccessDeniedObjectAce( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, GUID * ObjectTypeGuid, GUID * InheritedObjectTypeGuid, PSID pSid )
{
    static FN_AddAccessDeniedObjectAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAccessDeniedObjectAce", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, AceFlags, AccessMask, ObjectTypeGuid, InheritedObjectTypeGuid, pSid );
}

typedef BOOL WINAPI FN_AddAuditAccessObjectAce( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, GUID * ObjectTypeGuid, GUID * InheritedObjectTypeGuid, PSID pSid, BOOL bAuditSuccess, BOOL bAuditFailure );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AddAuditAccessObjectAce( PACL pAcl, DWORD dwAceRevision, DWORD AceFlags, DWORD AccessMask, GUID * ObjectTypeGuid, GUID * InheritedObjectTypeGuid, PSID pSid, BOOL bAuditSuccess, BOOL bAuditFailure )
{
    static FN_AddAuditAccessObjectAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddAuditAccessObjectAce", &g_Kernel32);
    return pfn( pAcl, dwAceRevision, AceFlags, AccessMask, ObjectTypeGuid, InheritedObjectTypeGuid, pSid, bAuditSuccess, bAuditFailure );
}

typedef BOOL WINAPI FN_FindFirstFreeAce( PACL pAcl, LPVOID * pAce );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindFirstFreeAce( PACL pAcl, LPVOID * pAce )
{
    static FN_FindFirstFreeAce *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstFreeAce", &g_Kernel32);
    return pfn( pAcl, pAce );
}

typedef BOOL WINAPI FN_InitializeSecurityDescriptor( PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD dwRevision );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_InitializeSecurityDescriptor( PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD dwRevision )
{
    static FN_InitializeSecurityDescriptor *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "InitializeSecurityDescriptor", &g_Kernel32);
    return pfn( pSecurityDescriptor, dwRevision );
}

typedef BOOL WINAPI FN_IsValidSecurityDescriptor( PSECURITY_DESCRIPTOR pSecurityDescriptor );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsValidSecurityDescriptor( PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
    static FN_IsValidSecurityDescriptor *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsValidSecurityDescriptor", &g_Kernel32);
    return pfn( pSecurityDescriptor );
}

typedef DWORD WINAPI FN_GetSecurityDescriptorLength( PSECURITY_DESCRIPTOR pSecurityDescriptor );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetSecurityDescriptorLength( PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
    static FN_GetSecurityDescriptorLength *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSecurityDescriptorLength", &g_Kernel32);
    return pfn( pSecurityDescriptor );
}

typedef BOOL WINAPI FN_GetSecurityDescriptorControl( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSECURITY_DESCRIPTOR_CONTROL pControl, LPDWORD lpdwRevision );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSecurityDescriptorControl( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSECURITY_DESCRIPTOR_CONTROL pControl, LPDWORD lpdwRevision )
{
    static FN_GetSecurityDescriptorControl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSecurityDescriptorControl", &g_Kernel32);
    return pfn( pSecurityDescriptor, pControl, lpdwRevision );
}

typedef BOOL WINAPI FN_SetSecurityDescriptorControl( PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest, SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetSecurityDescriptorControl( PSECURITY_DESCRIPTOR pSecurityDescriptor, SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest, SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet )
{
    static FN_SetSecurityDescriptorControl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSecurityDescriptorControl", &g_Kernel32);
    return pfn( pSecurityDescriptor, ControlBitsOfInterest, ControlBitsToSet );
}

typedef BOOL WINAPI FN_SetSecurityDescriptorDacl( PSECURITY_DESCRIPTOR pSecurityDescriptor, BOOL bDaclPresent, PACL pDacl, BOOL bDaclDefaulted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetSecurityDescriptorDacl( PSECURITY_DESCRIPTOR pSecurityDescriptor, BOOL bDaclPresent, PACL pDacl, BOOL bDaclDefaulted )
{
    static FN_SetSecurityDescriptorDacl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSecurityDescriptorDacl", &g_Kernel32);
    return pfn( pSecurityDescriptor, bDaclPresent, pDacl, bDaclDefaulted );
}

typedef BOOL WINAPI FN_GetSecurityDescriptorDacl( PSECURITY_DESCRIPTOR pSecurityDescriptor, LPBOOL lpbDaclPresent, PACL * pDacl, LPBOOL lpbDaclDefaulted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSecurityDescriptorDacl( PSECURITY_DESCRIPTOR pSecurityDescriptor, LPBOOL lpbDaclPresent, PACL * pDacl, LPBOOL lpbDaclDefaulted )
{
    static FN_GetSecurityDescriptorDacl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSecurityDescriptorDacl", &g_Kernel32);
    return pfn( pSecurityDescriptor, lpbDaclPresent, pDacl, lpbDaclDefaulted );
}

typedef BOOL WINAPI FN_SetSecurityDescriptorSacl( PSECURITY_DESCRIPTOR pSecurityDescriptor, BOOL bSaclPresent, PACL pSacl, BOOL bSaclDefaulted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetSecurityDescriptorSacl( PSECURITY_DESCRIPTOR pSecurityDescriptor, BOOL bSaclPresent, PACL pSacl, BOOL bSaclDefaulted )
{
    static FN_SetSecurityDescriptorSacl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSecurityDescriptorSacl", &g_Kernel32);
    return pfn( pSecurityDescriptor, bSaclPresent, pSacl, bSaclDefaulted );
}

typedef BOOL WINAPI FN_GetSecurityDescriptorSacl( PSECURITY_DESCRIPTOR pSecurityDescriptor, LPBOOL lpbSaclPresent, PACL * pSacl, LPBOOL lpbSaclDefaulted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSecurityDescriptorSacl( PSECURITY_DESCRIPTOR pSecurityDescriptor, LPBOOL lpbSaclPresent, PACL * pSacl, LPBOOL lpbSaclDefaulted )
{
    static FN_GetSecurityDescriptorSacl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSecurityDescriptorSacl", &g_Kernel32);
    return pfn( pSecurityDescriptor, lpbSaclPresent, pSacl, lpbSaclDefaulted );
}

typedef BOOL WINAPI FN_SetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID pOwner, BOOL bOwnerDefaulted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID pOwner, BOOL bOwnerDefaulted )
{
    static FN_SetSecurityDescriptorOwner *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSecurityDescriptorOwner", &g_Kernel32);
    return pfn( pSecurityDescriptor, pOwner, bOwnerDefaulted );
}

typedef BOOL WINAPI FN_GetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID * pOwner, LPBOOL lpbOwnerDefaulted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSecurityDescriptorOwner( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID * pOwner, LPBOOL lpbOwnerDefaulted )
{
    static FN_GetSecurityDescriptorOwner *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSecurityDescriptorOwner", &g_Kernel32);
    return pfn( pSecurityDescriptor, pOwner, lpbOwnerDefaulted );
}

typedef BOOL WINAPI FN_SetSecurityDescriptorGroup( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID pGroup, BOOL bGroupDefaulted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetSecurityDescriptorGroup( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID pGroup, BOOL bGroupDefaulted )
{
    static FN_SetSecurityDescriptorGroup *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSecurityDescriptorGroup", &g_Kernel32);
    return pfn( pSecurityDescriptor, pGroup, bGroupDefaulted );
}

typedef BOOL WINAPI FN_GetSecurityDescriptorGroup( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID * pGroup, LPBOOL lpbGroupDefaulted );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSecurityDescriptorGroup( PSECURITY_DESCRIPTOR pSecurityDescriptor, PSID * pGroup, LPBOOL lpbGroupDefaulted )
{
    static FN_GetSecurityDescriptorGroup *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSecurityDescriptorGroup", &g_Kernel32);
    return pfn( pSecurityDescriptor, pGroup, lpbGroupDefaulted );
}

typedef DWORD WINAPI FN_SetSecurityDescriptorRMControl( PSECURITY_DESCRIPTOR SecurityDescriptor, PUCHAR RMControl );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_SetSecurityDescriptorRMControl( PSECURITY_DESCRIPTOR SecurityDescriptor, PUCHAR RMControl )
{
    static FN_SetSecurityDescriptorRMControl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSecurityDescriptorRMControl", &g_Kernel32);
    return pfn( SecurityDescriptor, RMControl );
}

typedef DWORD WINAPI FN_GetSecurityDescriptorRMControl( PSECURITY_DESCRIPTOR SecurityDescriptor, PUCHAR RMControl );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetSecurityDescriptorRMControl( PSECURITY_DESCRIPTOR SecurityDescriptor, PUCHAR RMControl )
{
    static FN_GetSecurityDescriptorRMControl *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSecurityDescriptorRMControl", &g_Kernel32);
    return pfn( SecurityDescriptor, RMControl );
}

typedef BOOL WINAPI FN_CreatePrivateObjectSecurity( PSECURITY_DESCRIPTOR ParentDescriptor, PSECURITY_DESCRIPTOR CreatorDescriptor, PSECURITY_DESCRIPTOR * NewDescriptor, BOOL IsDirectoryObject, HANDLE Token, PGENERIC_MAPPING GenericMapping );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreatePrivateObjectSecurity( PSECURITY_DESCRIPTOR ParentDescriptor, PSECURITY_DESCRIPTOR CreatorDescriptor, PSECURITY_DESCRIPTOR * NewDescriptor, BOOL IsDirectoryObject, HANDLE Token, PGENERIC_MAPPING GenericMapping )
{
    static FN_CreatePrivateObjectSecurity *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreatePrivateObjectSecurity", &g_Kernel32);
    return pfn( ParentDescriptor, CreatorDescriptor, NewDescriptor, IsDirectoryObject, Token, GenericMapping );
}

typedef BOOL WINAPI FN_ConvertToAutoInheritPrivateObjectSecurity( PSECURITY_DESCRIPTOR ParentDescriptor, PSECURITY_DESCRIPTOR CurrentSecurityDescriptor, PSECURITY_DESCRIPTOR * NewSecurityDescriptor, GUID * ObjectType, BOOLEAN IsDirectoryObject, PGENERIC_MAPPING GenericMapping );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ConvertToAutoInheritPrivateObjectSecurity( PSECURITY_DESCRIPTOR ParentDescriptor, PSECURITY_DESCRIPTOR CurrentSecurityDescriptor, PSECURITY_DESCRIPTOR * NewSecurityDescriptor, GUID * ObjectType, BOOLEAN IsDirectoryObject, PGENERIC_MAPPING GenericMapping )
{
    static FN_ConvertToAutoInheritPrivateObjectSecurity *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ConvertToAutoInheritPrivateObjectSecurity", &g_Kernel32);
    return pfn( ParentDescriptor, CurrentSecurityDescriptor, NewSecurityDescriptor, ObjectType, IsDirectoryObject, GenericMapping );
}

typedef BOOL WINAPI FN_CreatePrivateObjectSecurityEx( PSECURITY_DESCRIPTOR ParentDescriptor, PSECURITY_DESCRIPTOR CreatorDescriptor, PSECURITY_DESCRIPTOR * NewDescriptor, GUID * ObjectType, BOOL IsContainerObject, ULONG AutoInheritFlags, HANDLE Token, PGENERIC_MAPPING GenericMapping );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreatePrivateObjectSecurityEx( PSECURITY_DESCRIPTOR ParentDescriptor, PSECURITY_DESCRIPTOR CreatorDescriptor, PSECURITY_DESCRIPTOR * NewDescriptor, GUID * ObjectType, BOOL IsContainerObject, ULONG AutoInheritFlags, HANDLE Token, PGENERIC_MAPPING GenericMapping )
{
    static FN_CreatePrivateObjectSecurityEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreatePrivateObjectSecurityEx", &g_Kernel32);
    return pfn( ParentDescriptor, CreatorDescriptor, NewDescriptor, ObjectType, IsContainerObject, AutoInheritFlags, Token, GenericMapping );
}

typedef BOOL WINAPI FN_CreatePrivateObjectSecurityWithMultipleInheritance( PSECURITY_DESCRIPTOR ParentDescriptor, PSECURITY_DESCRIPTOR CreatorDescriptor, PSECURITY_DESCRIPTOR * NewDescriptor, GUID * *ObjectTypes, ULONG GuidCount, BOOL IsContainerObject, ULONG AutoInheritFlags, HANDLE Token, PGENERIC_MAPPING GenericMapping );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreatePrivateObjectSecurityWithMultipleInheritance( PSECURITY_DESCRIPTOR ParentDescriptor, PSECURITY_DESCRIPTOR CreatorDescriptor, PSECURITY_DESCRIPTOR * NewDescriptor, GUID * *ObjectTypes, ULONG GuidCount, BOOL IsContainerObject, ULONG AutoInheritFlags, HANDLE Token, PGENERIC_MAPPING GenericMapping )
{
    static FN_CreatePrivateObjectSecurityWithMultipleInheritance *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreatePrivateObjectSecurityWithMultipleInheritance", &g_Kernel32);
    return pfn( ParentDescriptor, CreatorDescriptor, NewDescriptor, ObjectTypes, GuidCount, IsContainerObject, AutoInheritFlags, Token, GenericMapping );
}

typedef BOOL WINAPI FN_SetPrivateObjectSecurity( SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR ModificationDescriptor, PSECURITY_DESCRIPTOR * ObjectsSecurityDescriptor, PGENERIC_MAPPING GenericMapping, HANDLE Token );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetPrivateObjectSecurity( SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR ModificationDescriptor, PSECURITY_DESCRIPTOR * ObjectsSecurityDescriptor, PGENERIC_MAPPING GenericMapping, HANDLE Token )
{
    static FN_SetPrivateObjectSecurity *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetPrivateObjectSecurity", &g_Kernel32);
    return pfn( SecurityInformation, ModificationDescriptor, ObjectsSecurityDescriptor, GenericMapping, Token );
}

typedef BOOL WINAPI FN_SetPrivateObjectSecurityEx( SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR ModificationDescriptor, PSECURITY_DESCRIPTOR * ObjectsSecurityDescriptor, ULONG AutoInheritFlags, PGENERIC_MAPPING GenericMapping, HANDLE Token );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetPrivateObjectSecurityEx( SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR ModificationDescriptor, PSECURITY_DESCRIPTOR * ObjectsSecurityDescriptor, ULONG AutoInheritFlags, PGENERIC_MAPPING GenericMapping, HANDLE Token )
{
    static FN_SetPrivateObjectSecurityEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetPrivateObjectSecurityEx", &g_Kernel32);
    return pfn( SecurityInformation, ModificationDescriptor, ObjectsSecurityDescriptor, AutoInheritFlags, GenericMapping, Token );
}

typedef BOOL WINAPI FN_GetPrivateObjectSecurity( PSECURITY_DESCRIPTOR ObjectDescriptor, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR ResultantDescriptor, DWORD DescriptorLength, PDWORD ReturnLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetPrivateObjectSecurity( PSECURITY_DESCRIPTOR ObjectDescriptor, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR ResultantDescriptor, DWORD DescriptorLength, PDWORD ReturnLength )
{
    static FN_GetPrivateObjectSecurity *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPrivateObjectSecurity", &g_Kernel32);
    return pfn( ObjectDescriptor, SecurityInformation, ResultantDescriptor, DescriptorLength, ReturnLength );
}

typedef BOOL WINAPI FN_DestroyPrivateObjectSecurity( PSECURITY_DESCRIPTOR * ObjectDescriptor );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DestroyPrivateObjectSecurity( PSECURITY_DESCRIPTOR * ObjectDescriptor )
{
    static FN_DestroyPrivateObjectSecurity *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DestroyPrivateObjectSecurity", &g_Kernel32);
    return pfn( ObjectDescriptor );
}

typedef BOOL WINAPI FN_MakeSelfRelativeSD( PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor, PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor, LPDWORD lpdwBufferLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MakeSelfRelativeSD( PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor, PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor, LPDWORD lpdwBufferLength )
{
    static FN_MakeSelfRelativeSD *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MakeSelfRelativeSD", &g_Kernel32);
    return pfn( pAbsoluteSecurityDescriptor, pSelfRelativeSecurityDescriptor, lpdwBufferLength );
}

typedef BOOL WINAPI FN_MakeAbsoluteSD( PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor, PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor, LPDWORD lpdwAbsoluteSecurityDescriptorSize, PACL pDacl, LPDWORD lpdwDaclSize, PACL pSacl, LPDWORD lpdwSaclSize, PSID pOwner, LPDWORD lpdwOwnerSize, PSID pPrimaryGroup, LPDWORD lpdwPrimaryGroupSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MakeAbsoluteSD( PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor, PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor, LPDWORD lpdwAbsoluteSecurityDescriptorSize, PACL pDacl, LPDWORD lpdwDaclSize, PACL pSacl, LPDWORD lpdwSaclSize, PSID pOwner, LPDWORD lpdwOwnerSize, PSID pPrimaryGroup, LPDWORD lpdwPrimaryGroupSize )
{
    static FN_MakeAbsoluteSD *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MakeAbsoluteSD", &g_Kernel32);
    return pfn( pSelfRelativeSecurityDescriptor, pAbsoluteSecurityDescriptor, lpdwAbsoluteSecurityDescriptorSize, pDacl, lpdwDaclSize, pSacl, lpdwSaclSize, pOwner, lpdwOwnerSize, pPrimaryGroup, lpdwPrimaryGroupSize );
}

typedef BOOL WINAPI FN_MakeAbsoluteSD2( PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor, LPDWORD lpdwBufferSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MakeAbsoluteSD2( PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor, LPDWORD lpdwBufferSize )
{
    static FN_MakeAbsoluteSD2 *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MakeAbsoluteSD2", &g_Kernel32);
    return pfn( pSelfRelativeSecurityDescriptor, lpdwBufferSize );
}

typedef BOOL WINAPI FN_SetFileSecurityA( LPCSTR lpFileName, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFileSecurityA( LPCSTR lpFileName, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
    static FN_SetFileSecurityA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileSecurityA", &g_Kernel32);
    return pfn( lpFileName, SecurityInformation, pSecurityDescriptor );
}

typedef BOOL WINAPI FN_SetFileSecurityW( LPCWSTR lpFileName, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetFileSecurityW( LPCWSTR lpFileName, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
    static FN_SetFileSecurityW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetFileSecurityW", &g_Kernel32);
    return pfn( lpFileName, SecurityInformation, pSecurityDescriptor );
}

typedef BOOL WINAPI FN_GetFileSecurityA( LPCSTR lpFileName, SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetFileSecurityA( LPCSTR lpFileName, SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded )
{
    static FN_GetFileSecurityA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileSecurityA", &g_Kernel32);
    return pfn( lpFileName, RequestedInformation, pSecurityDescriptor, nLength, lpnLengthNeeded );
}

typedef BOOL WINAPI FN_GetFileSecurityW( LPCWSTR lpFileName, SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetFileSecurityW( LPCWSTR lpFileName, SECURITY_INFORMATION RequestedInformation, PSECURITY_DESCRIPTOR pSecurityDescriptor, DWORD nLength, LPDWORD lpnLengthNeeded )
{
    static FN_GetFileSecurityW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileSecurityW", &g_Kernel32);
    return pfn( lpFileName, RequestedInformation, pSecurityDescriptor, nLength, lpnLengthNeeded );
}

typedef BOOL WINAPI FN_SetKernelObjectSecurity( HANDLE Handle, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetKernelObjectSecurity( HANDLE Handle, SECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor )
{
    static FN_SetKernelObjectSecurity *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetKernelObjectSecurity", &g_Kernel32);
    return pfn( Handle, SecurityInformation, SecurityDescriptor );
}

typedef HANDLE WINAPI FN_FindFirstChangeNotificationA( LPCSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstChangeNotificationA( LPCSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter )
{
    static FN_FindFirstChangeNotificationA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstChangeNotificationA", &g_Kernel32);
    return pfn( lpPathName, bWatchSubtree, dwNotifyFilter );
}

typedef HANDLE WINAPI FN_FindFirstChangeNotificationW( LPCWSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstChangeNotificationW( LPCWSTR lpPathName, BOOL bWatchSubtree, DWORD dwNotifyFilter )
{
    static FN_FindFirstChangeNotificationW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstChangeNotificationW", &g_Kernel32);
    return pfn( lpPathName, bWatchSubtree, dwNotifyFilter );
}

typedef BOOL WINAPI FN_FindNextChangeNotification( HANDLE hChangeHandle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindNextChangeNotification( HANDLE hChangeHandle )
{
    static FN_FindNextChangeNotification *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindNextChangeNotification", &g_Kernel32);
    return pfn( hChangeHandle );
}

typedef BOOL WINAPI FN_FindCloseChangeNotification( HANDLE hChangeHandle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindCloseChangeNotification( HANDLE hChangeHandle )
{
    static FN_FindCloseChangeNotification *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindCloseChangeNotification", &g_Kernel32);
    return pfn( hChangeHandle );
}

typedef BOOL WINAPI FN_ReadDirectoryChangesW( HANDLE hDirectory, LPVOID lpBuffer, DWORD nBufferLength, BOOL bWatchSubtree, DWORD dwNotifyFilter, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadDirectoryChangesW( HANDLE hDirectory, LPVOID lpBuffer, DWORD nBufferLength, BOOL bWatchSubtree, DWORD dwNotifyFilter, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped, LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine )
{
    static FN_ReadDirectoryChangesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadDirectoryChangesW", &g_Kernel32);
    return pfn( hDirectory, lpBuffer, nBufferLength, bWatchSubtree, dwNotifyFilter, lpBytesReturned, lpOverlapped, lpCompletionRoutine );
}

typedef BOOL WINAPI FN_VirtualLock( LPVOID lpAddress, SIZE_T dwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_VirtualLock( LPVOID lpAddress, SIZE_T dwSize )
{
    static FN_VirtualLock *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualLock", &g_Kernel32);
    return pfn( lpAddress, dwSize );
}

typedef BOOL WINAPI FN_VirtualUnlock( LPVOID lpAddress, SIZE_T dwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_VirtualUnlock( LPVOID lpAddress, SIZE_T dwSize )
{
    static FN_VirtualUnlock *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VirtualUnlock", &g_Kernel32);
    return pfn( lpAddress, dwSize );
}

typedef LPVOID WINAPI FN_MapViewOfFileEx( HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap, LPVOID lpBaseAddress );
__declspec(dllexport) LPVOID WINAPI kPrf2Wrap_MapViewOfFileEx( HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap, LPVOID lpBaseAddress )
{
    static FN_MapViewOfFileEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MapViewOfFileEx", &g_Kernel32);
    return pfn( hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh, dwFileOffsetLow, dwNumberOfBytesToMap, lpBaseAddress );
}

typedef BOOL WINAPI FN_SetPriorityClass( HANDLE hProcess, DWORD dwPriorityClass );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetPriorityClass( HANDLE hProcess, DWORD dwPriorityClass )
{
    static FN_SetPriorityClass *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetPriorityClass", &g_Kernel32);
    return pfn( hProcess, dwPriorityClass );
}

typedef DWORD WINAPI FN_GetPriorityClass( HANDLE hProcess );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetPriorityClass( HANDLE hProcess )
{
    static FN_GetPriorityClass *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetPriorityClass", &g_Kernel32);
    return pfn( hProcess );
}

typedef BOOL WINAPI FN_IsBadReadPtr( CONST VOID * lp, UINT_PTR ucb );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsBadReadPtr( CONST VOID * lp, UINT_PTR ucb )
{
    static FN_IsBadReadPtr *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsBadReadPtr", &g_Kernel32);
    return pfn( lp, ucb );
}

typedef BOOL WINAPI FN_IsBadWritePtr( LPVOID lp, UINT_PTR ucb );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsBadWritePtr( LPVOID lp, UINT_PTR ucb )
{
    static FN_IsBadWritePtr *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsBadWritePtr", &g_Kernel32);
    return pfn( lp, ucb );
}

typedef BOOL WINAPI FN_IsBadHugeReadPtr( CONST VOID * lp, UINT_PTR ucb );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsBadHugeReadPtr( CONST VOID * lp, UINT_PTR ucb )
{
    static FN_IsBadHugeReadPtr *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsBadHugeReadPtr", &g_Kernel32);
    return pfn( lp, ucb );
}

typedef BOOL WINAPI FN_IsBadHugeWritePtr( LPVOID lp, UINT_PTR ucb );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsBadHugeWritePtr( LPVOID lp, UINT_PTR ucb )
{
    static FN_IsBadHugeWritePtr *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsBadHugeWritePtr", &g_Kernel32);
    return pfn( lp, ucb );
}

typedef BOOL WINAPI FN_IsBadCodePtr( FARPROC lpfn );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsBadCodePtr( FARPROC lpfn )
{
    static FN_IsBadCodePtr *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsBadCodePtr", &g_Kernel32);
    return pfn( lpfn );
}

typedef BOOL WINAPI FN_IsBadStringPtrA( LPCSTR lpsz, UINT_PTR ucchMax );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsBadStringPtrA( LPCSTR lpsz, UINT_PTR ucchMax )
{
    static FN_IsBadStringPtrA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsBadStringPtrA", &g_Kernel32);
    return pfn( lpsz, ucchMax );
}

typedef BOOL WINAPI FN_IsBadStringPtrW( LPCWSTR lpsz, UINT_PTR ucchMax );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsBadStringPtrW( LPCWSTR lpsz, UINT_PTR ucchMax )
{
    static FN_IsBadStringPtrW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsBadStringPtrW", &g_Kernel32);
    return pfn( lpsz, ucchMax );
}

typedef BOOL WINAPI FN_LookupAccountSidA( LPCSTR lpSystemName, PSID Sid, LPSTR Name, LPDWORD cchName, LPSTR ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupAccountSidA( LPCSTR lpSystemName, PSID Sid, LPSTR Name, LPDWORD cchName, LPSTR ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse )
{
    static FN_LookupAccountSidA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupAccountSidA", &g_Kernel32);
    return pfn( lpSystemName, Sid, Name, cchName, ReferencedDomainName, cchReferencedDomainName, peUse );
}

typedef BOOL WINAPI FN_LookupAccountSidW( LPCWSTR lpSystemName, PSID Sid, LPWSTR Name, LPDWORD cchName, LPWSTR ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupAccountSidW( LPCWSTR lpSystemName, PSID Sid, LPWSTR Name, LPDWORD cchName, LPWSTR ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse )
{
    static FN_LookupAccountSidW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupAccountSidW", &g_Kernel32);
    return pfn( lpSystemName, Sid, Name, cchName, ReferencedDomainName, cchReferencedDomainName, peUse );
}

typedef BOOL WINAPI FN_LookupAccountNameA( LPCSTR lpSystemName, LPCSTR lpAccountName, PSID Sid, LPDWORD cbSid, LPSTR ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupAccountNameA( LPCSTR lpSystemName, LPCSTR lpAccountName, PSID Sid, LPDWORD cbSid, LPSTR ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse )
{
    static FN_LookupAccountNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupAccountNameA", &g_Kernel32);
    return pfn( lpSystemName, lpAccountName, Sid, cbSid, ReferencedDomainName, cchReferencedDomainName, peUse );
}

typedef BOOL WINAPI FN_LookupAccountNameW( LPCWSTR lpSystemName, LPCWSTR lpAccountName, PSID Sid, LPDWORD cbSid, LPWSTR ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupAccountNameW( LPCWSTR lpSystemName, LPCWSTR lpAccountName, PSID Sid, LPDWORD cbSid, LPWSTR ReferencedDomainName, LPDWORD cchReferencedDomainName, PSID_NAME_USE peUse )
{
    static FN_LookupAccountNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupAccountNameW", &g_Kernel32);
    return pfn( lpSystemName, lpAccountName, Sid, cbSid, ReferencedDomainName, cchReferencedDomainName, peUse );
}

typedef BOOL WINAPI FN_LookupPrivilegeValueA( LPCSTR lpSystemName, LPCSTR lpName, PLUID lpLuid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupPrivilegeValueA( LPCSTR lpSystemName, LPCSTR lpName, PLUID lpLuid )
{
    static FN_LookupPrivilegeValueA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupPrivilegeValueA", &g_Kernel32);
    return pfn( lpSystemName, lpName, lpLuid );
}

typedef BOOL WINAPI FN_LookupPrivilegeValueW( LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupPrivilegeValueW( LPCWSTR lpSystemName, LPCWSTR lpName, PLUID lpLuid )
{
    static FN_LookupPrivilegeValueW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupPrivilegeValueW", &g_Kernel32);
    return pfn( lpSystemName, lpName, lpLuid );
}

typedef BOOL WINAPI FN_LookupPrivilegeNameA( LPCSTR lpSystemName, PLUID lpLuid, LPSTR lpName, LPDWORD cchName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupPrivilegeNameA( LPCSTR lpSystemName, PLUID lpLuid, LPSTR lpName, LPDWORD cchName )
{
    static FN_LookupPrivilegeNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupPrivilegeNameA", &g_Kernel32);
    return pfn( lpSystemName, lpLuid, lpName, cchName );
}

typedef BOOL WINAPI FN_LookupPrivilegeNameW( LPCWSTR lpSystemName, PLUID lpLuid, LPWSTR lpName, LPDWORD cchName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupPrivilegeNameW( LPCWSTR lpSystemName, PLUID lpLuid, LPWSTR lpName, LPDWORD cchName )
{
    static FN_LookupPrivilegeNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupPrivilegeNameW", &g_Kernel32);
    return pfn( lpSystemName, lpLuid, lpName, cchName );
}

typedef BOOL WINAPI FN_LookupPrivilegeDisplayNameA( LPCSTR lpSystemName, LPCSTR lpName, LPSTR lpDisplayName, LPDWORD cchDisplayName, LPDWORD lpLanguageId );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupPrivilegeDisplayNameA( LPCSTR lpSystemName, LPCSTR lpName, LPSTR lpDisplayName, LPDWORD cchDisplayName, LPDWORD lpLanguageId )
{
    static FN_LookupPrivilegeDisplayNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupPrivilegeDisplayNameA", &g_Kernel32);
    return pfn( lpSystemName, lpName, lpDisplayName, cchDisplayName, lpLanguageId );
}

typedef BOOL WINAPI FN_LookupPrivilegeDisplayNameW( LPCWSTR lpSystemName, LPCWSTR lpName, LPWSTR lpDisplayName, LPDWORD cchDisplayName, LPDWORD lpLanguageId );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LookupPrivilegeDisplayNameW( LPCWSTR lpSystemName, LPCWSTR lpName, LPWSTR lpDisplayName, LPDWORD cchDisplayName, LPDWORD lpLanguageId )
{
    static FN_LookupPrivilegeDisplayNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LookupPrivilegeDisplayNameW", &g_Kernel32);
    return pfn( lpSystemName, lpName, lpDisplayName, cchDisplayName, lpLanguageId );
}

typedef BOOL WINAPI FN_AllocateLocallyUniqueId( PLUID Luid );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AllocateLocallyUniqueId( PLUID Luid )
{
    static FN_AllocateLocallyUniqueId *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AllocateLocallyUniqueId", &g_Kernel32);
    return pfn( Luid );
}

typedef BOOL WINAPI FN_BuildCommDCBA( LPCSTR lpDef, LPDCB lpDCB );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BuildCommDCBA( LPCSTR lpDef, LPDCB lpDCB )
{
    static FN_BuildCommDCBA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BuildCommDCBA", &g_Kernel32);
    return pfn( lpDef, lpDCB );
}

typedef BOOL WINAPI FN_BuildCommDCBW( LPCWSTR lpDef, LPDCB lpDCB );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BuildCommDCBW( LPCWSTR lpDef, LPDCB lpDCB )
{
    static FN_BuildCommDCBW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BuildCommDCBW", &g_Kernel32);
    return pfn( lpDef, lpDCB );
}

typedef BOOL WINAPI FN_BuildCommDCBAndTimeoutsA( LPCSTR lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BuildCommDCBAndTimeoutsA( LPCSTR lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts )
{
    static FN_BuildCommDCBAndTimeoutsA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BuildCommDCBAndTimeoutsA", &g_Kernel32);
    return pfn( lpDef, lpDCB, lpCommTimeouts );
}

typedef BOOL WINAPI FN_BuildCommDCBAndTimeoutsW( LPCWSTR lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BuildCommDCBAndTimeoutsW( LPCWSTR lpDef, LPDCB lpDCB, LPCOMMTIMEOUTS lpCommTimeouts )
{
    static FN_BuildCommDCBAndTimeoutsW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BuildCommDCBAndTimeoutsW", &g_Kernel32);
    return pfn( lpDef, lpDCB, lpCommTimeouts );
}

typedef BOOL WINAPI FN_CommConfigDialogA( LPCSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CommConfigDialogA( LPCSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC )
{
    static FN_CommConfigDialogA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CommConfigDialogA", &g_Kernel32);
    return pfn( lpszName, hWnd, lpCC );
}

typedef BOOL WINAPI FN_CommConfigDialogW( LPCWSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CommConfigDialogW( LPCWSTR lpszName, HWND hWnd, LPCOMMCONFIG lpCC )
{
    static FN_CommConfigDialogW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CommConfigDialogW", &g_Kernel32);
    return pfn( lpszName, hWnd, lpCC );
}

typedef BOOL WINAPI FN_GetDefaultCommConfigA( LPCSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetDefaultCommConfigA( LPCSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize )
{
    static FN_GetDefaultCommConfigA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDefaultCommConfigA", &g_Kernel32);
    return pfn( lpszName, lpCC, lpdwSize );
}

typedef BOOL WINAPI FN_GetDefaultCommConfigW( LPCWSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetDefaultCommConfigW( LPCWSTR lpszName, LPCOMMCONFIG lpCC, LPDWORD lpdwSize )
{
    static FN_GetDefaultCommConfigW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDefaultCommConfigW", &g_Kernel32);
    return pfn( lpszName, lpCC, lpdwSize );
}

typedef BOOL WINAPI FN_SetDefaultCommConfigA( LPCSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetDefaultCommConfigA( LPCSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize )
{
    static FN_SetDefaultCommConfigA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetDefaultCommConfigA", &g_Kernel32);
    return pfn( lpszName, lpCC, dwSize );
}

typedef BOOL WINAPI FN_SetDefaultCommConfigW( LPCWSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetDefaultCommConfigW( LPCWSTR lpszName, LPCOMMCONFIG lpCC, DWORD dwSize )
{
    static FN_SetDefaultCommConfigW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetDefaultCommConfigW", &g_Kernel32);
    return pfn( lpszName, lpCC, dwSize );
}

typedef BOOL WINAPI FN_GetComputerNameA( LPSTR lpBuffer, LPDWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetComputerNameA( LPSTR lpBuffer, LPDWORD nSize )
{
    static FN_GetComputerNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetComputerNameA", &g_Kernel32);
    return pfn( lpBuffer, nSize );
}

typedef BOOL WINAPI FN_GetComputerNameW( LPWSTR lpBuffer, LPDWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetComputerNameW( LPWSTR lpBuffer, LPDWORD nSize )
{
    static FN_GetComputerNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetComputerNameW", &g_Kernel32);
    return pfn( lpBuffer, nSize );
}

typedef BOOL WINAPI FN_SetComputerNameA( LPCSTR lpComputerName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetComputerNameA( LPCSTR lpComputerName )
{
    static FN_SetComputerNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetComputerNameA", &g_Kernel32);
    return pfn( lpComputerName );
}

typedef BOOL WINAPI FN_SetComputerNameW( LPCWSTR lpComputerName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetComputerNameW( LPCWSTR lpComputerName )
{
    static FN_SetComputerNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetComputerNameW", &g_Kernel32);
    return pfn( lpComputerName );
}

typedef BOOL WINAPI FN_GetComputerNameExA( COMPUTER_NAME_FORMAT NameType, LPSTR lpBuffer, LPDWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetComputerNameExA( COMPUTER_NAME_FORMAT NameType, LPSTR lpBuffer, LPDWORD nSize )
{
    static FN_GetComputerNameExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetComputerNameExA", &g_Kernel32);
    return pfn( NameType, lpBuffer, nSize );
}

typedef BOOL WINAPI FN_GetComputerNameExW( COMPUTER_NAME_FORMAT NameType, LPWSTR lpBuffer, LPDWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetComputerNameExW( COMPUTER_NAME_FORMAT NameType, LPWSTR lpBuffer, LPDWORD nSize )
{
    static FN_GetComputerNameExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetComputerNameExW", &g_Kernel32);
    return pfn( NameType, lpBuffer, nSize );
}

typedef BOOL WINAPI FN_SetComputerNameExA( COMPUTER_NAME_FORMAT NameType, LPCSTR lpBuffer );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetComputerNameExA( COMPUTER_NAME_FORMAT NameType, LPCSTR lpBuffer )
{
    static FN_SetComputerNameExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetComputerNameExA", &g_Kernel32);
    return pfn( NameType, lpBuffer );
}

typedef BOOL WINAPI FN_SetComputerNameExW( COMPUTER_NAME_FORMAT NameType, LPCWSTR lpBuffer );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetComputerNameExW( COMPUTER_NAME_FORMAT NameType, LPCWSTR lpBuffer )
{
    static FN_SetComputerNameExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetComputerNameExW", &g_Kernel32);
    return pfn( NameType, lpBuffer );
}

typedef BOOL WINAPI FN_DnsHostnameToComputerNameA( LPCSTR Hostname, LPSTR ComputerName, LPDWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DnsHostnameToComputerNameA( LPCSTR Hostname, LPSTR ComputerName, LPDWORD nSize )
{
    static FN_DnsHostnameToComputerNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DnsHostnameToComputerNameA", &g_Kernel32);
    return pfn( Hostname, ComputerName, nSize );
}

typedef BOOL WINAPI FN_DnsHostnameToComputerNameW( LPCWSTR Hostname, LPWSTR ComputerName, LPDWORD nSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DnsHostnameToComputerNameW( LPCWSTR Hostname, LPWSTR ComputerName, LPDWORD nSize )
{
    static FN_DnsHostnameToComputerNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DnsHostnameToComputerNameW", &g_Kernel32);
    return pfn( Hostname, ComputerName, nSize );
}

typedef BOOL WINAPI FN_GetUserNameA( LPSTR lpBuffer, LPDWORD pcbBuffer );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetUserNameA( LPSTR lpBuffer, LPDWORD pcbBuffer )
{
    static FN_GetUserNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetUserNameA", &g_Kernel32);
    return pfn( lpBuffer, pcbBuffer );
}

typedef BOOL WINAPI FN_GetUserNameW( LPWSTR lpBuffer, LPDWORD pcbBuffer );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetUserNameW( LPWSTR lpBuffer, LPDWORD pcbBuffer )
{
    static FN_GetUserNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetUserNameW", &g_Kernel32);
    return pfn( lpBuffer, pcbBuffer );
}

typedef BOOL WINAPI FN_LogonUserA( LPCSTR lpszUsername, LPCSTR lpszDomain, LPCSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LogonUserA( LPCSTR lpszUsername, LPCSTR lpszDomain, LPCSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken )
{
    static FN_LogonUserA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LogonUserA", &g_Kernel32);
    return pfn( lpszUsername, lpszDomain, lpszPassword, dwLogonType, dwLogonProvider, phToken );
}

typedef BOOL WINAPI FN_LogonUserW( LPCWSTR lpszUsername, LPCWSTR lpszDomain, LPCWSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LogonUserW( LPCWSTR lpszUsername, LPCWSTR lpszDomain, LPCWSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken )
{
    static FN_LogonUserW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LogonUserW", &g_Kernel32);
    return pfn( lpszUsername, lpszDomain, lpszPassword, dwLogonType, dwLogonProvider, phToken );
}

typedef BOOL WINAPI FN_LogonUserExA( LPCSTR lpszUsername, LPCSTR lpszDomain, LPCSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken, PSID * ppLogonSid, PVOID * ppProfileBuffer, LPDWORD pdwProfileLength, PQUOTA_LIMITS pQuotaLimits );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LogonUserExA( LPCSTR lpszUsername, LPCSTR lpszDomain, LPCSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken, PSID * ppLogonSid, PVOID * ppProfileBuffer, LPDWORD pdwProfileLength, PQUOTA_LIMITS pQuotaLimits )
{
    static FN_LogonUserExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LogonUserExA", &g_Kernel32);
    return pfn( lpszUsername, lpszDomain, lpszPassword, dwLogonType, dwLogonProvider, phToken, ppLogonSid, ppProfileBuffer, pdwProfileLength, pQuotaLimits );
}

typedef BOOL WINAPI FN_LogonUserExW( LPCWSTR lpszUsername, LPCWSTR lpszDomain, LPCWSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken, PSID * ppLogonSid, PVOID * ppProfileBuffer, LPDWORD pdwProfileLength, PQUOTA_LIMITS pQuotaLimits );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_LogonUserExW( LPCWSTR lpszUsername, LPCWSTR lpszDomain, LPCWSTR lpszPassword, DWORD dwLogonType, DWORD dwLogonProvider, PHANDLE phToken, PSID * ppLogonSid, PVOID * ppProfileBuffer, LPDWORD pdwProfileLength, PQUOTA_LIMITS pQuotaLimits )
{
    static FN_LogonUserExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LogonUserExW", &g_Kernel32);
    return pfn( lpszUsername, lpszDomain, lpszPassword, dwLogonType, dwLogonProvider, phToken, ppLogonSid, ppProfileBuffer, pdwProfileLength, pQuotaLimits );
}

typedef BOOL WINAPI FN_ImpersonateLoggedOnUser( HANDLE hToken );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ImpersonateLoggedOnUser( HANDLE hToken )
{
    static FN_ImpersonateLoggedOnUser *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ImpersonateLoggedOnUser", &g_Kernel32);
    return pfn( hToken );
}

typedef BOOL WINAPI FN_CreateProcessAsUserA( HANDLE hToken, LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateProcessAsUserA( HANDLE hToken, LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation )
{
    static FN_CreateProcessAsUserA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateProcessAsUserA", &g_Kernel32);
    return pfn( hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation );
}

typedef BOOL WINAPI FN_CreateProcessAsUserW( HANDLE hToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateProcessAsUserW( HANDLE hToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation )
{
    static FN_CreateProcessAsUserW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateProcessAsUserW", &g_Kernel32);
    return pfn( hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation );
}

typedef BOOL WINAPI FN_CreateProcessWithLogonW( LPCWSTR lpUsername, LPCWSTR lpDomain, LPCWSTR lpPassword, DWORD dwLogonFlags, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateProcessWithLogonW( LPCWSTR lpUsername, LPCWSTR lpDomain, LPCWSTR lpPassword, DWORD dwLogonFlags, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation )
{
    static FN_CreateProcessWithLogonW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateProcessWithLogonW", &g_Kernel32);
    return pfn( lpUsername, lpDomain, lpPassword, dwLogonFlags, lpApplicationName, lpCommandLine, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation );
}

typedef BOOL WINAPI FN_CreateProcessWithTokenW( HANDLE hToken, DWORD dwLogonFlags, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateProcessWithTokenW( HANDLE hToken, DWORD dwLogonFlags, LPCWSTR lpApplicationName, LPWSTR lpCommandLine, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation )
{
    static FN_CreateProcessWithTokenW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateProcessWithTokenW", &g_Kernel32);
    return pfn( hToken, dwLogonFlags, lpApplicationName, lpCommandLine, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation );
}

typedef BOOL APIENTRY FN_ImpersonateAnonymousToken( HANDLE ThreadHandle );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_ImpersonateAnonymousToken( HANDLE ThreadHandle )
{
    static FN_ImpersonateAnonymousToken *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ImpersonateAnonymousToken", &g_Kernel32);
    return pfn( ThreadHandle );
}

typedef BOOL WINAPI FN_DuplicateTokenEx( HANDLE hExistingToken, DWORD dwDesiredAccess, LPSECURITY_ATTRIBUTES lpTokenAttributes, SECURITY_IMPERSONATION_LEVEL ImpersonationLevel, TOKEN_TYPE TokenType, PHANDLE phNewToken );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DuplicateTokenEx( HANDLE hExistingToken, DWORD dwDesiredAccess, LPSECURITY_ATTRIBUTES lpTokenAttributes, SECURITY_IMPERSONATION_LEVEL ImpersonationLevel, TOKEN_TYPE TokenType, PHANDLE phNewToken )
{
    static FN_DuplicateTokenEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DuplicateTokenEx", &g_Kernel32);
    return pfn( hExistingToken, dwDesiredAccess, lpTokenAttributes, ImpersonationLevel, TokenType, phNewToken );
}

typedef BOOL APIENTRY FN_CreateRestrictedToken( HANDLE ExistingTokenHandle, DWORD Flags, DWORD DisableSidCount, PSID_AND_ATTRIBUTES SidsToDisable, DWORD DeletePrivilegeCount, PLUID_AND_ATTRIBUTES PrivilegesToDelete, DWORD RestrictedSidCount, PSID_AND_ATTRIBUTES SidsToRestrict, PHANDLE NewTokenHandle );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_CreateRestrictedToken( HANDLE ExistingTokenHandle, DWORD Flags, DWORD DisableSidCount, PSID_AND_ATTRIBUTES SidsToDisable, DWORD DeletePrivilegeCount, PLUID_AND_ATTRIBUTES PrivilegesToDelete, DWORD RestrictedSidCount, PSID_AND_ATTRIBUTES SidsToRestrict, PHANDLE NewTokenHandle )
{
    static FN_CreateRestrictedToken *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateRestrictedToken", &g_Kernel32);
    return pfn( ExistingTokenHandle, Flags, DisableSidCount, SidsToDisable, DeletePrivilegeCount, PrivilegesToDelete, RestrictedSidCount, SidsToRestrict, NewTokenHandle );
}

typedef BOOL WINAPI FN_IsTokenRestricted( HANDLE TokenHandle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsTokenRestricted( HANDLE TokenHandle )
{
    static FN_IsTokenRestricted *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsTokenRestricted", &g_Kernel32);
    return pfn( TokenHandle );
}

typedef BOOL WINAPI FN_IsTokenUntrusted( HANDLE TokenHandle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsTokenUntrusted( HANDLE TokenHandle )
{
    static FN_IsTokenUntrusted *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsTokenUntrusted", &g_Kernel32);
    return pfn( TokenHandle );
}

typedef BOOL APIENTRY FN_CheckTokenMembership( HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_CheckTokenMembership( HANDLE TokenHandle, PSID SidToCheck, PBOOL IsMember )
{
    static FN_CheckTokenMembership *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CheckTokenMembership", &g_Kernel32);
    return pfn( TokenHandle, SidToCheck, IsMember );
}

typedef BOOL WINAPI FN_RegisterWaitForSingleObject( PHANDLE phNewWaitObject, HANDLE hObject, WAITORTIMERCALLBACK Callback, PVOID Context, ULONG dwMilliseconds, ULONG dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_RegisterWaitForSingleObject( PHANDLE phNewWaitObject, HANDLE hObject, WAITORTIMERCALLBACK Callback, PVOID Context, ULONG dwMilliseconds, ULONG dwFlags )
{
    static FN_RegisterWaitForSingleObject *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RegisterWaitForSingleObject", &g_Kernel32);
    return pfn( phNewWaitObject, hObject, Callback, Context, dwMilliseconds, dwFlags );
}

typedef HANDLE WINAPI FN_RegisterWaitForSingleObjectEx( HANDLE hObject, WAITORTIMERCALLBACK Callback, PVOID Context, ULONG dwMilliseconds, ULONG dwFlags );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_RegisterWaitForSingleObjectEx( HANDLE hObject, WAITORTIMERCALLBACK Callback, PVOID Context, ULONG dwMilliseconds, ULONG dwFlags )
{
    static FN_RegisterWaitForSingleObjectEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RegisterWaitForSingleObjectEx", &g_Kernel32);
    return pfn( hObject, Callback, Context, dwMilliseconds, dwFlags );
}

typedef BOOL WINAPI FN_UnregisterWait( HANDLE WaitHandle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_UnregisterWait( HANDLE WaitHandle )
{
    static FN_UnregisterWait *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "UnregisterWait", &g_Kernel32);
    return pfn( WaitHandle );
}

typedef BOOL WINAPI FN_UnregisterWaitEx( HANDLE WaitHandle, HANDLE CompletionEvent );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_UnregisterWaitEx( HANDLE WaitHandle, HANDLE CompletionEvent )
{
    static FN_UnregisterWaitEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "UnregisterWaitEx", &g_Kernel32);
    return pfn( WaitHandle, CompletionEvent );
}

typedef BOOL WINAPI FN_QueueUserWorkItem( LPTHREAD_START_ROUTINE Function, PVOID Context, ULONG Flags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_QueueUserWorkItem( LPTHREAD_START_ROUTINE Function, PVOID Context, ULONG Flags )
{
    static FN_QueueUserWorkItem *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueueUserWorkItem", &g_Kernel32);
    return pfn( Function, Context, Flags );
}

typedef BOOL WINAPI FN_BindIoCompletionCallback( HANDLE FileHandle, LPOVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_BindIoCompletionCallback( HANDLE FileHandle, LPOVERLAPPED_COMPLETION_ROUTINE Function, ULONG Flags )
{
    static FN_BindIoCompletionCallback *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "BindIoCompletionCallback", &g_Kernel32);
    return pfn( FileHandle, Function, Flags );
}

typedef HANDLE WINAPI FN_CreateTimerQueue( VOID );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateTimerQueue( VOID )
{
    static FN_CreateTimerQueue *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateTimerQueue", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_CreateTimerQueueTimer( PHANDLE phNewTimer, HANDLE TimerQueue, WAITORTIMERCALLBACK Callback, PVOID Parameter, DWORD DueTime, DWORD Period, ULONG Flags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateTimerQueueTimer( PHANDLE phNewTimer, HANDLE TimerQueue, WAITORTIMERCALLBACK Callback, PVOID Parameter, DWORD DueTime, DWORD Period, ULONG Flags )
{
    static FN_CreateTimerQueueTimer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateTimerQueueTimer", &g_Kernel32);
    return pfn( phNewTimer, TimerQueue, Callback, Parameter, DueTime, Period, Flags );
}

typedef BOOL WINAPI FN_ChangeTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer, ULONG DueTime, ULONG Period );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ChangeTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer, ULONG DueTime, ULONG Period )
{
    static FN_ChangeTimerQueueTimer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ChangeTimerQueueTimer", &g_Kernel32);
    return pfn( TimerQueue, Timer, DueTime, Period );
}

typedef BOOL WINAPI FN_DeleteTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer, HANDLE CompletionEvent );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeleteTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer, HANDLE CompletionEvent )
{
    static FN_DeleteTimerQueueTimer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteTimerQueueTimer", &g_Kernel32);
    return pfn( TimerQueue, Timer, CompletionEvent );
}

typedef BOOL WINAPI FN_DeleteTimerQueueEx( HANDLE TimerQueue, HANDLE CompletionEvent );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeleteTimerQueueEx( HANDLE TimerQueue, HANDLE CompletionEvent )
{
    static FN_DeleteTimerQueueEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteTimerQueueEx", &g_Kernel32);
    return pfn( TimerQueue, CompletionEvent );
}

typedef HANDLE WINAPI FN_SetTimerQueueTimer( HANDLE TimerQueue, WAITORTIMERCALLBACK Callback, PVOID Parameter, DWORD DueTime, DWORD Period, BOOL PreferIo );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_SetTimerQueueTimer( HANDLE TimerQueue, WAITORTIMERCALLBACK Callback, PVOID Parameter, DWORD DueTime, DWORD Period, BOOL PreferIo )
{
    static FN_SetTimerQueueTimer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetTimerQueueTimer", &g_Kernel32);
    return pfn( TimerQueue, Callback, Parameter, DueTime, Period, PreferIo );
}

typedef BOOL WINAPI FN_CancelTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CancelTimerQueueTimer( HANDLE TimerQueue, HANDLE Timer )
{
    static FN_CancelTimerQueueTimer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CancelTimerQueueTimer", &g_Kernel32);
    return pfn( TimerQueue, Timer );
}

typedef BOOL WINAPI FN_DeleteTimerQueue( HANDLE TimerQueue );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeleteTimerQueue( HANDLE TimerQueue )
{
    static FN_DeleteTimerQueue *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteTimerQueue", &g_Kernel32);
    return pfn( TimerQueue );
}

typedef BOOL WINAPI FN_GetCurrentHwProfileA( LPHW_PROFILE_INFOA lpHwProfileInfo );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCurrentHwProfileA( LPHW_PROFILE_INFOA lpHwProfileInfo )
{
    static FN_GetCurrentHwProfileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentHwProfileA", &g_Kernel32);
    return pfn( lpHwProfileInfo );
}

typedef BOOL WINAPI FN_GetCurrentHwProfileW( LPHW_PROFILE_INFOW lpHwProfileInfo );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCurrentHwProfileW( LPHW_PROFILE_INFOW lpHwProfileInfo )
{
    static FN_GetCurrentHwProfileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentHwProfileW", &g_Kernel32);
    return pfn( lpHwProfileInfo );
}

typedef BOOL WINAPI FN_QueryPerformanceCounter( LARGE_INTEGER * lpPerformanceCount );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_QueryPerformanceCounter( LARGE_INTEGER * lpPerformanceCount )
{
    static FN_QueryPerformanceCounter *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueryPerformanceCounter", &g_Kernel32);
    return pfn( lpPerformanceCount );
}

typedef BOOL WINAPI FN_QueryPerformanceFrequency( LARGE_INTEGER * lpFrequency );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_QueryPerformanceFrequency( LARGE_INTEGER * lpFrequency )
{
    static FN_QueryPerformanceFrequency *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueryPerformanceFrequency", &g_Kernel32);
    return pfn( lpFrequency );
}

typedef BOOL WINAPI FN_GetVersionExA( LPOSVERSIONINFOA lpVersionInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVersionExA( LPOSVERSIONINFOA lpVersionInformation )
{
    static FN_GetVersionExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVersionExA", &g_Kernel32);
    return pfn( lpVersionInformation );
}

typedef BOOL WINAPI FN_GetVersionExW( LPOSVERSIONINFOW lpVersionInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVersionExW( LPOSVERSIONINFOW lpVersionInformation )
{
    static FN_GetVersionExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVersionExW", &g_Kernel32);
    return pfn( lpVersionInformation );
}

typedef BOOL WINAPI FN_VerifyVersionInfoA( LPOSVERSIONINFOEXA lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_VerifyVersionInfoA( LPOSVERSIONINFOEXA lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask )
{
    static FN_VerifyVersionInfoA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerifyVersionInfoA", &g_Kernel32);
    return pfn( lpVersionInformation, dwTypeMask, dwlConditionMask );
}

typedef BOOL WINAPI FN_VerifyVersionInfoW( LPOSVERSIONINFOEXW lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_VerifyVersionInfoW( LPOSVERSIONINFOEXW lpVersionInformation, DWORD dwTypeMask, DWORDLONG dwlConditionMask )
{
    static FN_VerifyVersionInfoW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerifyVersionInfoW", &g_Kernel32);
    return pfn( lpVersionInformation, dwTypeMask, dwlConditionMask );
}

typedef BOOL WINAPI FN_GetSystemPowerStatus( LPSYSTEM_POWER_STATUS lpSystemPowerStatus );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetSystemPowerStatus( LPSYSTEM_POWER_STATUS lpSystemPowerStatus )
{
    static FN_GetSystemPowerStatus *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemPowerStatus", &g_Kernel32);
    return pfn( lpSystemPowerStatus );
}

typedef BOOL WINAPI FN_SetSystemPowerState( BOOL fSuspend, BOOL fForce );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetSystemPowerState( BOOL fSuspend, BOOL fForce )
{
    static FN_SetSystemPowerState *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetSystemPowerState", &g_Kernel32);
    return pfn( fSuspend, fForce );
}

typedef BOOL WINAPI FN_AllocateUserPhysicalPages( HANDLE hProcess, PULONG_PTR NumberOfPages, PULONG_PTR PageArray );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AllocateUserPhysicalPages( HANDLE hProcess, PULONG_PTR NumberOfPages, PULONG_PTR PageArray )
{
    static FN_AllocateUserPhysicalPages *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AllocateUserPhysicalPages", &g_Kernel32);
    return pfn( hProcess, NumberOfPages, PageArray );
}

typedef BOOL WINAPI FN_FreeUserPhysicalPages( HANDLE hProcess, PULONG_PTR NumberOfPages, PULONG_PTR PageArray );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FreeUserPhysicalPages( HANDLE hProcess, PULONG_PTR NumberOfPages, PULONG_PTR PageArray )
{
    static FN_FreeUserPhysicalPages *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FreeUserPhysicalPages", &g_Kernel32);
    return pfn( hProcess, NumberOfPages, PageArray );
}

typedef BOOL WINAPI FN_MapUserPhysicalPages( PVOID VirtualAddress, ULONG_PTR NumberOfPages, PULONG_PTR PageArray );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MapUserPhysicalPages( PVOID VirtualAddress, ULONG_PTR NumberOfPages, PULONG_PTR PageArray )
{
    static FN_MapUserPhysicalPages *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MapUserPhysicalPages", &g_Kernel32);
    return pfn( VirtualAddress, NumberOfPages, PageArray );
}

typedef BOOL WINAPI FN_MapUserPhysicalPagesScatter( PVOID * VirtualAddresses, ULONG_PTR NumberOfPages, PULONG_PTR PageArray );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_MapUserPhysicalPagesScatter( PVOID * VirtualAddresses, ULONG_PTR NumberOfPages, PULONG_PTR PageArray )
{
    static FN_MapUserPhysicalPagesScatter *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MapUserPhysicalPagesScatter", &g_Kernel32);
    return pfn( VirtualAddresses, NumberOfPages, PageArray );
}

typedef HANDLE WINAPI FN_CreateJobObjectA( LPSECURITY_ATTRIBUTES lpJobAttributes, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateJobObjectA( LPSECURITY_ATTRIBUTES lpJobAttributes, LPCSTR lpName )
{
    static FN_CreateJobObjectA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateJobObjectA", &g_Kernel32);
    return pfn( lpJobAttributes, lpName );
}

typedef HANDLE WINAPI FN_CreateJobObjectW( LPSECURITY_ATTRIBUTES lpJobAttributes, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateJobObjectW( LPSECURITY_ATTRIBUTES lpJobAttributes, LPCWSTR lpName )
{
    static FN_CreateJobObjectW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateJobObjectW", &g_Kernel32);
    return pfn( lpJobAttributes, lpName );
}

typedef HANDLE WINAPI FN_OpenJobObjectA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenJobObjectA( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName )
{
    static FN_OpenJobObjectA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenJobObjectA", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef HANDLE WINAPI FN_OpenJobObjectW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_OpenJobObjectW( DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName )
{
    static FN_OpenJobObjectW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "OpenJobObjectW", &g_Kernel32);
    return pfn( dwDesiredAccess, bInheritHandle, lpName );
}

typedef BOOL WINAPI FN_AssignProcessToJobObject( HANDLE hJob, HANDLE hProcess );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AssignProcessToJobObject( HANDLE hJob, HANDLE hProcess )
{
    static FN_AssignProcessToJobObject *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AssignProcessToJobObject", &g_Kernel32);
    return pfn( hJob, hProcess );
}

typedef BOOL WINAPI FN_TerminateJobObject( HANDLE hJob, UINT uExitCode );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_TerminateJobObject( HANDLE hJob, UINT uExitCode )
{
    static FN_TerminateJobObject *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "TerminateJobObject", &g_Kernel32);
    return pfn( hJob, uExitCode );
}

typedef BOOL WINAPI FN_QueryInformationJobObject( HANDLE hJob, JOBOBJECTINFOCLASS JobObjectInformationClass, LPVOID lpJobObjectInformation, DWORD cbJobObjectInformationLength, LPDWORD lpReturnLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_QueryInformationJobObject( HANDLE hJob, JOBOBJECTINFOCLASS JobObjectInformationClass, LPVOID lpJobObjectInformation, DWORD cbJobObjectInformationLength, LPDWORD lpReturnLength )
{
    static FN_QueryInformationJobObject *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueryInformationJobObject", &g_Kernel32);
    return pfn( hJob, JobObjectInformationClass, lpJobObjectInformation, cbJobObjectInformationLength, lpReturnLength );
}

typedef BOOL WINAPI FN_SetInformationJobObject( HANDLE hJob, JOBOBJECTINFOCLASS JobObjectInformationClass, LPVOID lpJobObjectInformation, DWORD cbJobObjectInformationLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetInformationJobObject( HANDLE hJob, JOBOBJECTINFOCLASS JobObjectInformationClass, LPVOID lpJobObjectInformation, DWORD cbJobObjectInformationLength )
{
    static FN_SetInformationJobObject *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetInformationJobObject", &g_Kernel32);
    return pfn( hJob, JobObjectInformationClass, lpJobObjectInformation, cbJobObjectInformationLength );
}

typedef BOOL WINAPI FN_IsProcessInJob( HANDLE ProcessHandle, HANDLE JobHandle, PBOOL Result );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsProcessInJob( HANDLE ProcessHandle, HANDLE JobHandle, PBOOL Result )
{
    static FN_IsProcessInJob *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsProcessInJob", &g_Kernel32);
    return pfn( ProcessHandle, JobHandle, Result );
}

typedef BOOL WINAPI FN_CreateJobSet( ULONG NumJob, PJOB_SET_ARRAY UserJobSet, ULONG Flags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_CreateJobSet( ULONG NumJob, PJOB_SET_ARRAY UserJobSet, ULONG Flags )
{
    static FN_CreateJobSet *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateJobSet", &g_Kernel32);
    return pfn( NumJob, UserJobSet, Flags );
}

typedef PVOID WINAPI FN_AddVectoredExceptionHandler( ULONG First, PVECTORED_EXCEPTION_HANDLER Handler );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_AddVectoredExceptionHandler( ULONG First, PVECTORED_EXCEPTION_HANDLER Handler )
{
    static FN_AddVectoredExceptionHandler *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddVectoredExceptionHandler", &g_Kernel32);
    return pfn( First, Handler );
}

typedef ULONG WINAPI FN_RemoveVectoredExceptionHandler( PVOID Handle );
__declspec(dllexport) ULONG WINAPI kPrf2Wrap_RemoveVectoredExceptionHandler( PVOID Handle )
{
    static FN_RemoveVectoredExceptionHandler *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RemoveVectoredExceptionHandler", &g_Kernel32);
    return pfn( Handle );
}

typedef PVOID WINAPI FN_AddVectoredContinueHandler( ULONG First, PVECTORED_EXCEPTION_HANDLER Handler );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_AddVectoredContinueHandler( ULONG First, PVECTORED_EXCEPTION_HANDLER Handler )
{
    static FN_AddVectoredContinueHandler *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddVectoredContinueHandler", &g_Kernel32);
    return pfn( First, Handler );
}

typedef ULONG WINAPI FN_RemoveVectoredContinueHandler( PVOID Handle );
__declspec(dllexport) ULONG WINAPI kPrf2Wrap_RemoveVectoredContinueHandler( PVOID Handle )
{
    static FN_RemoveVectoredContinueHandler *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RemoveVectoredContinueHandler", &g_Kernel32);
    return pfn( Handle );
}

typedef HANDLE WINAPI FN_FindFirstVolumeA( LPSTR lpszVolumeName, DWORD cchBufferLength );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstVolumeA( LPSTR lpszVolumeName, DWORD cchBufferLength )
{
    static FN_FindFirstVolumeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstVolumeA", &g_Kernel32);
    return pfn( lpszVolumeName, cchBufferLength );
}

typedef HANDLE WINAPI FN_FindFirstVolumeW( LPWSTR lpszVolumeName, DWORD cchBufferLength );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstVolumeW( LPWSTR lpszVolumeName, DWORD cchBufferLength )
{
    static FN_FindFirstVolumeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstVolumeW", &g_Kernel32);
    return pfn( lpszVolumeName, cchBufferLength );
}

typedef BOOL WINAPI FN_FindNextVolumeA( HANDLE hFindVolume, LPSTR lpszVolumeName, DWORD cchBufferLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindNextVolumeA( HANDLE hFindVolume, LPSTR lpszVolumeName, DWORD cchBufferLength )
{
    static FN_FindNextVolumeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindNextVolumeA", &g_Kernel32);
    return pfn( hFindVolume, lpszVolumeName, cchBufferLength );
}

typedef BOOL WINAPI FN_FindNextVolumeW( HANDLE hFindVolume, LPWSTR lpszVolumeName, DWORD cchBufferLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindNextVolumeW( HANDLE hFindVolume, LPWSTR lpszVolumeName, DWORD cchBufferLength )
{
    static FN_FindNextVolumeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindNextVolumeW", &g_Kernel32);
    return pfn( hFindVolume, lpszVolumeName, cchBufferLength );
}

typedef BOOL WINAPI FN_FindVolumeClose( HANDLE hFindVolume );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindVolumeClose( HANDLE hFindVolume )
{
    static FN_FindVolumeClose *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindVolumeClose", &g_Kernel32);
    return pfn( hFindVolume );
}

typedef HANDLE WINAPI FN_FindFirstVolumeMountPointA( LPCSTR lpszRootPathName, LPSTR lpszVolumeMountPoint, DWORD cchBufferLength );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstVolumeMountPointA( LPCSTR lpszRootPathName, LPSTR lpszVolumeMountPoint, DWORD cchBufferLength )
{
    static FN_FindFirstVolumeMountPointA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstVolumeMountPointA", &g_Kernel32);
    return pfn( lpszRootPathName, lpszVolumeMountPoint, cchBufferLength );
}

typedef HANDLE WINAPI FN_FindFirstVolumeMountPointW( LPCWSTR lpszRootPathName, LPWSTR lpszVolumeMountPoint, DWORD cchBufferLength );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_FindFirstVolumeMountPointW( LPCWSTR lpszRootPathName, LPWSTR lpszVolumeMountPoint, DWORD cchBufferLength )
{
    static FN_FindFirstVolumeMountPointW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindFirstVolumeMountPointW", &g_Kernel32);
    return pfn( lpszRootPathName, lpszVolumeMountPoint, cchBufferLength );
}

typedef BOOL WINAPI FN_FindNextVolumeMountPointA( HANDLE hFindVolumeMountPoint, LPSTR lpszVolumeMountPoint, DWORD cchBufferLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindNextVolumeMountPointA( HANDLE hFindVolumeMountPoint, LPSTR lpszVolumeMountPoint, DWORD cchBufferLength )
{
    static FN_FindNextVolumeMountPointA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindNextVolumeMountPointA", &g_Kernel32);
    return pfn( hFindVolumeMountPoint, lpszVolumeMountPoint, cchBufferLength );
}

typedef BOOL WINAPI FN_FindNextVolumeMountPointW( HANDLE hFindVolumeMountPoint, LPWSTR lpszVolumeMountPoint, DWORD cchBufferLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindNextVolumeMountPointW( HANDLE hFindVolumeMountPoint, LPWSTR lpszVolumeMountPoint, DWORD cchBufferLength )
{
    static FN_FindNextVolumeMountPointW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindNextVolumeMountPointW", &g_Kernel32);
    return pfn( hFindVolumeMountPoint, lpszVolumeMountPoint, cchBufferLength );
}

typedef BOOL WINAPI FN_FindVolumeMountPointClose( HANDLE hFindVolumeMountPoint );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindVolumeMountPointClose( HANDLE hFindVolumeMountPoint )
{
    static FN_FindVolumeMountPointClose *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindVolumeMountPointClose", &g_Kernel32);
    return pfn( hFindVolumeMountPoint );
}

typedef BOOL WINAPI FN_SetVolumeMountPointA( LPCSTR lpszVolumeMountPoint, LPCSTR lpszVolumeName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetVolumeMountPointA( LPCSTR lpszVolumeMountPoint, LPCSTR lpszVolumeName )
{
    static FN_SetVolumeMountPointA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetVolumeMountPointA", &g_Kernel32);
    return pfn( lpszVolumeMountPoint, lpszVolumeName );
}

typedef BOOL WINAPI FN_SetVolumeMountPointW( LPCWSTR lpszVolumeMountPoint, LPCWSTR lpszVolumeName );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetVolumeMountPointW( LPCWSTR lpszVolumeMountPoint, LPCWSTR lpszVolumeName )
{
    static FN_SetVolumeMountPointW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetVolumeMountPointW", &g_Kernel32);
    return pfn( lpszVolumeMountPoint, lpszVolumeName );
}

typedef BOOL WINAPI FN_DeleteVolumeMountPointA( LPCSTR lpszVolumeMountPoint );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeleteVolumeMountPointA( LPCSTR lpszVolumeMountPoint )
{
    static FN_DeleteVolumeMountPointA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteVolumeMountPointA", &g_Kernel32);
    return pfn( lpszVolumeMountPoint );
}

typedef BOOL WINAPI FN_DeleteVolumeMountPointW( LPCWSTR lpszVolumeMountPoint );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeleteVolumeMountPointW( LPCWSTR lpszVolumeMountPoint )
{
    static FN_DeleteVolumeMountPointW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeleteVolumeMountPointW", &g_Kernel32);
    return pfn( lpszVolumeMountPoint );
}

typedef BOOL WINAPI FN_GetVolumeNameForVolumeMountPointA( LPCSTR lpszVolumeMountPoint, LPSTR lpszVolumeName, DWORD cchBufferLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVolumeNameForVolumeMountPointA( LPCSTR lpszVolumeMountPoint, LPSTR lpszVolumeName, DWORD cchBufferLength )
{
    static FN_GetVolumeNameForVolumeMountPointA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVolumeNameForVolumeMountPointA", &g_Kernel32);
    return pfn( lpszVolumeMountPoint, lpszVolumeName, cchBufferLength );
}

typedef BOOL WINAPI FN_GetVolumeNameForVolumeMountPointW( LPCWSTR lpszVolumeMountPoint, LPWSTR lpszVolumeName, DWORD cchBufferLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVolumeNameForVolumeMountPointW( LPCWSTR lpszVolumeMountPoint, LPWSTR lpszVolumeName, DWORD cchBufferLength )
{
    static FN_GetVolumeNameForVolumeMountPointW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVolumeNameForVolumeMountPointW", &g_Kernel32);
    return pfn( lpszVolumeMountPoint, lpszVolumeName, cchBufferLength );
}

typedef BOOL WINAPI FN_GetVolumePathNameA( LPCSTR lpszFileName, LPSTR lpszVolumePathName, DWORD cchBufferLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVolumePathNameA( LPCSTR lpszFileName, LPSTR lpszVolumePathName, DWORD cchBufferLength )
{
    static FN_GetVolumePathNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVolumePathNameA", &g_Kernel32);
    return pfn( lpszFileName, lpszVolumePathName, cchBufferLength );
}

typedef BOOL WINAPI FN_GetVolumePathNameW( LPCWSTR lpszFileName, LPWSTR lpszVolumePathName, DWORD cchBufferLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVolumePathNameW( LPCWSTR lpszFileName, LPWSTR lpszVolumePathName, DWORD cchBufferLength )
{
    static FN_GetVolumePathNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVolumePathNameW", &g_Kernel32);
    return pfn( lpszFileName, lpszVolumePathName, cchBufferLength );
}

typedef BOOL WINAPI FN_GetVolumePathNamesForVolumeNameA( LPCSTR lpszVolumeName, LPCH lpszVolumePathNames, DWORD cchBufferLength, PDWORD lpcchReturnLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVolumePathNamesForVolumeNameA( LPCSTR lpszVolumeName, LPCH lpszVolumePathNames, DWORD cchBufferLength, PDWORD lpcchReturnLength )
{
    static FN_GetVolumePathNamesForVolumeNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVolumePathNamesForVolumeNameA", &g_Kernel32);
    return pfn( lpszVolumeName, lpszVolumePathNames, cchBufferLength, lpcchReturnLength );
}

typedef BOOL WINAPI FN_GetVolumePathNamesForVolumeNameW( LPCWSTR lpszVolumeName, LPWCH lpszVolumePathNames, DWORD cchBufferLength, PDWORD lpcchReturnLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetVolumePathNamesForVolumeNameW( LPCWSTR lpszVolumeName, LPWCH lpszVolumePathNames, DWORD cchBufferLength, PDWORD lpcchReturnLength )
{
    static FN_GetVolumePathNamesForVolumeNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetVolumePathNamesForVolumeNameW", &g_Kernel32);
    return pfn( lpszVolumeName, lpszVolumePathNames, cchBufferLength, lpcchReturnLength );
}

typedef HANDLE WINAPI FN_CreateActCtxA( PCACTCTXA pActCtx );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateActCtxA( PCACTCTXA pActCtx )
{
    static FN_CreateActCtxA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateActCtxA", &g_Kernel32);
    return pfn( pActCtx );
}

typedef HANDLE WINAPI FN_CreateActCtxW( PCACTCTXW pActCtx );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateActCtxW( PCACTCTXW pActCtx )
{
    static FN_CreateActCtxW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateActCtxW", &g_Kernel32);
    return pfn( pActCtx );
}

typedef VOID WINAPI FN_AddRefActCtx( HANDLE hActCtx );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_AddRefActCtx( HANDLE hActCtx )
{
    static FN_AddRefActCtx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddRefActCtx", &g_Kernel32);
    pfn( hActCtx );
}

typedef VOID WINAPI FN_ReleaseActCtx( HANDLE hActCtx );
__declspec(dllexport) VOID WINAPI kPrf2Wrap_ReleaseActCtx( HANDLE hActCtx )
{
    static FN_ReleaseActCtx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReleaseActCtx", &g_Kernel32);
    pfn( hActCtx );
}

typedef BOOL WINAPI FN_ZombifyActCtx( HANDLE hActCtx );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ZombifyActCtx( HANDLE hActCtx )
{
    static FN_ZombifyActCtx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ZombifyActCtx", &g_Kernel32);
    return pfn( hActCtx );
}

typedef BOOL WINAPI FN_ActivateActCtx( HANDLE hActCtx, ULONG_PTR * lpCookie );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ActivateActCtx( HANDLE hActCtx, ULONG_PTR * lpCookie )
{
    static FN_ActivateActCtx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ActivateActCtx", &g_Kernel32);
    return pfn( hActCtx, lpCookie );
}

typedef BOOL WINAPI FN_DeactivateActCtx( DWORD dwFlags, ULONG_PTR ulCookie );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_DeactivateActCtx( DWORD dwFlags, ULONG_PTR ulCookie )
{
    static FN_DeactivateActCtx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "DeactivateActCtx", &g_Kernel32);
    return pfn( dwFlags, ulCookie );
}

typedef BOOL WINAPI FN_GetCurrentActCtx( HANDLE * lphActCtx );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCurrentActCtx( HANDLE * lphActCtx )
{
    static FN_GetCurrentActCtx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentActCtx", &g_Kernel32);
    return pfn( lphActCtx );
}

typedef BOOL WINAPI FN_FindActCtxSectionStringA( DWORD dwFlags, const GUID * lpExtensionGuid, ULONG ulSectionId, LPCSTR lpStringToFind, PACTCTX_SECTION_KEYED_DATA ReturnedData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindActCtxSectionStringA( DWORD dwFlags, const GUID * lpExtensionGuid, ULONG ulSectionId, LPCSTR lpStringToFind, PACTCTX_SECTION_KEYED_DATA ReturnedData )
{
    static FN_FindActCtxSectionStringA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindActCtxSectionStringA", &g_Kernel32);
    return pfn( dwFlags, lpExtensionGuid, ulSectionId, lpStringToFind, ReturnedData );
}

typedef BOOL WINAPI FN_FindActCtxSectionStringW( DWORD dwFlags, const GUID * lpExtensionGuid, ULONG ulSectionId, LPCWSTR lpStringToFind, PACTCTX_SECTION_KEYED_DATA ReturnedData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindActCtxSectionStringW( DWORD dwFlags, const GUID * lpExtensionGuid, ULONG ulSectionId, LPCWSTR lpStringToFind, PACTCTX_SECTION_KEYED_DATA ReturnedData )
{
    static FN_FindActCtxSectionStringW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindActCtxSectionStringW", &g_Kernel32);
    return pfn( dwFlags, lpExtensionGuid, ulSectionId, lpStringToFind, ReturnedData );
}

typedef BOOL WINAPI FN_FindActCtxSectionGuid( DWORD dwFlags, const GUID * lpExtensionGuid, ULONG ulSectionId, const GUID * lpGuidToFind, PACTCTX_SECTION_KEYED_DATA ReturnedData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FindActCtxSectionGuid( DWORD dwFlags, const GUID * lpExtensionGuid, ULONG ulSectionId, const GUID * lpGuidToFind, PACTCTX_SECTION_KEYED_DATA ReturnedData )
{
    static FN_FindActCtxSectionGuid *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FindActCtxSectionGuid", &g_Kernel32);
    return pfn( dwFlags, lpExtensionGuid, ulSectionId, lpGuidToFind, ReturnedData );
}

typedef BOOL WINAPI FN_QueryActCtxW( DWORD dwFlags, HANDLE hActCtx, PVOID pvSubInstance, ULONG ulInfoClass, PVOID pvBuffer, SIZE_T cbBuffer, SIZE_T * pcbWrittenOrRequired );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_QueryActCtxW( DWORD dwFlags, HANDLE hActCtx, PVOID pvSubInstance, ULONG ulInfoClass, PVOID pvBuffer, SIZE_T cbBuffer, SIZE_T * pcbWrittenOrRequired )
{
    static FN_QueryActCtxW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "QueryActCtxW", &g_Kernel32);
    return pfn( dwFlags, hActCtx, pvSubInstance, ulInfoClass, pvBuffer, cbBuffer, pcbWrittenOrRequired );
}

typedef BOOL WINAPI FN_ProcessIdToSessionId( DWORD dwProcessId, DWORD * pSessionId );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ProcessIdToSessionId( DWORD dwProcessId, DWORD * pSessionId )
{
    static FN_ProcessIdToSessionId *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ProcessIdToSessionId", &g_Kernel32);
    return pfn( dwProcessId, pSessionId );
}

typedef DWORD WINAPI FN_WTSGetActiveConsoleSessionId( );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_WTSGetActiveConsoleSessionId( )
{
    static FN_WTSGetActiveConsoleSessionId *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WTSGetActiveConsoleSessionId", &g_Kernel32);
    return pfn( );
}

typedef BOOL WINAPI FN_IsWow64Process( HANDLE hProcess, PBOOL Wow64Process );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsWow64Process( HANDLE hProcess, PBOOL Wow64Process )
{
    static FN_IsWow64Process *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsWow64Process", &g_Kernel32);
    return pfn( hProcess, Wow64Process );
}

typedef BOOL WINAPI FN_GetLogicalProcessorInformation( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetLogicalProcessorInformation( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer, PDWORD ReturnedLength )
{
    static FN_GetLogicalProcessorInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLogicalProcessorInformation", &g_Kernel32);
    return pfn( Buffer, ReturnedLength );
}

typedef BOOL WINAPI FN_GetNumaHighestNodeNumber( PULONG HighestNodeNumber );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNumaHighestNodeNumber( PULONG HighestNodeNumber )
{
    static FN_GetNumaHighestNodeNumber *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNumaHighestNodeNumber", &g_Kernel32);
    return pfn( HighestNodeNumber );
}

typedef BOOL WINAPI FN_GetNumaProcessorNode( UCHAR Processor, PUCHAR NodeNumber );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNumaProcessorNode( UCHAR Processor, PUCHAR NodeNumber )
{
    static FN_GetNumaProcessorNode *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNumaProcessorNode", &g_Kernel32);
    return pfn( Processor, NodeNumber );
}

typedef BOOL WINAPI FN_GetNumaNodeProcessorMask( UCHAR Node, PULONGLONG ProcessorMask );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNumaNodeProcessorMask( UCHAR Node, PULONGLONG ProcessorMask )
{
    static FN_GetNumaNodeProcessorMask *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNumaNodeProcessorMask", &g_Kernel32);
    return pfn( Node, ProcessorMask );
}

typedef BOOL WINAPI FN_GetNumaAvailableMemoryNode( UCHAR Node, PULONGLONG AvailableBytes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNumaAvailableMemoryNode( UCHAR Node, PULONGLONG AvailableBytes )
{
    static FN_GetNumaAvailableMemoryNode *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNumaAvailableMemoryNode", &g_Kernel32);
    return pfn( Node, AvailableBytes );
}

typedef BOOL WINAPI FN_PeekConsoleInputA( IN HANDLE hConsoleInput, OUT PINPUT_RECORD lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsRead );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_PeekConsoleInputA( IN HANDLE hConsoleInput, OUT PINPUT_RECORD lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsRead )
{
    static FN_PeekConsoleInputA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PeekConsoleInputA", &g_Kernel32);
    return pfn( hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead );
}

typedef BOOL WINAPI FN_PeekConsoleInputW( IN HANDLE hConsoleInput, OUT PINPUT_RECORD lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsRead );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_PeekConsoleInputW( IN HANDLE hConsoleInput, OUT PINPUT_RECORD lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsRead )
{
    static FN_PeekConsoleInputW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "PeekConsoleInputW", &g_Kernel32);
    return pfn( hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead );
}

typedef BOOL WINAPI FN_ReadConsoleInputA( IN HANDLE hConsoleInput, OUT PINPUT_RECORD lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsRead );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadConsoleInputA( IN HANDLE hConsoleInput, OUT PINPUT_RECORD lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsRead )
{
    static FN_ReadConsoleInputA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadConsoleInputA", &g_Kernel32);
    return pfn( hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead );
}

typedef BOOL WINAPI FN_ReadConsoleInputW( IN HANDLE hConsoleInput, OUT PINPUT_RECORD lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsRead );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadConsoleInputW( IN HANDLE hConsoleInput, OUT PINPUT_RECORD lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsRead )
{
    static FN_ReadConsoleInputW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadConsoleInputW", &g_Kernel32);
    return pfn( hConsoleInput, lpBuffer, nLength, lpNumberOfEventsRead );
}

typedef BOOL WINAPI FN_WriteConsoleInputA( IN HANDLE hConsoleInput, IN CONST INPUT_RECORD * lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsWritten );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteConsoleInputA( IN HANDLE hConsoleInput, IN CONST INPUT_RECORD * lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsWritten )
{
    static FN_WriteConsoleInputA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteConsoleInputA", &g_Kernel32);
    return pfn( hConsoleInput, lpBuffer, nLength, lpNumberOfEventsWritten );
}

typedef BOOL WINAPI FN_WriteConsoleInputW( IN HANDLE hConsoleInput, IN CONST INPUT_RECORD * lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsWritten );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteConsoleInputW( IN HANDLE hConsoleInput, IN CONST INPUT_RECORD * lpBuffer, IN DWORD nLength, OUT LPDWORD lpNumberOfEventsWritten )
{
    static FN_WriteConsoleInputW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteConsoleInputW", &g_Kernel32);
    return pfn( hConsoleInput, lpBuffer, nLength, lpNumberOfEventsWritten );
}

typedef BOOL WINAPI FN_ReadConsoleOutputA( IN HANDLE hConsoleOutput, OUT PCHAR_INFO lpBuffer, IN COORD dwBufferSize, IN COORD dwBufferCoord, IN OUT PSMALL_RECT lpReadRegion );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadConsoleOutputA( IN HANDLE hConsoleOutput, OUT PCHAR_INFO lpBuffer, IN COORD dwBufferSize, IN COORD dwBufferCoord, IN OUT PSMALL_RECT lpReadRegion )
{
    static FN_ReadConsoleOutputA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadConsoleOutputA", &g_Kernel32);
    return pfn( hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpReadRegion );
}

typedef BOOL WINAPI FN_ReadConsoleOutputW( IN HANDLE hConsoleOutput, OUT PCHAR_INFO lpBuffer, IN COORD dwBufferSize, IN COORD dwBufferCoord, IN OUT PSMALL_RECT lpReadRegion );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadConsoleOutputW( IN HANDLE hConsoleOutput, OUT PCHAR_INFO lpBuffer, IN COORD dwBufferSize, IN COORD dwBufferCoord, IN OUT PSMALL_RECT lpReadRegion )
{
    static FN_ReadConsoleOutputW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadConsoleOutputW", &g_Kernel32);
    return pfn( hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpReadRegion );
}

typedef BOOL WINAPI FN_WriteConsoleOutputA( IN HANDLE hConsoleOutput, IN CONST CHAR_INFO * lpBuffer, IN COORD dwBufferSize, IN COORD dwBufferCoord, IN OUT PSMALL_RECT lpWriteRegion );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteConsoleOutputA( IN HANDLE hConsoleOutput, IN CONST CHAR_INFO * lpBuffer, IN COORD dwBufferSize, IN COORD dwBufferCoord, IN OUT PSMALL_RECT lpWriteRegion )
{
    static FN_WriteConsoleOutputA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteConsoleOutputA", &g_Kernel32);
    return pfn( hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion );
}

typedef BOOL WINAPI FN_WriteConsoleOutputW( IN HANDLE hConsoleOutput, IN CONST CHAR_INFO * lpBuffer, IN COORD dwBufferSize, IN COORD dwBufferCoord, IN OUT PSMALL_RECT lpWriteRegion );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteConsoleOutputW( IN HANDLE hConsoleOutput, IN CONST CHAR_INFO * lpBuffer, IN COORD dwBufferSize, IN COORD dwBufferCoord, IN OUT PSMALL_RECT lpWriteRegion )
{
    static FN_WriteConsoleOutputW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteConsoleOutputW", &g_Kernel32);
    return pfn( hConsoleOutput, lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion );
}

typedef BOOL WINAPI FN_ReadConsoleOutputCharacterA( IN HANDLE hConsoleOutput, OUT LPSTR lpCharacter, IN DWORD nLength, IN COORD dwReadCoord, OUT LPDWORD lpNumberOfCharsRead );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadConsoleOutputCharacterA( IN HANDLE hConsoleOutput, OUT LPSTR lpCharacter, IN DWORD nLength, IN COORD dwReadCoord, OUT LPDWORD lpNumberOfCharsRead )
{
    static FN_ReadConsoleOutputCharacterA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadConsoleOutputCharacterA", &g_Kernel32);
    return pfn( hConsoleOutput, lpCharacter, nLength, dwReadCoord, lpNumberOfCharsRead );
}

typedef BOOL WINAPI FN_ReadConsoleOutputCharacterW( IN HANDLE hConsoleOutput, OUT LPWSTR lpCharacter, IN DWORD nLength, IN COORD dwReadCoord, OUT LPDWORD lpNumberOfCharsRead );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadConsoleOutputCharacterW( IN HANDLE hConsoleOutput, OUT LPWSTR lpCharacter, IN DWORD nLength, IN COORD dwReadCoord, OUT LPDWORD lpNumberOfCharsRead )
{
    static FN_ReadConsoleOutputCharacterW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadConsoleOutputCharacterW", &g_Kernel32);
    return pfn( hConsoleOutput, lpCharacter, nLength, dwReadCoord, lpNumberOfCharsRead );
}

typedef BOOL WINAPI FN_ReadConsoleOutputAttribute( IN HANDLE hConsoleOutput, OUT LPWORD lpAttribute, IN DWORD nLength, IN COORD dwReadCoord, OUT LPDWORD lpNumberOfAttrsRead );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadConsoleOutputAttribute( IN HANDLE hConsoleOutput, OUT LPWORD lpAttribute, IN DWORD nLength, IN COORD dwReadCoord, OUT LPDWORD lpNumberOfAttrsRead )
{
    static FN_ReadConsoleOutputAttribute *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadConsoleOutputAttribute", &g_Kernel32);
    return pfn( hConsoleOutput, lpAttribute, nLength, dwReadCoord, lpNumberOfAttrsRead );
}

typedef BOOL WINAPI FN_WriteConsoleOutputCharacterA( IN HANDLE hConsoleOutput, IN LPCSTR lpCharacter, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfCharsWritten );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteConsoleOutputCharacterA( IN HANDLE hConsoleOutput, IN LPCSTR lpCharacter, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfCharsWritten )
{
    static FN_WriteConsoleOutputCharacterA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteConsoleOutputCharacterA", &g_Kernel32);
    return pfn( hConsoleOutput, lpCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten );
}

typedef BOOL WINAPI FN_WriteConsoleOutputCharacterW( IN HANDLE hConsoleOutput, IN LPCWSTR lpCharacter, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfCharsWritten );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteConsoleOutputCharacterW( IN HANDLE hConsoleOutput, IN LPCWSTR lpCharacter, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfCharsWritten )
{
    static FN_WriteConsoleOutputCharacterW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteConsoleOutputCharacterW", &g_Kernel32);
    return pfn( hConsoleOutput, lpCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten );
}

typedef BOOL WINAPI FN_WriteConsoleOutputAttribute( IN HANDLE hConsoleOutput, IN CONST WORD * lpAttribute, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfAttrsWritten );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteConsoleOutputAttribute( IN HANDLE hConsoleOutput, IN CONST WORD * lpAttribute, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfAttrsWritten )
{
    static FN_WriteConsoleOutputAttribute *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteConsoleOutputAttribute", &g_Kernel32);
    return pfn( hConsoleOutput, lpAttribute, nLength, dwWriteCoord, lpNumberOfAttrsWritten );
}

typedef BOOL WINAPI FN_FillConsoleOutputCharacterA( IN HANDLE hConsoleOutput, IN CHAR cCharacter, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfCharsWritten );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FillConsoleOutputCharacterA( IN HANDLE hConsoleOutput, IN CHAR cCharacter, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfCharsWritten )
{
    static FN_FillConsoleOutputCharacterA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FillConsoleOutputCharacterA", &g_Kernel32);
    return pfn( hConsoleOutput, cCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten );
}

typedef BOOL WINAPI FN_FillConsoleOutputCharacterW( IN HANDLE hConsoleOutput, IN WCHAR cCharacter, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfCharsWritten );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FillConsoleOutputCharacterW( IN HANDLE hConsoleOutput, IN WCHAR cCharacter, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfCharsWritten )
{
    static FN_FillConsoleOutputCharacterW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FillConsoleOutputCharacterW", &g_Kernel32);
    return pfn( hConsoleOutput, cCharacter, nLength, dwWriteCoord, lpNumberOfCharsWritten );
}

typedef BOOL WINAPI FN_FillConsoleOutputAttribute( IN HANDLE hConsoleOutput, IN WORD wAttribute, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfAttrsWritten );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FillConsoleOutputAttribute( IN HANDLE hConsoleOutput, IN WORD wAttribute, IN DWORD nLength, IN COORD dwWriteCoord, OUT LPDWORD lpNumberOfAttrsWritten )
{
    static FN_FillConsoleOutputAttribute *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FillConsoleOutputAttribute", &g_Kernel32);
    return pfn( hConsoleOutput, wAttribute, nLength, dwWriteCoord, lpNumberOfAttrsWritten );
}

typedef BOOL WINAPI FN_GetConsoleMode( IN HANDLE hConsoleHandle, OUT LPDWORD lpMode );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetConsoleMode( IN HANDLE hConsoleHandle, OUT LPDWORD lpMode )
{
    static FN_GetConsoleMode *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleMode", &g_Kernel32);
    return pfn( hConsoleHandle, lpMode );
}

typedef BOOL WINAPI FN_GetNumberOfConsoleInputEvents( IN HANDLE hConsoleInput, OUT LPDWORD lpNumberOfEvents );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNumberOfConsoleInputEvents( IN HANDLE hConsoleInput, OUT LPDWORD lpNumberOfEvents )
{
    static FN_GetNumberOfConsoleInputEvents *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNumberOfConsoleInputEvents", &g_Kernel32);
    return pfn( hConsoleInput, lpNumberOfEvents );
}

typedef BOOL WINAPI FN_GetConsoleScreenBufferInfo( IN HANDLE hConsoleOutput, OUT PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetConsoleScreenBufferInfo( IN HANDLE hConsoleOutput, OUT PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo )
{
    static FN_GetConsoleScreenBufferInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleScreenBufferInfo", &g_Kernel32);
    return pfn( hConsoleOutput, lpConsoleScreenBufferInfo );
}

typedef COORD WINAPI FN_GetLargestConsoleWindowSize( IN HANDLE hConsoleOutput );
__declspec(dllexport) COORD WINAPI kPrf2Wrap_GetLargestConsoleWindowSize( IN HANDLE hConsoleOutput )
{
    static FN_GetLargestConsoleWindowSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLargestConsoleWindowSize", &g_Kernel32);
    return pfn( hConsoleOutput );
}

typedef BOOL WINAPI FN_GetConsoleCursorInfo( IN HANDLE hConsoleOutput, OUT PCONSOLE_CURSOR_INFO lpConsoleCursorInfo );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetConsoleCursorInfo( IN HANDLE hConsoleOutput, OUT PCONSOLE_CURSOR_INFO lpConsoleCursorInfo )
{
    static FN_GetConsoleCursorInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleCursorInfo", &g_Kernel32);
    return pfn( hConsoleOutput, lpConsoleCursorInfo );
}

typedef BOOL WINAPI FN_GetCurrentConsoleFont( IN HANDLE hConsoleOutput, IN BOOL bMaximumWindow, OUT PCONSOLE_FONT_INFO lpConsoleCurrentFont );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCurrentConsoleFont( IN HANDLE hConsoleOutput, IN BOOL bMaximumWindow, OUT PCONSOLE_FONT_INFO lpConsoleCurrentFont )
{
    static FN_GetCurrentConsoleFont *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrentConsoleFont", &g_Kernel32);
    return pfn( hConsoleOutput, bMaximumWindow, lpConsoleCurrentFont );
}

typedef COORD WINAPI FN_GetConsoleFontSize( IN HANDLE hConsoleOutput, IN DWORD nFont );
__declspec(dllexport) COORD WINAPI kPrf2Wrap_GetConsoleFontSize( IN HANDLE hConsoleOutput, IN DWORD nFont )
{
    static FN_GetConsoleFontSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleFontSize", &g_Kernel32);
    return pfn( hConsoleOutput, nFont );
}

typedef BOOL WINAPI FN_GetConsoleSelectionInfo( OUT PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetConsoleSelectionInfo( OUT PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo )
{
    static FN_GetConsoleSelectionInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleSelectionInfo", &g_Kernel32);
    return pfn( lpConsoleSelectionInfo );
}

typedef BOOL WINAPI FN_GetNumberOfConsoleMouseButtons( OUT LPDWORD lpNumberOfMouseButtons );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNumberOfConsoleMouseButtons( OUT LPDWORD lpNumberOfMouseButtons )
{
    static FN_GetNumberOfConsoleMouseButtons *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNumberOfConsoleMouseButtons", &g_Kernel32);
    return pfn( lpNumberOfMouseButtons );
}

typedef BOOL WINAPI FN_SetConsoleMode( IN HANDLE hConsoleHandle, IN DWORD dwMode );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleMode( IN HANDLE hConsoleHandle, IN DWORD dwMode )
{
    static FN_SetConsoleMode *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleMode", &g_Kernel32);
    return pfn( hConsoleHandle, dwMode );
}

typedef BOOL WINAPI FN_SetConsoleActiveScreenBuffer( IN HANDLE hConsoleOutput );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleActiveScreenBuffer( IN HANDLE hConsoleOutput )
{
    static FN_SetConsoleActiveScreenBuffer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleActiveScreenBuffer", &g_Kernel32);
    return pfn( hConsoleOutput );
}

typedef BOOL WINAPI FN_FlushConsoleInputBuffer( IN HANDLE hConsoleInput );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FlushConsoleInputBuffer( IN HANDLE hConsoleInput )
{
    static FN_FlushConsoleInputBuffer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FlushConsoleInputBuffer", &g_Kernel32);
    return pfn( hConsoleInput );
}

typedef BOOL WINAPI FN_SetConsoleScreenBufferSize( IN HANDLE hConsoleOutput, IN COORD dwSize );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleScreenBufferSize( IN HANDLE hConsoleOutput, IN COORD dwSize )
{
    static FN_SetConsoleScreenBufferSize *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleScreenBufferSize", &g_Kernel32);
    return pfn( hConsoleOutput, dwSize );
}

typedef BOOL WINAPI FN_SetConsoleCursorPosition( IN HANDLE hConsoleOutput, IN COORD dwCursorPosition );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleCursorPosition( IN HANDLE hConsoleOutput, IN COORD dwCursorPosition )
{
    static FN_SetConsoleCursorPosition *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleCursorPosition", &g_Kernel32);
    return pfn( hConsoleOutput, dwCursorPosition );
}

typedef BOOL WINAPI FN_SetConsoleCursorInfo( IN HANDLE hConsoleOutput, IN CONST CONSOLE_CURSOR_INFO * lpConsoleCursorInfo );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleCursorInfo( IN HANDLE hConsoleOutput, IN CONST CONSOLE_CURSOR_INFO * lpConsoleCursorInfo )
{
    static FN_SetConsoleCursorInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleCursorInfo", &g_Kernel32);
    return pfn( hConsoleOutput, lpConsoleCursorInfo );
}

typedef BOOL WINAPI FN_ScrollConsoleScreenBufferA( IN HANDLE hConsoleOutput, IN CONST SMALL_RECT * lpScrollRectangle, IN CONST SMALL_RECT * lpClipRectangle, IN COORD dwDestinationOrigin, IN CONST CHAR_INFO * lpFill );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ScrollConsoleScreenBufferA( IN HANDLE hConsoleOutput, IN CONST SMALL_RECT * lpScrollRectangle, IN CONST SMALL_RECT * lpClipRectangle, IN COORD dwDestinationOrigin, IN CONST CHAR_INFO * lpFill )
{
    static FN_ScrollConsoleScreenBufferA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ScrollConsoleScreenBufferA", &g_Kernel32);
    return pfn( hConsoleOutput, lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill );
}

typedef BOOL WINAPI FN_ScrollConsoleScreenBufferW( IN HANDLE hConsoleOutput, IN CONST SMALL_RECT * lpScrollRectangle, IN CONST SMALL_RECT * lpClipRectangle, IN COORD dwDestinationOrigin, IN CONST CHAR_INFO * lpFill );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ScrollConsoleScreenBufferW( IN HANDLE hConsoleOutput, IN CONST SMALL_RECT * lpScrollRectangle, IN CONST SMALL_RECT * lpClipRectangle, IN COORD dwDestinationOrigin, IN CONST CHAR_INFO * lpFill )
{
    static FN_ScrollConsoleScreenBufferW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ScrollConsoleScreenBufferW", &g_Kernel32);
    return pfn( hConsoleOutput, lpScrollRectangle, lpClipRectangle, dwDestinationOrigin, lpFill );
}

typedef BOOL WINAPI FN_SetConsoleWindowInfo( IN HANDLE hConsoleOutput, IN BOOL bAbsolute, IN CONST SMALL_RECT * lpConsoleWindow );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleWindowInfo( IN HANDLE hConsoleOutput, IN BOOL bAbsolute, IN CONST SMALL_RECT * lpConsoleWindow )
{
    static FN_SetConsoleWindowInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleWindowInfo", &g_Kernel32);
    return pfn( hConsoleOutput, bAbsolute, lpConsoleWindow );
}

typedef BOOL WINAPI FN_SetConsoleTextAttribute( IN HANDLE hConsoleOutput, IN WORD wAttributes );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleTextAttribute( IN HANDLE hConsoleOutput, IN WORD wAttributes )
{
    static FN_SetConsoleTextAttribute *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleTextAttribute", &g_Kernel32);
    return pfn( hConsoleOutput, wAttributes );
}

typedef BOOL WINAPI FN_SetConsoleCtrlHandler( IN PHANDLER_ROUTINE HandlerRoutine, IN BOOL Add );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleCtrlHandler( IN PHANDLER_ROUTINE HandlerRoutine, IN BOOL Add )
{
    static FN_SetConsoleCtrlHandler *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleCtrlHandler", &g_Kernel32);
    return pfn( HandlerRoutine, Add );
}

typedef BOOL WINAPI FN_GenerateConsoleCtrlEvent( IN DWORD dwCtrlEvent, IN DWORD dwProcessGroupId );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GenerateConsoleCtrlEvent( IN DWORD dwCtrlEvent, IN DWORD dwProcessGroupId )
{
    static FN_GenerateConsoleCtrlEvent *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GenerateConsoleCtrlEvent", &g_Kernel32);
    return pfn( dwCtrlEvent, dwProcessGroupId );
}

typedef BOOL WINAPI FN_AllocConsole( VOID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AllocConsole( VOID )
{
    static FN_AllocConsole *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AllocConsole", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_FreeConsole( VOID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_FreeConsole( VOID )
{
    static FN_FreeConsole *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FreeConsole", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_AttachConsole( IN DWORD dwProcessId );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_AttachConsole( IN DWORD dwProcessId )
{
    static FN_AttachConsole *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AttachConsole", &g_Kernel32);
    return pfn( dwProcessId );
}

typedef DWORD WINAPI FN_GetConsoleTitleA( OUT LPSTR lpConsoleTitle, IN DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetConsoleTitleA( OUT LPSTR lpConsoleTitle, IN DWORD nSize )
{
    static FN_GetConsoleTitleA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleTitleA", &g_Kernel32);
    return pfn( lpConsoleTitle, nSize );
}

typedef DWORD WINAPI FN_GetConsoleTitleW( OUT LPWSTR lpConsoleTitle, IN DWORD nSize );
__declspec(dllexport) DWORD WINAPI kPrf2Wrap_GetConsoleTitleW( OUT LPWSTR lpConsoleTitle, IN DWORD nSize )
{
    static FN_GetConsoleTitleW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleTitleW", &g_Kernel32);
    return pfn( lpConsoleTitle, nSize );
}

typedef BOOL WINAPI FN_SetConsoleTitleA( IN LPCSTR lpConsoleTitle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleTitleA( IN LPCSTR lpConsoleTitle )
{
    static FN_SetConsoleTitleA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleTitleA", &g_Kernel32);
    return pfn( lpConsoleTitle );
}

typedef BOOL WINAPI FN_SetConsoleTitleW( IN LPCWSTR lpConsoleTitle );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleTitleW( IN LPCWSTR lpConsoleTitle )
{
    static FN_SetConsoleTitleW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleTitleW", &g_Kernel32);
    return pfn( lpConsoleTitle );
}

typedef BOOL WINAPI FN_ReadConsoleA( IN HANDLE hConsoleInput, OUT LPVOID lpBuffer, IN DWORD nNumberOfCharsToRead, OUT LPDWORD lpNumberOfCharsRead, IN LPVOID lpReserved );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadConsoleA( IN HANDLE hConsoleInput, OUT LPVOID lpBuffer, IN DWORD nNumberOfCharsToRead, OUT LPDWORD lpNumberOfCharsRead, IN LPVOID lpReserved )
{
    static FN_ReadConsoleA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadConsoleA", &g_Kernel32);
    return pfn( hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, lpReserved );
}

typedef BOOL WINAPI FN_ReadConsoleW( IN HANDLE hConsoleInput, OUT LPVOID lpBuffer, IN DWORD nNumberOfCharsToRead, OUT LPDWORD lpNumberOfCharsRead, IN LPVOID lpReserved );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReadConsoleW( IN HANDLE hConsoleInput, OUT LPVOID lpBuffer, IN DWORD nNumberOfCharsToRead, OUT LPDWORD lpNumberOfCharsRead, IN LPVOID lpReserved )
{
    static FN_ReadConsoleW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReadConsoleW", &g_Kernel32);
    return pfn( hConsoleInput, lpBuffer, nNumberOfCharsToRead, lpNumberOfCharsRead, lpReserved );
}

typedef BOOL WINAPI FN_WriteConsoleA( IN HANDLE hConsoleOutput, IN CONST VOID * lpBuffer, IN DWORD nNumberOfCharsToWrite, OUT LPDWORD lpNumberOfCharsWritten, IN LPVOID lpReserved );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteConsoleA( IN HANDLE hConsoleOutput, IN CONST VOID * lpBuffer, IN DWORD nNumberOfCharsToWrite, OUT LPDWORD lpNumberOfCharsWritten, IN LPVOID lpReserved )
{
    static FN_WriteConsoleA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteConsoleA", &g_Kernel32);
    return pfn( hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved );
}

typedef BOOL WINAPI FN_WriteConsoleW( IN HANDLE hConsoleOutput, IN CONST VOID * lpBuffer, IN DWORD nNumberOfCharsToWrite, OUT LPDWORD lpNumberOfCharsWritten, IN LPVOID lpReserved );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_WriteConsoleW( IN HANDLE hConsoleOutput, IN CONST VOID * lpBuffer, IN DWORD nNumberOfCharsToWrite, OUT LPDWORD lpNumberOfCharsWritten, IN LPVOID lpReserved )
{
    static FN_WriteConsoleW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WriteConsoleW", &g_Kernel32);
    return pfn( hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved );
}

typedef HANDLE WINAPI FN_CreateConsoleScreenBuffer( IN DWORD dwDesiredAccess, IN DWORD dwShareMode, IN CONST SECURITY_ATTRIBUTES * lpSecurityAttributes, IN DWORD dwFlags, IN LPVOID lpScreenBufferData );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateConsoleScreenBuffer( IN DWORD dwDesiredAccess, IN DWORD dwShareMode, IN CONST SECURITY_ATTRIBUTES * lpSecurityAttributes, IN DWORD dwFlags, IN LPVOID lpScreenBufferData )
{
    static FN_CreateConsoleScreenBuffer *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateConsoleScreenBuffer", &g_Kernel32);
    return pfn( dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwFlags, lpScreenBufferData );
}

typedef UINT WINAPI FN_GetConsoleCP( VOID );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetConsoleCP( VOID )
{
    static FN_GetConsoleCP *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleCP", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_SetConsoleCP( IN UINT wCodePageID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleCP( IN UINT wCodePageID )
{
    static FN_SetConsoleCP *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleCP", &g_Kernel32);
    return pfn( wCodePageID );
}

typedef UINT WINAPI FN_GetConsoleOutputCP( VOID );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetConsoleOutputCP( VOID )
{
    static FN_GetConsoleOutputCP *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleOutputCP", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_SetConsoleOutputCP( IN UINT wCodePageID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleOutputCP( IN UINT wCodePageID )
{
    static FN_SetConsoleOutputCP *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleOutputCP", &g_Kernel32);
    return pfn( wCodePageID );
}

typedef BOOL APIENTRY FN_GetConsoleDisplayMode( OUT LPDWORD lpModeFlags );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_GetConsoleDisplayMode( OUT LPDWORD lpModeFlags )
{
    static FN_GetConsoleDisplayMode *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleDisplayMode", &g_Kernel32);
    return pfn( lpModeFlags );
}

typedef HWND APIENTRY FN_GetConsoleWindow( VOID );
__declspec(dllexport) HWND APIENTRY kPrf2Wrap_GetConsoleWindow( VOID )
{
    static FN_GetConsoleWindow *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleWindow", &g_Kernel32);
    return pfn ();
}

typedef DWORD APIENTRY FN_GetConsoleProcessList( OUT LPDWORD lpdwProcessList, IN DWORD dwProcessCount );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleProcessList( OUT LPDWORD lpdwProcessList, IN DWORD dwProcessCount )
{
    static FN_GetConsoleProcessList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleProcessList", &g_Kernel32);
    return pfn( lpdwProcessList, dwProcessCount );
}

typedef BOOL APIENTRY FN_AddConsoleAliasA( IN LPSTR Source, IN LPSTR Target, IN LPSTR ExeName );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_AddConsoleAliasA( IN LPSTR Source, IN LPSTR Target, IN LPSTR ExeName )
{
    static FN_AddConsoleAliasA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddConsoleAliasA", &g_Kernel32);
    return pfn( Source, Target, ExeName );
}

typedef BOOL APIENTRY FN_AddConsoleAliasW( IN LPWSTR Source, IN LPWSTR Target, IN LPWSTR ExeName );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_AddConsoleAliasW( IN LPWSTR Source, IN LPWSTR Target, IN LPWSTR ExeName )
{
    static FN_AddConsoleAliasW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "AddConsoleAliasW", &g_Kernel32);
    return pfn( Source, Target, ExeName );
}

typedef DWORD APIENTRY FN_GetConsoleAliasA( IN LPSTR Source, OUT LPSTR TargetBuffer, IN DWORD TargetBufferLength, IN LPSTR ExeName );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasA( IN LPSTR Source, OUT LPSTR TargetBuffer, IN DWORD TargetBufferLength, IN LPSTR ExeName )
{
    static FN_GetConsoleAliasA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasA", &g_Kernel32);
    return pfn( Source, TargetBuffer, TargetBufferLength, ExeName );
}

typedef DWORD APIENTRY FN_GetConsoleAliasW( IN LPWSTR Source, OUT LPWSTR TargetBuffer, IN DWORD TargetBufferLength, IN LPWSTR ExeName );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasW( IN LPWSTR Source, OUT LPWSTR TargetBuffer, IN DWORD TargetBufferLength, IN LPWSTR ExeName )
{
    static FN_GetConsoleAliasW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasW", &g_Kernel32);
    return pfn( Source, TargetBuffer, TargetBufferLength, ExeName );
}

typedef DWORD APIENTRY FN_GetConsoleAliasesLengthA( IN LPSTR ExeName );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasesLengthA( IN LPSTR ExeName )
{
    static FN_GetConsoleAliasesLengthA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasesLengthA", &g_Kernel32);
    return pfn( ExeName );
}

typedef DWORD APIENTRY FN_GetConsoleAliasesLengthW( IN LPWSTR ExeName );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasesLengthW( IN LPWSTR ExeName )
{
    static FN_GetConsoleAliasesLengthW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasesLengthW", &g_Kernel32);
    return pfn( ExeName );
}

typedef DWORD APIENTRY FN_GetConsoleAliasExesLengthA( VOID );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasExesLengthA( VOID )
{
    static FN_GetConsoleAliasExesLengthA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasExesLengthA", &g_Kernel32);
    return pfn ();
}

typedef DWORD APIENTRY FN_GetConsoleAliasExesLengthW( VOID );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasExesLengthW( VOID )
{
    static FN_GetConsoleAliasExesLengthW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasExesLengthW", &g_Kernel32);
    return pfn ();
}

typedef DWORD APIENTRY FN_GetConsoleAliasesA( OUT LPSTR AliasBuffer, IN DWORD AliasBufferLength, IN LPSTR ExeName );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasesA( OUT LPSTR AliasBuffer, IN DWORD AliasBufferLength, IN LPSTR ExeName )
{
    static FN_GetConsoleAliasesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasesA", &g_Kernel32);
    return pfn( AliasBuffer, AliasBufferLength, ExeName );
}

typedef DWORD APIENTRY FN_GetConsoleAliasesW( OUT LPWSTR AliasBuffer, IN DWORD AliasBufferLength, IN LPWSTR ExeName );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasesW( OUT LPWSTR AliasBuffer, IN DWORD AliasBufferLength, IN LPWSTR ExeName )
{
    static FN_GetConsoleAliasesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasesW", &g_Kernel32);
    return pfn( AliasBuffer, AliasBufferLength, ExeName );
}

typedef DWORD APIENTRY FN_GetConsoleAliasExesA( OUT LPSTR ExeNameBuffer, IN DWORD ExeNameBufferLength );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasExesA( OUT LPSTR ExeNameBuffer, IN DWORD ExeNameBufferLength )
{
    static FN_GetConsoleAliasExesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasExesA", &g_Kernel32);
    return pfn( ExeNameBuffer, ExeNameBufferLength );
}

typedef DWORD APIENTRY FN_GetConsoleAliasExesW( OUT LPWSTR ExeNameBuffer, IN DWORD ExeNameBufferLength );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetConsoleAliasExesW( OUT LPWSTR ExeNameBuffer, IN DWORD ExeNameBufferLength )
{
    static FN_GetConsoleAliasExesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetConsoleAliasExesW", &g_Kernel32);
    return pfn( ExeNameBuffer, ExeNameBufferLength );
}

typedef BOOL WINAPI FN_IsValidCodePage( UINT CodePage );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsValidCodePage( UINT CodePage )
{
    static FN_IsValidCodePage *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsValidCodePage", &g_Kernel32);
    return pfn( CodePage );
}

typedef UINT WINAPI FN_GetACP( void );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetACP( void )
{
    static FN_GetACP *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetACP", &g_Kernel32);
    return pfn ();
}

typedef UINT WINAPI FN_GetOEMCP( void );
__declspec(dllexport) UINT WINAPI kPrf2Wrap_GetOEMCP( void )
{
    static FN_GetOEMCP *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetOEMCP", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_GetCPInfo( UINT CodePage, LPCPINFO lpCPInfo );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCPInfo( UINT CodePage, LPCPINFO lpCPInfo )
{
    static FN_GetCPInfo *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCPInfo", &g_Kernel32);
    return pfn( CodePage, lpCPInfo );
}

typedef BOOL WINAPI FN_GetCPInfoExA( UINT CodePage, DWORD dwFlags, LPCPINFOEXA lpCPInfoEx );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCPInfoExA( UINT CodePage, DWORD dwFlags, LPCPINFOEXA lpCPInfoEx )
{
    static FN_GetCPInfoExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCPInfoExA", &g_Kernel32);
    return pfn( CodePage, dwFlags, lpCPInfoEx );
}

typedef BOOL WINAPI FN_GetCPInfoExW( UINT CodePage, DWORD dwFlags, LPCPINFOEXW lpCPInfoEx );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetCPInfoExW( UINT CodePage, DWORD dwFlags, LPCPINFOEXW lpCPInfoEx )
{
    static FN_GetCPInfoExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCPInfoExW", &g_Kernel32);
    return pfn( CodePage, dwFlags, lpCPInfoEx );
}

typedef BOOL WINAPI FN_IsDBCSLeadByte( BYTE TestChar );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsDBCSLeadByte( BYTE TestChar )
{
    static FN_IsDBCSLeadByte *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsDBCSLeadByte", &g_Kernel32);
    return pfn( TestChar );
}

typedef BOOL WINAPI FN_IsDBCSLeadByteEx( UINT CodePage, BYTE TestChar );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsDBCSLeadByteEx( UINT CodePage, BYTE TestChar )
{
    static FN_IsDBCSLeadByteEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsDBCSLeadByteEx", &g_Kernel32);
    return pfn( CodePage, TestChar );
}

typedef int WINAPI FN_MultiByteToWideChar( UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar );
__declspec(dllexport) int WINAPI kPrf2Wrap_MultiByteToWideChar( UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar )
{
    static FN_MultiByteToWideChar *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "MultiByteToWideChar", &g_Kernel32);
    return pfn( CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar );
}

typedef int WINAPI FN_WideCharToMultiByte( UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar );
__declspec(dllexport) int WINAPI kPrf2Wrap_WideCharToMultiByte( UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar )
{
    static FN_WideCharToMultiByte *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "WideCharToMultiByte", &g_Kernel32);
    return pfn( CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar );
}

typedef int WINAPI FN_CompareStringA( LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_CompareStringA( LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2 )
{
    static FN_CompareStringA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CompareStringA", &g_Kernel32);
    return pfn( Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2 );
}

typedef int WINAPI FN_CompareStringW( LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_CompareStringW( LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2 )
{
    static FN_CompareStringW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CompareStringW", &g_Kernel32);
    return pfn( Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2 );
}

typedef int WINAPI FN_LCMapStringA( LCID Locale, DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest );
__declspec(dllexport) int WINAPI kPrf2Wrap_LCMapStringA( LCID Locale, DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest )
{
    static FN_LCMapStringA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LCMapStringA", &g_Kernel32);
    return pfn( Locale, dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest );
}

typedef int WINAPI FN_LCMapStringW( LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest );
__declspec(dllexport) int WINAPI kPrf2Wrap_LCMapStringW( LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest )
{
    static FN_LCMapStringW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "LCMapStringW", &g_Kernel32);
    return pfn( Locale, dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest );
}

typedef int WINAPI FN_GetLocaleInfoA( LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetLocaleInfoA( LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData )
{
    static FN_GetLocaleInfoA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLocaleInfoA", &g_Kernel32);
    return pfn( Locale, LCType, lpLCData, cchData );
}

typedef int WINAPI FN_GetLocaleInfoW( LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetLocaleInfoW( LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData )
{
    static FN_GetLocaleInfoW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetLocaleInfoW", &g_Kernel32);
    return pfn( Locale, LCType, lpLCData, cchData );
}

typedef BOOL WINAPI FN_SetLocaleInfoA( LCID Locale, LCTYPE LCType, LPCSTR lpLCData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetLocaleInfoA( LCID Locale, LCTYPE LCType, LPCSTR lpLCData )
{
    static FN_SetLocaleInfoA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetLocaleInfoA", &g_Kernel32);
    return pfn( Locale, LCType, lpLCData );
}

typedef BOOL WINAPI FN_SetLocaleInfoW( LCID Locale, LCTYPE LCType, LPCWSTR lpLCData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetLocaleInfoW( LCID Locale, LCTYPE LCType, LPCWSTR lpLCData )
{
    static FN_SetLocaleInfoW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetLocaleInfoW", &g_Kernel32);
    return pfn( Locale, LCType, lpLCData );
}

typedef int WINAPI FN_GetCalendarInfoA( LCID Locale, CALID Calendar, CALTYPE CalType, LPSTR lpCalData, int cchData, LPDWORD lpValue );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetCalendarInfoA( LCID Locale, CALID Calendar, CALTYPE CalType, LPSTR lpCalData, int cchData, LPDWORD lpValue )
{
    static FN_GetCalendarInfoA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCalendarInfoA", &g_Kernel32);
    return pfn( Locale, Calendar, CalType, lpCalData, cchData, lpValue );
}

typedef int WINAPI FN_GetCalendarInfoW( LCID Locale, CALID Calendar, CALTYPE CalType, LPWSTR lpCalData, int cchData, LPDWORD lpValue );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetCalendarInfoW( LCID Locale, CALID Calendar, CALTYPE CalType, LPWSTR lpCalData, int cchData, LPDWORD lpValue )
{
    static FN_GetCalendarInfoW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCalendarInfoW", &g_Kernel32);
    return pfn( Locale, Calendar, CalType, lpCalData, cchData, lpValue );
}

typedef BOOL WINAPI FN_SetCalendarInfoA( LCID Locale, CALID Calendar, CALTYPE CalType, LPCSTR lpCalData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetCalendarInfoA( LCID Locale, CALID Calendar, CALTYPE CalType, LPCSTR lpCalData )
{
    static FN_SetCalendarInfoA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCalendarInfoA", &g_Kernel32);
    return pfn( Locale, Calendar, CalType, lpCalData );
}

typedef BOOL WINAPI FN_SetCalendarInfoW( LCID Locale, CALID Calendar, CALTYPE CalType, LPCWSTR lpCalData );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetCalendarInfoW( LCID Locale, CALID Calendar, CALTYPE CalType, LPCWSTR lpCalData )
{
    static FN_SetCalendarInfoW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetCalendarInfoW", &g_Kernel32);
    return pfn( Locale, Calendar, CalType, lpCalData );
}

typedef int WINAPI FN_GetTimeFormatA( LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpTime, LPCSTR lpFormat, LPSTR lpTimeStr, int cchTime );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetTimeFormatA( LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpTime, LPCSTR lpFormat, LPSTR lpTimeStr, int cchTime )
{
    static FN_GetTimeFormatA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTimeFormatA", &g_Kernel32);
    return pfn( Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime );
}

typedef int WINAPI FN_GetTimeFormatW( LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetTimeFormatW( LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime )
{
    static FN_GetTimeFormatW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetTimeFormatW", &g_Kernel32);
    return pfn( Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime );
}

typedef int WINAPI FN_GetDateFormatA( LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpDate, LPCSTR lpFormat, LPSTR lpDateStr, int cchDate );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetDateFormatA( LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpDate, LPCSTR lpFormat, LPSTR lpDateStr, int cchDate )
{
    static FN_GetDateFormatA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDateFormatA", &g_Kernel32);
    return pfn( Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate );
}

typedef int WINAPI FN_GetDateFormatW( LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetDateFormatW( LCID Locale, DWORD dwFlags, CONST SYSTEMTIME * lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate )
{
    static FN_GetDateFormatW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetDateFormatW", &g_Kernel32);
    return pfn( Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate );
}

typedef int WINAPI FN_GetNumberFormatA( LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST NUMBERFMTA * lpFormat, LPSTR lpNumberStr, int cchNumber );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetNumberFormatA( LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST NUMBERFMTA * lpFormat, LPSTR lpNumberStr, int cchNumber )
{
    static FN_GetNumberFormatA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNumberFormatA", &g_Kernel32);
    return pfn( Locale, dwFlags, lpValue, lpFormat, lpNumberStr, cchNumber );
}

typedef int WINAPI FN_GetNumberFormatW( LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST NUMBERFMTW * lpFormat, LPWSTR lpNumberStr, int cchNumber );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetNumberFormatW( LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST NUMBERFMTW * lpFormat, LPWSTR lpNumberStr, int cchNumber )
{
    static FN_GetNumberFormatW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNumberFormatW", &g_Kernel32);
    return pfn( Locale, dwFlags, lpValue, lpFormat, lpNumberStr, cchNumber );
}

typedef int WINAPI FN_GetCurrencyFormatA( LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST CURRENCYFMTA * lpFormat, LPSTR lpCurrencyStr, int cchCurrency );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetCurrencyFormatA( LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST CURRENCYFMTA * lpFormat, LPSTR lpCurrencyStr, int cchCurrency )
{
    static FN_GetCurrencyFormatA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrencyFormatA", &g_Kernel32);
    return pfn( Locale, dwFlags, lpValue, lpFormat, lpCurrencyStr, cchCurrency );
}

typedef int WINAPI FN_GetCurrencyFormatW( LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST CURRENCYFMTW * lpFormat, LPWSTR lpCurrencyStr, int cchCurrency );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetCurrencyFormatW( LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST CURRENCYFMTW * lpFormat, LPWSTR lpCurrencyStr, int cchCurrency )
{
    static FN_GetCurrencyFormatW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetCurrencyFormatW", &g_Kernel32);
    return pfn( Locale, dwFlags, lpValue, lpFormat, lpCurrencyStr, cchCurrency );
}

typedef BOOL WINAPI FN_EnumCalendarInfoA( CALINFO_ENUMPROCA lpCalInfoEnumProc, LCID Locale, CALID Calendar, CALTYPE CalType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumCalendarInfoA( CALINFO_ENUMPROCA lpCalInfoEnumProc, LCID Locale, CALID Calendar, CALTYPE CalType )
{
    static FN_EnumCalendarInfoA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumCalendarInfoA", &g_Kernel32);
    return pfn( lpCalInfoEnumProc, Locale, Calendar, CalType );
}

typedef BOOL WINAPI FN_EnumCalendarInfoW( CALINFO_ENUMPROCW lpCalInfoEnumProc, LCID Locale, CALID Calendar, CALTYPE CalType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumCalendarInfoW( CALINFO_ENUMPROCW lpCalInfoEnumProc, LCID Locale, CALID Calendar, CALTYPE CalType )
{
    static FN_EnumCalendarInfoW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumCalendarInfoW", &g_Kernel32);
    return pfn( lpCalInfoEnumProc, Locale, Calendar, CalType );
}

typedef BOOL WINAPI FN_EnumCalendarInfoExA( CALINFO_ENUMPROCEXA lpCalInfoEnumProcEx, LCID Locale, CALID Calendar, CALTYPE CalType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumCalendarInfoExA( CALINFO_ENUMPROCEXA lpCalInfoEnumProcEx, LCID Locale, CALID Calendar, CALTYPE CalType )
{
    static FN_EnumCalendarInfoExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumCalendarInfoExA", &g_Kernel32);
    return pfn( lpCalInfoEnumProcEx, Locale, Calendar, CalType );
}

typedef BOOL WINAPI FN_EnumCalendarInfoExW( CALINFO_ENUMPROCEXW lpCalInfoEnumProcEx, LCID Locale, CALID Calendar, CALTYPE CalType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumCalendarInfoExW( CALINFO_ENUMPROCEXW lpCalInfoEnumProcEx, LCID Locale, CALID Calendar, CALTYPE CalType )
{
    static FN_EnumCalendarInfoExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumCalendarInfoExW", &g_Kernel32);
    return pfn( lpCalInfoEnumProcEx, Locale, Calendar, CalType );
}

typedef BOOL WINAPI FN_EnumTimeFormatsA( TIMEFMT_ENUMPROCA lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumTimeFormatsA( TIMEFMT_ENUMPROCA lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags )
{
    static FN_EnumTimeFormatsA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumTimeFormatsA", &g_Kernel32);
    return pfn( lpTimeFmtEnumProc, Locale, dwFlags );
}

typedef BOOL WINAPI FN_EnumTimeFormatsW( TIMEFMT_ENUMPROCW lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumTimeFormatsW( TIMEFMT_ENUMPROCW lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags )
{
    static FN_EnumTimeFormatsW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumTimeFormatsW", &g_Kernel32);
    return pfn( lpTimeFmtEnumProc, Locale, dwFlags );
}

typedef BOOL WINAPI FN_EnumDateFormatsA( DATEFMT_ENUMPROCA lpDateFmtEnumProc, LCID Locale, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumDateFormatsA( DATEFMT_ENUMPROCA lpDateFmtEnumProc, LCID Locale, DWORD dwFlags )
{
    static FN_EnumDateFormatsA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumDateFormatsA", &g_Kernel32);
    return pfn( lpDateFmtEnumProc, Locale, dwFlags );
}

typedef BOOL WINAPI FN_EnumDateFormatsW( DATEFMT_ENUMPROCW lpDateFmtEnumProc, LCID Locale, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumDateFormatsW( DATEFMT_ENUMPROCW lpDateFmtEnumProc, LCID Locale, DWORD dwFlags )
{
    static FN_EnumDateFormatsW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumDateFormatsW", &g_Kernel32);
    return pfn( lpDateFmtEnumProc, Locale, dwFlags );
}

typedef BOOL WINAPI FN_EnumDateFormatsExA( DATEFMT_ENUMPROCEXA lpDateFmtEnumProcEx, LCID Locale, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumDateFormatsExA( DATEFMT_ENUMPROCEXA lpDateFmtEnumProcEx, LCID Locale, DWORD dwFlags )
{
    static FN_EnumDateFormatsExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumDateFormatsExA", &g_Kernel32);
    return pfn( lpDateFmtEnumProcEx, Locale, dwFlags );
}

typedef BOOL WINAPI FN_EnumDateFormatsExW( DATEFMT_ENUMPROCEXW lpDateFmtEnumProcEx, LCID Locale, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumDateFormatsExW( DATEFMT_ENUMPROCEXW lpDateFmtEnumProcEx, LCID Locale, DWORD dwFlags )
{
    static FN_EnumDateFormatsExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumDateFormatsExW", &g_Kernel32);
    return pfn( lpDateFmtEnumProcEx, Locale, dwFlags );
}

typedef BOOL WINAPI FN_IsValidLanguageGroup( LGRPID LanguageGroup, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsValidLanguageGroup( LGRPID LanguageGroup, DWORD dwFlags )
{
    static FN_IsValidLanguageGroup *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsValidLanguageGroup", &g_Kernel32);
    return pfn( LanguageGroup, dwFlags );
}

typedef BOOL WINAPI FN_GetNLSVersion( NLS_FUNCTION Function, LCID Locale, LPNLSVERSIONINFO lpVersionInformation );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetNLSVersion( NLS_FUNCTION Function, LCID Locale, LPNLSVERSIONINFO lpVersionInformation )
{
    static FN_GetNLSVersion *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetNLSVersion", &g_Kernel32);
    return pfn( Function, Locale, lpVersionInformation );
}

typedef BOOL WINAPI FN_IsNLSDefinedString( NLS_FUNCTION Function, DWORD dwFlags, LPNLSVERSIONINFO lpVersionInformation, LPCWSTR lpString, INT cchStr );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsNLSDefinedString( NLS_FUNCTION Function, DWORD dwFlags, LPNLSVERSIONINFO lpVersionInformation, LPCWSTR lpString, INT cchStr )
{
    static FN_IsNLSDefinedString *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsNLSDefinedString", &g_Kernel32);
    return pfn( Function, dwFlags, lpVersionInformation, lpString, cchStr );
}

typedef BOOL WINAPI FN_IsValidLocale( LCID Locale, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_IsValidLocale( LCID Locale, DWORD dwFlags )
{
    static FN_IsValidLocale *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "IsValidLocale", &g_Kernel32);
    return pfn( Locale, dwFlags );
}

typedef int WINAPI FN_GetGeoInfoA( GEOID Location, GEOTYPE GeoType, LPSTR lpGeoData, int cchData, LANGID LangId );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetGeoInfoA( GEOID Location, GEOTYPE GeoType, LPSTR lpGeoData, int cchData, LANGID LangId )
{
    static FN_GetGeoInfoA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetGeoInfoA", &g_Kernel32);
    return pfn( Location, GeoType, lpGeoData, cchData, LangId );
}

typedef int WINAPI FN_GetGeoInfoW( GEOID Location, GEOTYPE GeoType, LPWSTR lpGeoData, int cchData, LANGID LangId );
__declspec(dllexport) int WINAPI kPrf2Wrap_GetGeoInfoW( GEOID Location, GEOTYPE GeoType, LPWSTR lpGeoData, int cchData, LANGID LangId )
{
    static FN_GetGeoInfoW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetGeoInfoW", &g_Kernel32);
    return pfn( Location, GeoType, lpGeoData, cchData, LangId );
}

typedef BOOL WINAPI FN_EnumSystemGeoID( GEOCLASS GeoClass, GEOID ParentGeoId, GEO_ENUMPROC lpGeoEnumProc );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumSystemGeoID( GEOCLASS GeoClass, GEOID ParentGeoId, GEO_ENUMPROC lpGeoEnumProc )
{
    static FN_EnumSystemGeoID *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumSystemGeoID", &g_Kernel32);
    return pfn( GeoClass, ParentGeoId, lpGeoEnumProc );
}

typedef GEOID WINAPI FN_GetUserGeoID( GEOCLASS GeoClass );
__declspec(dllexport) GEOID WINAPI kPrf2Wrap_GetUserGeoID( GEOCLASS GeoClass )
{
    static FN_GetUserGeoID *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetUserGeoID", &g_Kernel32);
    return pfn( GeoClass );
}

typedef BOOL WINAPI FN_SetUserGeoID( GEOID GeoId );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetUserGeoID( GEOID GeoId )
{
    static FN_SetUserGeoID *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetUserGeoID", &g_Kernel32);
    return pfn( GeoId );
}

typedef LCID WINAPI FN_ConvertDefaultLocale( LCID Locale );
__declspec(dllexport) LCID WINAPI kPrf2Wrap_ConvertDefaultLocale( LCID Locale )
{
    static FN_ConvertDefaultLocale *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ConvertDefaultLocale", &g_Kernel32);
    return pfn( Locale );
}

typedef LCID WINAPI FN_GetThreadLocale( void );
__declspec(dllexport) LCID WINAPI kPrf2Wrap_GetThreadLocale( void )
{
    static FN_GetThreadLocale *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetThreadLocale", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_SetThreadLocale( LCID Locale );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetThreadLocale( LCID Locale )
{
    static FN_SetThreadLocale *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetThreadLocale", &g_Kernel32);
    return pfn( Locale );
}

typedef LANGID WINAPI FN_GetSystemDefaultUILanguage( void );
__declspec(dllexport) LANGID WINAPI kPrf2Wrap_GetSystemDefaultUILanguage( void )
{
    static FN_GetSystemDefaultUILanguage *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemDefaultUILanguage", &g_Kernel32);
    return pfn ();
}

typedef LANGID WINAPI FN_GetUserDefaultUILanguage( void );
__declspec(dllexport) LANGID WINAPI kPrf2Wrap_GetUserDefaultUILanguage( void )
{
    static FN_GetUserDefaultUILanguage *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetUserDefaultUILanguage", &g_Kernel32);
    return pfn ();
}

typedef LANGID WINAPI FN_GetSystemDefaultLangID( void );
__declspec(dllexport) LANGID WINAPI kPrf2Wrap_GetSystemDefaultLangID( void )
{
    static FN_GetSystemDefaultLangID *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemDefaultLangID", &g_Kernel32);
    return pfn ();
}

typedef LANGID WINAPI FN_GetUserDefaultLangID( void );
__declspec(dllexport) LANGID WINAPI kPrf2Wrap_GetUserDefaultLangID( void )
{
    static FN_GetUserDefaultLangID *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetUserDefaultLangID", &g_Kernel32);
    return pfn ();
}

typedef LCID WINAPI FN_GetSystemDefaultLCID( void );
__declspec(dllexport) LCID WINAPI kPrf2Wrap_GetSystemDefaultLCID( void )
{
    static FN_GetSystemDefaultLCID *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetSystemDefaultLCID", &g_Kernel32);
    return pfn ();
}

typedef LCID WINAPI FN_GetUserDefaultLCID( void );
__declspec(dllexport) LCID WINAPI kPrf2Wrap_GetUserDefaultLCID( void )
{
    static FN_GetUserDefaultLCID *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetUserDefaultLCID", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_GetStringTypeExA( LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetStringTypeExA( LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType )
{
    static FN_GetStringTypeExA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetStringTypeExA", &g_Kernel32);
    return pfn( Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType );
}

typedef BOOL WINAPI FN_GetStringTypeExW( LCID Locale, DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetStringTypeExW( LCID Locale, DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType )
{
    static FN_GetStringTypeExW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetStringTypeExW", &g_Kernel32);
    return pfn( Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType );
}

typedef BOOL WINAPI FN_GetStringTypeA( LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetStringTypeA( LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType )
{
    static FN_GetStringTypeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetStringTypeA", &g_Kernel32);
    return pfn( Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType );
}

typedef BOOL WINAPI FN_GetStringTypeW( DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetStringTypeW( DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType )
{
    static FN_GetStringTypeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetStringTypeW", &g_Kernel32);
    return pfn( dwInfoType, lpSrcStr, cchSrc, lpCharType );
}

typedef int WINAPI FN_FoldStringA( DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest );
__declspec(dllexport) int WINAPI kPrf2Wrap_FoldStringA( DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest )
{
    static FN_FoldStringA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FoldStringA", &g_Kernel32);
    return pfn( dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest );
}

typedef int WINAPI FN_FoldStringW( DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest );
__declspec(dllexport) int WINAPI kPrf2Wrap_FoldStringW( DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest )
{
    static FN_FoldStringW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "FoldStringW", &g_Kernel32);
    return pfn( dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest );
}

typedef BOOL WINAPI FN_EnumSystemLanguageGroupsA( LANGUAGEGROUP_ENUMPROCA lpLanguageGroupEnumProc, DWORD dwFlags, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumSystemLanguageGroupsA( LANGUAGEGROUP_ENUMPROCA lpLanguageGroupEnumProc, DWORD dwFlags, LONG_PTR lParam )
{
    static FN_EnumSystemLanguageGroupsA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumSystemLanguageGroupsA", &g_Kernel32);
    return pfn( lpLanguageGroupEnumProc, dwFlags, lParam );
}

typedef BOOL WINAPI FN_EnumSystemLanguageGroupsW( LANGUAGEGROUP_ENUMPROCW lpLanguageGroupEnumProc, DWORD dwFlags, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumSystemLanguageGroupsW( LANGUAGEGROUP_ENUMPROCW lpLanguageGroupEnumProc, DWORD dwFlags, LONG_PTR lParam )
{
    static FN_EnumSystemLanguageGroupsW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumSystemLanguageGroupsW", &g_Kernel32);
    return pfn( lpLanguageGroupEnumProc, dwFlags, lParam );
}

typedef BOOL WINAPI FN_EnumLanguageGroupLocalesA( LANGGROUPLOCALE_ENUMPROCA lpLangGroupLocaleEnumProc, LGRPID LanguageGroup, DWORD dwFlags, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumLanguageGroupLocalesA( LANGGROUPLOCALE_ENUMPROCA lpLangGroupLocaleEnumProc, LGRPID LanguageGroup, DWORD dwFlags, LONG_PTR lParam )
{
    static FN_EnumLanguageGroupLocalesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumLanguageGroupLocalesA", &g_Kernel32);
    return pfn( lpLangGroupLocaleEnumProc, LanguageGroup, dwFlags, lParam );
}

typedef BOOL WINAPI FN_EnumLanguageGroupLocalesW( LANGGROUPLOCALE_ENUMPROCW lpLangGroupLocaleEnumProc, LGRPID LanguageGroup, DWORD dwFlags, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumLanguageGroupLocalesW( LANGGROUPLOCALE_ENUMPROCW lpLangGroupLocaleEnumProc, LGRPID LanguageGroup, DWORD dwFlags, LONG_PTR lParam )
{
    static FN_EnumLanguageGroupLocalesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumLanguageGroupLocalesW", &g_Kernel32);
    return pfn( lpLangGroupLocaleEnumProc, LanguageGroup, dwFlags, lParam );
}

typedef BOOL WINAPI FN_EnumUILanguagesA( UILANGUAGE_ENUMPROCA lpUILanguageEnumProc, DWORD dwFlags, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumUILanguagesA( UILANGUAGE_ENUMPROCA lpUILanguageEnumProc, DWORD dwFlags, LONG_PTR lParam )
{
    static FN_EnumUILanguagesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumUILanguagesA", &g_Kernel32);
    return pfn( lpUILanguageEnumProc, dwFlags, lParam );
}

typedef BOOL WINAPI FN_EnumUILanguagesW( UILANGUAGE_ENUMPROCW lpUILanguageEnumProc, DWORD dwFlags, LONG_PTR lParam );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumUILanguagesW( UILANGUAGE_ENUMPROCW lpUILanguageEnumProc, DWORD dwFlags, LONG_PTR lParam )
{
    static FN_EnumUILanguagesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumUILanguagesW", &g_Kernel32);
    return pfn( lpUILanguageEnumProc, dwFlags, lParam );
}

typedef BOOL WINAPI FN_EnumSystemLocalesA( LOCALE_ENUMPROCA lpLocaleEnumProc, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumSystemLocalesA( LOCALE_ENUMPROCA lpLocaleEnumProc, DWORD dwFlags )
{
    static FN_EnumSystemLocalesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumSystemLocalesA", &g_Kernel32);
    return pfn( lpLocaleEnumProc, dwFlags );
}

typedef BOOL WINAPI FN_EnumSystemLocalesW( LOCALE_ENUMPROCW lpLocaleEnumProc, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumSystemLocalesW( LOCALE_ENUMPROCW lpLocaleEnumProc, DWORD dwFlags )
{
    static FN_EnumSystemLocalesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumSystemLocalesW", &g_Kernel32);
    return pfn( lpLocaleEnumProc, dwFlags );
}

typedef BOOL WINAPI FN_EnumSystemCodePagesA( CODEPAGE_ENUMPROCA lpCodePageEnumProc, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumSystemCodePagesA( CODEPAGE_ENUMPROCA lpCodePageEnumProc, DWORD dwFlags )
{
    static FN_EnumSystemCodePagesA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumSystemCodePagesA", &g_Kernel32);
    return pfn( lpCodePageEnumProc, dwFlags );
}

typedef BOOL WINAPI FN_EnumSystemCodePagesW( CODEPAGE_ENUMPROCW lpCodePageEnumProc, DWORD dwFlags );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_EnumSystemCodePagesW( CODEPAGE_ENUMPROCW lpCodePageEnumProc, DWORD dwFlags )
{
    static FN_EnumSystemCodePagesW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "EnumSystemCodePagesW", &g_Kernel32);
    return pfn( lpCodePageEnumProc, dwFlags );
}

typedef DWORD APIENTRY FN_VerFindFileA( DWORD uFlags, LPSTR szFileName, LPSTR szWinDir, LPSTR szAppDir, LPSTR szCurDir, PUINT lpuCurDirLen, LPSTR szDestDir, PUINT lpuDestDirLen );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_VerFindFileA( DWORD uFlags, LPSTR szFileName, LPSTR szWinDir, LPSTR szAppDir, LPSTR szCurDir, PUINT lpuCurDirLen, LPSTR szDestDir, PUINT lpuDestDirLen )
{
    static FN_VerFindFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerFindFileA", &g_Kernel32);
    return pfn( uFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen );
}

typedef DWORD APIENTRY FN_VerFindFileW( DWORD uFlags, LPWSTR szFileName, LPWSTR szWinDir, LPWSTR szAppDir, LPWSTR szCurDir, PUINT lpuCurDirLen, LPWSTR szDestDir, PUINT lpuDestDirLen );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_VerFindFileW( DWORD uFlags, LPWSTR szFileName, LPWSTR szWinDir, LPWSTR szAppDir, LPWSTR szCurDir, PUINT lpuCurDirLen, LPWSTR szDestDir, PUINT lpuDestDirLen )
{
    static FN_VerFindFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerFindFileW", &g_Kernel32);
    return pfn( uFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen );
}

typedef DWORD APIENTRY FN_VerInstallFileA( DWORD uFlags, LPSTR szSrcFileName, LPSTR szDestFileName, LPSTR szSrcDir, LPSTR szDestDir, LPSTR szCurDir, LPSTR szTmpFile, PUINT lpuTmpFileLen );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_VerInstallFileA( DWORD uFlags, LPSTR szSrcFileName, LPSTR szDestFileName, LPSTR szSrcDir, LPSTR szDestDir, LPSTR szCurDir, LPSTR szTmpFile, PUINT lpuTmpFileLen )
{
    static FN_VerInstallFileA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerInstallFileA", &g_Kernel32);
    return pfn( uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, lpuTmpFileLen );
}

typedef DWORD APIENTRY FN_VerInstallFileW( DWORD uFlags, LPWSTR szSrcFileName, LPWSTR szDestFileName, LPWSTR szSrcDir, LPWSTR szDestDir, LPWSTR szCurDir, LPWSTR szTmpFile, PUINT lpuTmpFileLen );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_VerInstallFileW( DWORD uFlags, LPWSTR szSrcFileName, LPWSTR szDestFileName, LPWSTR szSrcDir, LPWSTR szDestDir, LPWSTR szCurDir, LPWSTR szTmpFile, PUINT lpuTmpFileLen )
{
    static FN_VerInstallFileW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerInstallFileW", &g_Kernel32);
    return pfn( uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, lpuTmpFileLen );
}

typedef DWORD APIENTRY FN_GetFileVersionInfoSizeA( LPCSTR lptstrFilename, LPDWORD lpdwHandle );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetFileVersionInfoSizeA( LPCSTR lptstrFilename, LPDWORD lpdwHandle )
{
    static FN_GetFileVersionInfoSizeA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileVersionInfoSizeA", &g_Kernel32);
    return pfn( lptstrFilename, lpdwHandle );
}

typedef DWORD APIENTRY FN_GetFileVersionInfoSizeW( LPCWSTR lptstrFilename, LPDWORD lpdwHandle );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_GetFileVersionInfoSizeW( LPCWSTR lptstrFilename, LPDWORD lpdwHandle )
{
    static FN_GetFileVersionInfoSizeW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileVersionInfoSizeW", &g_Kernel32);
    return pfn( lptstrFilename, lpdwHandle );
}

typedef BOOL APIENTRY FN_GetFileVersionInfoA( LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_GetFileVersionInfoA( LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData )
{
    static FN_GetFileVersionInfoA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileVersionInfoA", &g_Kernel32);
    return pfn( lptstrFilename, dwHandle, dwLen, lpData );
}

typedef BOOL APIENTRY FN_GetFileVersionInfoW( LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_GetFileVersionInfoW( LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData )
{
    static FN_GetFileVersionInfoW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetFileVersionInfoW", &g_Kernel32);
    return pfn( lptstrFilename, dwHandle, dwLen, lpData );
}

typedef DWORD APIENTRY FN_VerLanguageNameA( DWORD wLang, LPSTR szLang, DWORD nSize );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_VerLanguageNameA( DWORD wLang, LPSTR szLang, DWORD nSize )
{
    static FN_VerLanguageNameA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerLanguageNameA", &g_Kernel32);
    return pfn( wLang, szLang, nSize );
}

typedef DWORD APIENTRY FN_VerLanguageNameW( DWORD wLang, LPWSTR szLang, DWORD nSize );
__declspec(dllexport) DWORD APIENTRY kPrf2Wrap_VerLanguageNameW( DWORD wLang, LPWSTR szLang, DWORD nSize )
{
    static FN_VerLanguageNameW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerLanguageNameW", &g_Kernel32);
    return pfn( wLang, szLang, nSize );
}

typedef BOOL APIENTRY FN_VerQueryValueA( const LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_VerQueryValueA( const LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen )
{
    static FN_VerQueryValueA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerQueryValueA", &g_Kernel32);
    return pfn( pBlock, lpSubBlock, lplpBuffer, puLen );
}

typedef BOOL APIENTRY FN_VerQueryValueW( const LPVOID pBlock, LPWSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen );
__declspec(dllexport) BOOL APIENTRY kPrf2Wrap_VerQueryValueW( const LPVOID pBlock, LPWSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen )
{
    static FN_VerQueryValueW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerQueryValueW", &g_Kernel32);
    return pfn( pBlock, lpSubBlock, lplpBuffer, puLen );
}

typedef VOID __cdecl FN_RtlRestoreContext( IN PCONTEXT ContextRecord, IN struct _EXCEPTION_RECORD * ExceptionRecord );
__declspec(dllexport) VOID __cdecl kPrf2Wrap_RtlRestoreContext( IN PCONTEXT ContextRecord, IN struct _EXCEPTION_RECORD * ExceptionRecord )
{
    static FN_RtlRestoreContext *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlRestoreContext", &g_Kernel32);
    pfn( ContextRecord, ExceptionRecord );
}

typedef BOOLEAN __cdecl FN_RtlAddFunctionTable( IN PRUNTIME_FUNCTION FunctionTable, IN DWORD EntryCount, IN DWORD64 BaseAddress );
__declspec(dllexport) BOOLEAN __cdecl kPrf2Wrap_RtlAddFunctionTable( IN PRUNTIME_FUNCTION FunctionTable, IN DWORD EntryCount, IN DWORD64 BaseAddress )
{
    static FN_RtlAddFunctionTable *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlAddFunctionTable", &g_Kernel32);
    return pfn( FunctionTable, EntryCount, BaseAddress );
}

typedef BOOLEAN __cdecl FN_RtlInstallFunctionTableCallback( IN DWORD64 TableIdentifier, IN DWORD64 BaseAddress, IN DWORD Length, IN PGET_RUNTIME_FUNCTION_CALLBACK Callback, IN PVOID Context, IN PCWSTR OutOfProcessCallbackDll );
__declspec(dllexport) BOOLEAN __cdecl kPrf2Wrap_RtlInstallFunctionTableCallback( IN DWORD64 TableIdentifier, IN DWORD64 BaseAddress, IN DWORD Length, IN PGET_RUNTIME_FUNCTION_CALLBACK Callback, IN PVOID Context, IN PCWSTR OutOfProcessCallbackDll )
{
    static FN_RtlInstallFunctionTableCallback *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlInstallFunctionTableCallback", &g_Kernel32);
    return pfn( TableIdentifier, BaseAddress, Length, Callback, Context, OutOfProcessCallbackDll );
}

typedef BOOLEAN __cdecl FN_RtlDeleteFunctionTable( IN PRUNTIME_FUNCTION FunctionTable );
__declspec(dllexport) BOOLEAN __cdecl kPrf2Wrap_RtlDeleteFunctionTable( IN PRUNTIME_FUNCTION FunctionTable )
{
    static FN_RtlDeleteFunctionTable *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlDeleteFunctionTable", &g_Kernel32);
    return pfn( FunctionTable );
}

typedef VOID NTAPI FN_RtlInitializeSListHead( IN PSLIST_HEADER ListHead );
__declspec(dllexport) VOID NTAPI kPrf2Wrap_RtlInitializeSListHead( IN PSLIST_HEADER ListHead )
{
    static FN_RtlInitializeSListHead *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlInitializeSListHead", &g_Kernel32);
    pfn( ListHead );
}

typedef PSLIST_ENTRY NTAPI FN_RtlFirstEntrySList( IN const SLIST_HEADER * ListHead );
__declspec(dllexport) PSLIST_ENTRY NTAPI kPrf2Wrap_RtlFirstEntrySList( IN const SLIST_HEADER * ListHead )
{
    static FN_RtlFirstEntrySList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlFirstEntrySList", &g_Kernel32);
    return pfn( ListHead );
}

typedef PSLIST_ENTRY NTAPI FN_RtlInterlockedPopEntrySList( IN PSLIST_HEADER ListHead );
__declspec(dllexport) PSLIST_ENTRY NTAPI kPrf2Wrap_RtlInterlockedPopEntrySList( IN PSLIST_HEADER ListHead )
{
    static FN_RtlInterlockedPopEntrySList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlInterlockedPopEntrySList", &g_Kernel32);
    return pfn( ListHead );
}

typedef PSLIST_ENTRY NTAPI FN_RtlInterlockedPushEntrySList( IN PSLIST_HEADER ListHead, IN PSLIST_ENTRY ListEntry );
__declspec(dllexport) PSLIST_ENTRY NTAPI kPrf2Wrap_RtlInterlockedPushEntrySList( IN PSLIST_HEADER ListHead, IN PSLIST_ENTRY ListEntry )
{
    static FN_RtlInterlockedPushEntrySList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlInterlockedPushEntrySList", &g_Kernel32);
    return pfn( ListHead, ListEntry );
}

typedef PSLIST_ENTRY NTAPI FN_RtlInterlockedFlushSList( IN PSLIST_HEADER ListHead );
__declspec(dllexport) PSLIST_ENTRY NTAPI kPrf2Wrap_RtlInterlockedFlushSList( IN PSLIST_HEADER ListHead )
{
    static FN_RtlInterlockedFlushSList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlInterlockedFlushSList", &g_Kernel32);
    return pfn( ListHead );
}

typedef WORD NTAPI FN_RtlQueryDepthSList( IN PSLIST_HEADER ListHead );
__declspec(dllexport) WORD NTAPI kPrf2Wrap_RtlQueryDepthSList( IN PSLIST_HEADER ListHead )
{
    static FN_RtlQueryDepthSList *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlQueryDepthSList", &g_Kernel32);
    return pfn( ListHead );
}

typedef VOID NTAPI FN_RtlCaptureContext( OUT PCONTEXT ContextRecord );
__declspec(dllexport) VOID NTAPI kPrf2Wrap_RtlCaptureContext( OUT PCONTEXT ContextRecord )
{
    static FN_RtlCaptureContext *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlCaptureContext", &g_Kernel32);
    pfn( ContextRecord );
}

typedef SIZE_T NTAPI FN_RtlCompareMemory( const VOID * Source1, const VOID * Source2, SIZE_T Length );
__declspec(dllexport) SIZE_T NTAPI kPrf2Wrap_RtlCompareMemory( const VOID * Source1, const VOID * Source2, SIZE_T Length )
{
    static FN_RtlCompareMemory *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlCompareMemory", &g_Kernel32);
    return pfn( Source1, Source2, Length );
}

typedef ULONGLONG NTAPI FN_VerSetConditionMask( IN ULONGLONG ConditionMask, IN DWORD TypeMask, IN BYTE Condition );
__declspec(dllexport) ULONGLONG NTAPI kPrf2Wrap_VerSetConditionMask( IN ULONGLONG ConditionMask, IN DWORD TypeMask, IN BYTE Condition )
{
    static FN_VerSetConditionMask *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "VerSetConditionMask", &g_Kernel32);
    return pfn( ConditionMask, TypeMask, Condition );
}

typedef DWORD NTAPI FN_RtlSetHeapInformation( IN PVOID HeapHandle, IN HEAP_INFORMATION_CLASS HeapInformationClass, IN PVOID HeapInformation , IN SIZE_T HeapInformationLength );
__declspec(dllexport) DWORD NTAPI kPrf2Wrap_RtlSetHeapInformation( IN PVOID HeapHandle, IN HEAP_INFORMATION_CLASS HeapInformationClass, IN PVOID HeapInformation , IN SIZE_T HeapInformationLength )
{
    static FN_RtlSetHeapInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlSetHeapInformation", &g_Kernel32);
    return pfn( HeapHandle, HeapInformationClass, HeapInformation , HeapInformationLength );
}

typedef DWORD NTAPI FN_RtlQueryHeapInformation( IN PVOID HeapHandle, IN HEAP_INFORMATION_CLASS HeapInformationClass, OUT PVOID HeapInformation , IN SIZE_T HeapInformationLength , OUT PSIZE_T ReturnLength );
__declspec(dllexport) DWORD NTAPI kPrf2Wrap_RtlQueryHeapInformation( IN PVOID HeapHandle, IN HEAP_INFORMATION_CLASS HeapInformationClass, OUT PVOID HeapInformation , IN SIZE_T HeapInformationLength , OUT PSIZE_T ReturnLength )
{
    static FN_RtlQueryHeapInformation *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlQueryHeapInformation", &g_Kernel32);
    return pfn( HeapHandle, HeapInformationClass, HeapInformation , HeapInformationLength , ReturnLength );
}

typedef HANDLE WINAPI FN_CreateToolhelp32Snapshot( DWORD dwFlags, DWORD th32ProcessID );
__declspec(dllexport) HANDLE WINAPI kPrf2Wrap_CreateToolhelp32Snapshot( DWORD dwFlags, DWORD th32ProcessID )
{
    static FN_CreateToolhelp32Snapshot *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "CreateToolhelp32Snapshot", &g_Kernel32);
    return pfn( dwFlags, th32ProcessID );
}

typedef BOOL WINAPI FN_Heap32ListFirst( HANDLE hSnapshot, LPHEAPLIST32 lphl );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Heap32ListFirst( HANDLE hSnapshot, LPHEAPLIST32 lphl )
{
    static FN_Heap32ListFirst *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Heap32ListFirst", &g_Kernel32);
    return pfn( hSnapshot, lphl );
}

typedef BOOL WINAPI FN_Heap32ListNext( HANDLE hSnapshot, LPHEAPLIST32 lphl );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Heap32ListNext( HANDLE hSnapshot, LPHEAPLIST32 lphl )
{
    static FN_Heap32ListNext *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Heap32ListNext", &g_Kernel32);
    return pfn( hSnapshot, lphl );
}

typedef BOOL WINAPI FN_Heap32First( LPHEAPENTRY32 lphe, DWORD th32ProcessID, ULONG_PTR th32HeapID );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Heap32First( LPHEAPENTRY32 lphe, DWORD th32ProcessID, ULONG_PTR th32HeapID )
{
    static FN_Heap32First *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Heap32First", &g_Kernel32);
    return pfn( lphe, th32ProcessID, th32HeapID );
}

typedef BOOL WINAPI FN_Heap32Next( LPHEAPENTRY32 lphe );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Heap32Next( LPHEAPENTRY32 lphe )
{
    static FN_Heap32Next *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Heap32Next", &g_Kernel32);
    return pfn( lphe );
}

typedef BOOL WINAPI FN_Toolhelp32ReadProcessMemory( DWORD th32ProcessID, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T cbRead, SIZE_T * lpNumberOfBytesRead );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Toolhelp32ReadProcessMemory( DWORD th32ProcessID, LPCVOID lpBaseAddress, LPVOID lpBuffer, SIZE_T cbRead, SIZE_T * lpNumberOfBytesRead )
{
    static FN_Toolhelp32ReadProcessMemory *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Toolhelp32ReadProcessMemory", &g_Kernel32);
    return pfn( th32ProcessID, lpBaseAddress, lpBuffer, cbRead, lpNumberOfBytesRead );
}

typedef BOOL WINAPI FN_Process32FirstW( HANDLE hSnapshot, LPPROCESSENTRY32W lppe );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Process32FirstW( HANDLE hSnapshot, LPPROCESSENTRY32W lppe )
{
    static FN_Process32FirstW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Process32FirstW", &g_Kernel32);
    return pfn( hSnapshot, lppe );
}

typedef BOOL WINAPI FN_Process32NextW( HANDLE hSnapshot, LPPROCESSENTRY32W lppe );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Process32NextW( HANDLE hSnapshot, LPPROCESSENTRY32W lppe )
{
    static FN_Process32NextW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Process32NextW", &g_Kernel32);
    return pfn( hSnapshot, lppe );
}

typedef BOOL WINAPI FN_Process32First( HANDLE hSnapshot, LPPROCESSENTRY32 lppe );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Process32First( HANDLE hSnapshot, LPPROCESSENTRY32 lppe )
{
    static FN_Process32First *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Process32First", &g_Kernel32);
    return pfn( hSnapshot, lppe );
}

typedef BOOL WINAPI FN_Process32Next( HANDLE hSnapshot, LPPROCESSENTRY32 lppe );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Process32Next( HANDLE hSnapshot, LPPROCESSENTRY32 lppe )
{
    static FN_Process32Next *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Process32Next", &g_Kernel32);
    return pfn( hSnapshot, lppe );
}

typedef BOOL WINAPI FN_Thread32First( HANDLE hSnapshot, LPTHREADENTRY32 lpte );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Thread32First( HANDLE hSnapshot, LPTHREADENTRY32 lpte )
{
    static FN_Thread32First *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Thread32First", &g_Kernel32);
    return pfn( hSnapshot, lpte );
}

typedef BOOL WINAPI FN_Thread32Next( HANDLE hSnapshot, LPTHREADENTRY32 lpte );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Thread32Next( HANDLE hSnapshot, LPTHREADENTRY32 lpte )
{
    static FN_Thread32Next *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Thread32Next", &g_Kernel32);
    return pfn( hSnapshot, lpte );
}

typedef BOOL WINAPI FN_Module32FirstW( HANDLE hSnapshot, LPMODULEENTRY32W lpme );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Module32FirstW( HANDLE hSnapshot, LPMODULEENTRY32W lpme )
{
    static FN_Module32FirstW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Module32FirstW", &g_Kernel32);
    return pfn( hSnapshot, lpme );
}

typedef BOOL WINAPI FN_Module32NextW( HANDLE hSnapshot, LPMODULEENTRY32W lpme );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Module32NextW( HANDLE hSnapshot, LPMODULEENTRY32W lpme )
{
    static FN_Module32NextW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Module32NextW", &g_Kernel32);
    return pfn( hSnapshot, lpme );
}

typedef BOOL WINAPI FN_Module32First( HANDLE hSnapshot, LPMODULEENTRY32 lpme );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Module32First( HANDLE hSnapshot, LPMODULEENTRY32 lpme )
{
    static FN_Module32First *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Module32First", &g_Kernel32);
    return pfn( hSnapshot, lpme );
}

typedef BOOL WINAPI FN_Module32Next( HANDLE hSnapshot, LPMODULEENTRY32 lpme );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_Module32Next( HANDLE hSnapshot, LPMODULEENTRY32 lpme )
{
    static FN_Module32Next *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "Module32Next", &g_Kernel32);
    return pfn( hSnapshot, lpme );
}

typedef BOOL WINAPI FN_ReplaceFile( LPCSTR lpReplacedFileName, LPCSTR lpReplacementFileName, LPCSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_ReplaceFile( LPCSTR lpReplacedFileName, LPCSTR lpReplacementFileName, LPCSTR lpBackupFileName, DWORD dwReplaceFlags, LPVOID lpExclude, LPVOID lpReserved )
{
    static FN_ReplaceFile *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "ReplaceFile", &g_Kernel32);
    return pfn( lpReplacedFileName, lpReplacementFileName, lpBackupFileName, dwReplaceFlags, lpExclude, lpReserved );
}

typedef BOOL WINAPI FN_SetConsoleCursor( PVOID pvUnknown1, PVOID pvUnknown2 );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_SetConsoleCursor( PVOID pvUnknown1, PVOID pvUnknown2 )
{
    static FN_SetConsoleCursor *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "SetConsoleCursor", &g_Kernel32);
    return pfn( pvUnknown1, pvUnknown2 );
}

typedef LPCH WINAPI FN_GetEnvironmentStringsA( VOID );
__declspec(dllexport) LPCH WINAPI kPrf2Wrap_GetEnvironmentStringsA( VOID )
{
    static FN_GetEnvironmentStringsA *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetEnvironmentStringsA", &g_Kernel32);
    return pfn ();
}

typedef BOOL WINAPI FN_GetBinaryType( LPCSTR lpApplicationName, LPDWORD lpBinaryType );
__declspec(dllexport) BOOL WINAPI kPrf2Wrap_GetBinaryType( LPCSTR lpApplicationName, LPDWORD lpBinaryType )
{
    static FN_GetBinaryType *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "GetBinaryType", &g_Kernel32);
    return pfn( lpApplicationName, lpBinaryType );
}

typedef WORD NTAPI FN_RtlCaptureStackBackTrace( DWORD FramesToSkip, DWORD FramesToCapture, PVOID * BackTrace, PDWORD BackTraceHash );
__declspec(dllexport) WORD NTAPI kPrf2Wrap_RtlCaptureStackBackTrace( DWORD FramesToSkip, DWORD FramesToCapture, PVOID * BackTrace, PDWORD BackTraceHash )
{
    static FN_RtlCaptureStackBackTrace *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlCaptureStackBackTrace", &g_Kernel32);
    return pfn( FramesToSkip, FramesToCapture, BackTrace, BackTraceHash );
}

typedef PVOID FN_RtlFillMemory( PVOID pv, int ch, SIZE_T cb );
__declspec(dllexport) PVOID kPrf2Wrap_RtlFillMemory( PVOID pv, int ch, SIZE_T cb )
{
    static FN_RtlFillMemory *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlFillMemory", &g_Kernel32);
    return pfn( pv, ch, cb );
}

typedef PVOID FN_RtlZeroMemory( PVOID pv, SIZE_T cb );
__declspec(dllexport) PVOID kPrf2Wrap_RtlZeroMemory( PVOID pv, SIZE_T cb )
{
    static FN_RtlZeroMemory *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlZeroMemory", &g_Kernel32);
    return pfn( pv, cb );
}

typedef PVOID FN_RtlMoveMemory( PVOID pvDst, PVOID pvSrc, SIZE_T cb );
__declspec(dllexport) PVOID kPrf2Wrap_RtlMoveMemory( PVOID pvDst, PVOID pvSrc, SIZE_T cb )
{
    static FN_RtlMoveMemory *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlMoveMemory", &g_Kernel32);
    return pfn( pvDst, pvSrc, cb );
}

typedef VOID NTAPI FN_RtlUnwind( PVOID TargetFrame, PVOID TargetIp, PEXCEPTION_RECORD ExceptionRecord, PVOID ReturnValue );
__declspec(dllexport) VOID NTAPI kPrf2Wrap_RtlUnwind( PVOID TargetFrame, PVOID TargetIp, PEXCEPTION_RECORD ExceptionRecord, PVOID ReturnValue )
{
    static FN_RtlUnwind *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlUnwind", &g_Kernel32);
    pfn( TargetFrame, TargetIp, ExceptionRecord, ReturnValue );
}

typedef VOID NTAPI FN_RtlUnwindEx( FRAME_POINTERS TargetFrame, PVOID TargetIp, PEXCEPTION_RECORD ExceptionRecord, PVOID ReturnValue, PCONTEXT ContextRecord, PUNWIND_HISTORY_TABLE HistoryTable );
__declspec(dllexport) VOID NTAPI kPrf2Wrap_RtlUnwindEx( FRAME_POINTERS TargetFrame, PVOID TargetIp, PEXCEPTION_RECORD ExceptionRecord, PVOID ReturnValue, PCONTEXT ContextRecord, PUNWIND_HISTORY_TABLE HistoryTable )
{
    static FN_RtlUnwindEx *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlUnwindEx", &g_Kernel32);
    pfn( TargetFrame, TargetIp, ExceptionRecord, ReturnValue, ContextRecord, HistoryTable );
}

typedef ULONGLONG WINAPI FN_RtlVirtualUnwind( ULONG HandlerType, ULONGLONG ImageBase, ULONGLONG ControlPC, PRUNTIME_FUNCTION FunctionEntry, PCONTEXT ContextRecord, PBOOLEAN InFunction, PFRAME_POINTERS EstablisherFrame, PKNONVOLATILE_CONTEXT_POINTERS ContextPointers );
__declspec(dllexport) ULONGLONG WINAPI kPrf2Wrap_RtlVirtualUnwind( ULONG HandlerType, ULONGLONG ImageBase, ULONGLONG ControlPC, PRUNTIME_FUNCTION FunctionEntry, PCONTEXT ContextRecord, PBOOLEAN InFunction, PFRAME_POINTERS EstablisherFrame, PKNONVOLATILE_CONTEXT_POINTERS ContextPointers )
{
    static FN_RtlVirtualUnwind *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlVirtualUnwind", &g_Kernel32);
    return pfn( HandlerType, ImageBase, ControlPC, FunctionEntry, ContextRecord, InFunction, EstablisherFrame, ContextPointers );
}

typedef PVOID WINAPI FN_RtlPcToFileHeader( PVOID PcValue, PVOID * BaseOfImage );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_RtlPcToFileHeader( PVOID PcValue, PVOID * BaseOfImage )
{
    static FN_RtlPcToFileHeader *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlPcToFileHeader", &g_Kernel32);
    return pfn( PcValue, BaseOfImage );
}

typedef PVOID WINAPI FN_RtlLookupFunctionEntry( ULONGLONG ControlPC, PULONGLONG ImageBase, PULONGLONG TargetGp );
__declspec(dllexport) PVOID WINAPI kPrf2Wrap_RtlLookupFunctionEntry( ULONGLONG ControlPC, PULONGLONG ImageBase, PULONGLONG TargetGp )
{
    static FN_RtlLookupFunctionEntry *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlLookupFunctionEntry", &g_Kernel32);
    return pfn( ControlPC, ImageBase, TargetGp );
}

typedef void WINAPI FN_RtlRaiseException(PEXCEPTION_RECORD pXcpRec);
__declspec(dllexport) void WINAPI kPrf2Wrap_RtlRaiseException(PEXCEPTION_RECORD pXcpRec)
{
    static FN_RtlRaiseException *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "RtlRaiseException", &g_Kernel32);
    pfn( pXcpRec);
}

typedef int WINAPI FN_uaw_lstrcmpW( LPCUWSTR lpString1, LPCUWSTR lpString2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_uaw_lstrcmpW( LPCUWSTR lpString1, LPCUWSTR lpString2 )
{
    static FN_uaw_lstrcmpW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "uaw_lstrcmpW", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef int WINAPI FN_uaw_lstrcmpiW( LPCUWSTR lpString1, LPCUWSTR lpString2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_uaw_lstrcmpiW( LPCUWSTR lpString1, LPCUWSTR lpString2 )
{
    static FN_uaw_lstrcmpiW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "uaw_lstrcmpiW", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef int WINAPI FN_uaw_lstrlenW( LPCUWSTR lpString );
__declspec(dllexport) int WINAPI kPrf2Wrap_uaw_lstrlenW( LPCUWSTR lpString )
{
    static FN_uaw_lstrlenW *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "uaw_lstrlenW", &g_Kernel32);
    return pfn( lpString );
}

typedef LPUWSTR WINAPI FN_uaw_wcschr( LPCUWSTR lpString, WCHAR wc );
__declspec(dllexport) LPUWSTR WINAPI kPrf2Wrap_uaw_wcschr( LPCUWSTR lpString, WCHAR wc )
{
    static FN_uaw_wcschr *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "uaw_wcschr", &g_Kernel32);
    return pfn( lpString, wc );
}

typedef LPUWSTR WINAPI FN_uaw_wcscpy( LPUWSTR lpDst, LPCUWSTR lpSrc );
__declspec(dllexport) LPUWSTR WINAPI kPrf2Wrap_uaw_wcscpy( LPUWSTR lpDst, LPCUWSTR lpSrc )
{
    static FN_uaw_wcscpy *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "uaw_wcscpy", &g_Kernel32);
    return pfn( lpDst, lpSrc );
}

typedef int WINAPI FN_uaw_wcsicmp( LPCUWSTR lp1, LPCUWSTR lp2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_uaw_wcsicmp( LPCUWSTR lp1, LPCUWSTR lp2 )
{
    static FN_uaw_wcsicmp *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "uaw_wcsicmp", &g_Kernel32);
    return pfn( lp1, lp2 );
}

typedef SIZE_T WINAPI FN_uaw_wcslen( LPCUWSTR lp1 );
__declspec(dllexport) SIZE_T WINAPI kPrf2Wrap_uaw_wcslen( LPCUWSTR lp1 )
{
    static FN_uaw_wcslen *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "uaw_wcslen", &g_Kernel32);
    return pfn( lp1 );
}

typedef LPUWSTR WINAPI FN_uaw_wcsrchr( LPCUWSTR lpString, WCHAR wc );
__declspec(dllexport) LPUWSTR WINAPI kPrf2Wrap_uaw_wcsrchr( LPCUWSTR lpString, WCHAR wc )
{
    static FN_uaw_wcsrchr *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "uaw_wcsrchr", &g_Kernel32);
    return pfn( lpString, wc );
}

typedef LPSTR WINAPI FN_lstrcat( LPSTR lpString1, LPCSTR lpString2 );
__declspec(dllexport) LPSTR WINAPI kPrf2Wrap_lstrcat( LPSTR lpString1, LPCSTR lpString2 )
{
    static FN_lstrcat *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcat", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef int WINAPI FN_lstrcmp( LPCSTR lpString1, LPCSTR lpString2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_lstrcmp( LPCSTR lpString1, LPCSTR lpString2 )
{
    static FN_lstrcmp *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcmp", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef int WINAPI FN_lstrcmpi( LPCSTR lpString1, LPCSTR lpString2 );
__declspec(dllexport) int WINAPI kPrf2Wrap_lstrcmpi( LPCSTR lpString1, LPCSTR lpString2 )
{
    static FN_lstrcmpi *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcmpi", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef LPSTR WINAPI FN_lstrcpy( LPSTR lpString1, LPCSTR lpString2 );
__declspec(dllexport) LPSTR WINAPI kPrf2Wrap_lstrcpy( LPSTR lpString1, LPCSTR lpString2 )
{
    static FN_lstrcpy *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcpy", &g_Kernel32);
    return pfn( lpString1, lpString2 );
}

typedef LPSTR WINAPI FN_lstrcpyn( LPSTR lpString1, LPCSTR lpString2, int iMaxLength );
__declspec(dllexport) LPSTR WINAPI kPrf2Wrap_lstrcpyn( LPSTR lpString1, LPCSTR lpString2, int iMaxLength )
{
    static FN_lstrcpyn *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrcpyn", &g_Kernel32);
    return pfn( lpString1, lpString2, iMaxLength );
}

typedef int WINAPI FN_lstrlen( LPCSTR lpString );
__declspec(dllexport) int WINAPI kPrf2Wrap_lstrlen( LPCSTR lpString )
{
    static FN_lstrlen *pfn = 0;
    if (!pfn)
        kPrf2WrapResolve((void **)&pfn, "lstrlen", &g_Kernel32);
    return pfn( lpString );
}

