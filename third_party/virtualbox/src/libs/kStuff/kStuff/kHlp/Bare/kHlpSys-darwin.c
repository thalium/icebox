/* $Id: kHlpSys-darwin.c 29 2009-07-01 20:30:29Z bird $ */
/** @file
 * kHlpBare -
 */

/*
 * Copyright (c) 2006-2007 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <k/kHlpSys.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <mach/mach_time.h>


#define USE_DARWIN_SYSCALLS

#if K_ARCH == K_ARCH_X86_32
# define DARWIN_SYSCALL(name, code) \
    asm("\
    .text                                   \n\
    .globl _" #name "                       \n\
    _" #name ":                             \n\
        mov     $ " #code ", %eax           \n\
        call    1f                          \n\
    1:                                      \n\
        pop     %edx                        \n\
        mov     %esp, %ecx                  \n\
        sysenter                            \n\
        jnae    2f                          \n\
        ret                                 \n\
    2:                                      \n\
        neg     %eax                        \n\
        ret                                 \n\
    ")

# define DARWIN_SYSCALL_RET64(name, code) \
    asm("\
    .text                                   \n\
    .globl _" #name "                       \n\
    _" #name ":                             \n\
        mov     $ " #code ", %eax           \n\
        int     $0x80                       \n\
        jnae    2f                          \n\
        ret                                 \n\
    2:                                      \n\
        neg     %eax                        \n\
        mov     $0xffffffff, %edx           \n\
        ret                                 \n\
    ")

# define DARWIN_SYSCALL_NOERR(name, code) \
    asm("\
    .text                                   \n\
    .globl _" #name "                       \n\
    _" #name ":                             \n\
        mov     $ " #code ", %eax           \n\
        call    1f                          \n\
    1:                                      \n\
        pop     %edx                        \n\
        mov     %esp, %ecx                  \n\
        sysenter                            \n\
        ret                                 \n\
    ")

#elif K_ARCH == K_ARCH_AMD64
# define DARWIN_SYSCALL(name, code) \
    asm("\
    .text                                   \n\
    .globl _" #name "                       \n\
    _" #name ":                             \n\
        mov     $ " #code ", %eax           \n\
        mov     %rcx, %r10                  \n\
        sysenter                            \n\
        jnae    2f                          \n\
        ret                                 \n\
    2:                                      \n\
        neg     %eax                        \n\
        movsx   %eax, %rax                  \n\
        ret                                 \n\
    ")

# define DARWIN_SYSCALL_RET64(name, code) DARWIN_SYSCALL_RET(name, code)

# define DARWIN_SYSCALL_NOERR(name, code) \
    asm("\
    .text                                   \n\
    .globl _" #name "                       \n\
    _" #name ":                             \n\
        mov     $ " #code ", %eax           \n\
        mov     %rcx, %r10                  \n\
        sysenter                            \n\
        ret                                 \n\
    ")


#else
# error later...
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_readlink, 0x000c003a);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_readlink, 0x0200003a);
#else
KSSIZE      kHlpSys_readlink(const char *pszPath, char *pszBuf, KSIZE cbBuf)
{
    KSSIZE cbRet = readlink(pszPath, pszBuf, cbBuf);
    return cbRet >= 0 ? cbRet : -errno;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_open, 0x000c0005);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_open, 0x02000005);
#else
int         kHlpSys_open(const char *filename, int flags, int mode)
{
    int fd = open(filename, flags, mode);
    return fd >= 0 ? fd : -errno;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_close, 0x000c0006);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_close, 0x02000006);
#else
int         kHlpSys_close(int fd)
{
   if (!close(fd))
       return 0;
   return -errno;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL_RET64(kHlpSys_lseek, 0x000000c7);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL_RET64(kHlpSys_lseek, 0x020000c7);
#else
KFOFF       kHlpSys_lseek(int fd, int whench, KFOFF off)
{
    KFOFF offRet = lseek(fd, whench, off);
    return offRet >= 0 ? offRet : -errno;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_read, 0x000c0003);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_read, 0x02000003);
#else
KSSIZE      kHlpSys_read(int fd, void *pvBuf, KSIZE cbBuf)
{
    KSSIZE cbRead = read(fd, pvBuf, cbBuf);
    return cbRead >= 0 ? cbRead : -errno;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_write, 0x000c0004);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_write, 0x02000004);
#else
KSSIZE      kHlpSys_write(int fd, const void *pvBuf, KSIZE cbBuf)
{
    KSSIZE cbWritten = write(fd, pvBuf, cbBuf);
    return cbWritten >= 0 ? cbWritten : -errno;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_mmap, 0x020000c5);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_mmap, 0x020000c5);
#else
void       *kHlpSys_mmap(void *addr, KSIZE len, int prot, int flags, int fd, KI64 off)
{
    void *pv = mmap(addr, len, prot, flags, fd, off);
    return pv != (void *)-1
        ? pv
        : errno < 256 ? (void *)(long)errno : (void *)(long)ENOMEM;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_mprotect, 0x000c004a);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_mprotect, 0x0200004a);
#else
int         kHlpSys_mprotect(void *addr, KSIZE len, int prot)
{
    if (!mprotect(addr, len, prot))
        return 0;
    return -errno;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_munmap, 0x00080049);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_munmap, 0x02000049);
#else
int         kHlpSys_munmap(void *addr, KSIZE len)
{
    if (!munmap(addr, len))
        return 0;
    return -errno;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_exit, 0x00040001);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(kHlpSys_exit, 0x02000001);
#else
void        kHlpSys_exit(int rc)
{
    _Exit(rc);
}
#endif


/*
 * Some other stuff we'll be needing - Move to an appropriate place?
 */

#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL_NOERR(mach_task_self, 0xffffffe4);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL_NOERR(mach_task_self, 0xffffffe4);
#endif

//#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
//DARWIN_SYSCALL(semaphore_create, 0x00040001);
//#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
//DARWIN_SYSCALL(semaphore_create, 0x02000001);
//#endif
#ifdef USE_DARWIN_SYSCALLS
kern_return_t semaphore_create(task_t t, semaphore_t *ps, int p, int v)
{
    return 0;
}
#endif

//#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
//DARWIN_SYSCALL(semaphore_destroy, 0x00040001);
//#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
//DARWIN_SYSCALL(semaphore_destroy, 0x02000001);
//#endif
#ifdef USE_DARWIN_SYSCALLS
kern_return_t semaphore_destroy(task_t t, semaphore_t s)
{
    return 0;
}
#endif


#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(semaphore_wait, 0xffffffdc);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(semaphore_wait, 0xffffffdc);
#endif

#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(semaphore_signal, 0xffffffdf);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(semaphore_signal, 0xffffffdf);
#endif

#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(mach_wait_until, 0xffffffa6);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(mach_wait_until, 0xffffffa6);
#endif

#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(mach_timebase_info, 0xffffffa7);
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
DARWIN_SYSCALL(mach_timebase_info, 0xffffffa7);
#endif

#if K_ARCH == K_ARCH_X86_32 && defined(USE_DARWIN_SYSCALLS)
asm("\n\
.text                           \n\
.globl _mach_absolute_time      \n\
_mach_absolute_time:            \n\
    mov     $0xffff1700, %edx   \n\
    jmp     *%edx\n"); /* common page stuff. */
#elif K_ARCH == K_ARCH_AMD64 && defined(USE_DARWIN_SYSCALLS)
#endif


void *dlopen(const char *pszModule, int fFlags)
{
    return NULL;
}


int dlclose(void *pvMod)
{

}


void *dlsym(void *pvMod, const char *pszSymbol)
{
    return NULL;
}

