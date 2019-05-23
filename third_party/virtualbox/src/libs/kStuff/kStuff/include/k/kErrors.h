/* $Id: kErrors.h 58 2013-10-12 20:18:21Z bird $ */
/** @file
 * kErrors - Status Codes.
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

#ifndef ___k_kErrors_h___
#define ___k_kErrors_h___

/** @defgroup grp_kErrors   Status Codes.
 * @{
 */
/** The base of the kErrors status codes. */
#define KERR_BASE                                       42000

/** @name General
 * @{
 */
/** The base of the general status codes. */
#define KERR_GENERAL_BASE                               (KERR_BASE)
/** Generic error. */
#define KERR_GENERAL_FAILURE                            (KERR_GENERAL_BASE + 1)
/** Out of memory. */
#define KERR_NO_MEMORY                                  (KERR_GENERAL_BASE + 2)
/** Hit some unimplemented functionality - feel free to implement it :-) . */
#define KERR_NOT_IMPLEMENTED                            (KERR_GENERAL_BASE + 3)
/** An environment variable wasn't found. */
#define KERR_ENVVAR_NOT_FOUND                           (KERR_GENERAL_BASE + 4)
/** Buffer overflow. */
#define KERR_BUFFER_OVERFLOW                            (KERR_GENERAL_BASE + 5)
/** @}*/

/** @name Input Validation
 * @{
 */
/** The base of the input validation status codes. */
#define KERR_INPUT_BASE                                 (KERR_GENERAL_BASE + 6)
/** An API was given an invalid parameter. */
#define KERR_INVALID_PARAMETER                          (KERR_INPUT_BASE + 0)
/** A pointer argument is not valid. */
#define KERR_INVALID_POINTER                            (KERR_INPUT_BASE + 1)
/** A handle argument is not valid. */
#define KERR_INVALID_HANDLE                             (KERR_INPUT_BASE + 2)
/** An offset argument is not valid. */
#define KERR_INVALID_OFFSET                             (KERR_INPUT_BASE + 3)
/** A size argument is not valid. */
#define KERR_INVALID_SIZE                               (KERR_INPUT_BASE + 4)
/** A range argument is not valid. */
#define KERR_INVALID_RANGE                              (KERR_INPUT_BASE + 5)
/** A parameter is out of range. */
#define KERR_OUT_OF_RANGE                               (KERR_INPUT_BASE + 6)
/** @} */

/** @name File System and I/O
 * @{
 */
/** The base of the file system and I/O status cdoes. */
#define KERR_FILE_SYSTEM_AND_IO_BASE                    (KERR_INPUT_BASE + 7)
/** The specified file was not found. */
#define KERR_FILE_NOT_FOUND                             (KERR_FILE_SYSTEM_AND_IO_BASE + 0)
/** End of file. */
#define KERR_EOF                                        (KERR_FILE_SYSTEM_AND_IO_BASE + 1)
/** @} */

/** @name   kDbg Specific
 * @{
 */
/** The base of the kDbg specific status codes. */
#define KDBG_ERR_BASE                                   (KERR_FILE_SYSTEM_AND_IO_BASE + 2)
/** The (module) format isn't known to use. */
#define KDBG_ERR_UNKOWN_FORMAT                          (KDBG_ERR_BASE + 0)
/** The (module) format isn't supported by this kDbg build. */
#define KDBG_ERR_FORMAT_NOT_SUPPORTED                   (KDBG_ERR_BASE + 1)
/** The (module) format isn't supported by this kDbg build. */
#define KDBG_ERR_BAD_EXE_FORMAT                         (KDBG_ERR_BASE + 2)
/** A specified address or an address found in the debug info is invalid. */
#define KDBG_ERR_INVALID_ADDRESS                        (KDBG_ERR_BASE + 3)
/** The dbghelp.dll is too old or something like that. */
#define KDBG_ERR_DBGHLP_VERSION_MISMATCH                (KDBG_ERR_BASE + 4)
/** @} */

/** @name   kRdr Specific
 * @{
 */
/** the base of the kRdr specific status codes. */
#define KRDR_ERR_BASE                                   (KDBG_ERR_BASE + 5)
/** The file reader can't take more concurrent mappings. */
#define KRDR_ERR_TOO_MANY_MAPPINGS                      (KRDR_ERR_BASE + 0)
/** The pRdr instance passed to a kRdrBuf* API isn't a buffered instance. */
#define KRDR_ERR_NOT_BUFFERED_RDR                       (KRDR_ERR_BASE + 1)
/** The line is too long to fit in the buffer passed to kRdrBufLine or kRdrBufLineEx. */
#define KRDR_ERR_LINE_TOO_LONG                          (KRDR_ERR_BASE + 2)
/** @} */

/** @name   kLdr Specific
 * @{
 */
/** The base of the kLdr specific status codes. */
#define KLDR_ERR_BASE                                   (KRDR_ERR_BASE + 3)

/** The image format is unknown. */
#define KLDR_ERR_UNKNOWN_FORMAT                         (KLDR_ERR_BASE + 0)
/** The MZ image format isn't supported by this kLdr build. */
#define KLDR_ERR_MZ_NOT_SUPPORTED                       (KLDR_ERR_BASE + 1)
/** The NE image format isn't supported by this kLdr build. */
#define KLDR_ERR_NE_NOT_SUPPORTED                       (KLDR_ERR_BASE + 2)
/** The LX image format isn't supported by this kLdr build. */
#define KLDR_ERR_LX_NOT_SUPPORTED                       (KLDR_ERR_BASE + 3)
/** The LE image format isn't supported by this kLdr build. */
#define KLDR_ERR_LE_NOT_SUPPORTED                       (KLDR_ERR_BASE + 4)
/** The PE image format isn't supported by this kLdr build. */
#define KLDR_ERR_PE_NOT_SUPPORTED                       (KLDR_ERR_BASE + 5)
/** The ELF image format isn't supported by this kLdr build. */
#define KLDR_ERR_ELF_NOT_SUPPORTED                      (KLDR_ERR_BASE + 6)
/** The mach-o image format isn't supported by this kLdr build. */
#define KLDR_ERR_MACHO_NOT_SUPPORTED                    (KLDR_ERR_BASE + 7)
/** The FAT image format isn't supported by this kLdr build or
 * a direct open was attempt without going thru the FAT file provider.
 * FAT images are also known as Universal Binaries. */
#define KLDR_ERR_FAT_NOT_SUPPORTED                      (KLDR_ERR_BASE + 8)
/** The a.out image format isn't supported by this kLdr build. */
#define KLDR_ERR_AOUT_NOT_SUPPORTED                     (KLDR_ERR_BASE + 9)

/** The module wasn't loaded dynamically. */
#define KLDR_ERR_NOT_LOADED_DYNAMICALLY                 (KLDR_ERR_BASE + 10)
/** The module wasn't found. */
#define KLDR_ERR_MODULE_NOT_FOUND                       (KLDR_ERR_BASE + 11)
/** A prerequisit module wasn't found. */
#define KLDR_ERR_PREREQUISITE_MODULE_NOT_FOUND          (KLDR_ERR_BASE + 12)
/** The module is being terminated and can therefore not be loaded. */
#define KLDR_ERR_MODULE_TERMINATING                     (KLDR_ERR_BASE + 13)
/** A prerequisit module is being terminated and can therefore not be loaded. */
#define KLDR_ERR_PREREQUISITE_MODULE_TERMINATING        (KLDR_ERR_BASE + 14)
/** The module initialization failed. */
#define KLDR_ERR_MODULE_INIT_FAILED                     (KLDR_ERR_BASE + 15)
/** The initialization of a prerequisite module failed. */
#define KLDR_ERR_PREREQUISITE_MODULE_INIT_FAILED        (KLDR_ERR_BASE + 16)
/** The module has already failed initialization and can't be attempted reloaded until
 * after we've finished garbage collection. */
#define KLDR_ERR_MODULE_INIT_FAILED_ALREADY             (KLDR_ERR_BASE + 17)
/** A prerequisite module has already failed initialization and can't be attempted
 * reloaded until after we've finished garbage collection. */
#define KLDR_ERR_PREREQUISITE_MODULE_INIT_FAILED_ALREADY (KLDR_ERR_BASE + 18)
/** Prerequisite recursed too deeply. */
#define KLDR_ERR_PREREQUISITE_RECURSED_TOO_DEEPLY       (KLDR_ERR_BASE + 19)
/** Failed to allocate the main stack. */
#define KLDR_ERR_MAIN_STACK_ALLOC_FAILED                (KLDR_ERR_BASE + 20)
/** Symbol not found. */
#define KLDR_ERR_SYMBOL_NOT_FOUND                       (KLDR_ERR_BASE + 21)
/** A forward symbol was encountered but the caller didn't provide any means to resolve it. */
#define KLDR_ERR_FORWARDER_SYMBOL                       (KLDR_ERR_BASE + 22)
/** Encountered a bad fixup. */
#define KLDR_ERR_BAD_FIXUP                              (KLDR_ERR_BASE + 23)
/** The import ordinal was out of bounds. */
#define KLDR_ERR_IMPORT_ORDINAL_OUT_OF_BOUNDS           (KLDR_ERR_BASE + 24)
/** A forwarder chain was too long. */
#define KLDR_ERR_TOO_LONG_FORWARDER_CHAIN               (KLDR_ERR_BASE + 25)
/** The module has no debug info. */
#define KLDR_ERR_NO_DEBUG_INFO                          (KLDR_ERR_BASE + 26)
/** The module is already mapped.
 * kLdrModMap() can only be called once (without kLdrModUnmap() in between). */
#define KLDR_ERR_ALREADY_MAPPED                         (KLDR_ERR_BASE + 27)
/** The module was not mapped.
 * kLdrModUnmap() should not called without being preceeded by a kLdrModMap(). */
#define KLDR_ERR_NOT_MAPPED                             (KLDR_ERR_BASE + 28)
/** Couldn't fit the address value into the field. Typically a relocation kind of error. */
#define KLDR_ERR_ADDRESS_OVERFLOW                       (KLDR_ERR_BASE + 29)
/** Couldn't fit a calculated size value into the native size type of the host. */
#define KLDR_ERR_SIZE_OVERFLOW                          (KLDR_ERR_BASE + 30)
/** Thread attach failed. */
#define KLDR_ERR_THREAD_ATTACH_FAILED                   (KLDR_ERR_BASE + 31)
/** The module wasn't a DLL or object file. */
#define KLDR_ERR_NOT_DLL                                (KLDR_ERR_BASE + 32)
/** The module wasn't an EXE. */
#define KLDR_ERR_NOT_EXE                                (KLDR_ERR_BASE + 33)
/** Not implemented yet. */
#define KLDR_ERR_TODO                                   (KLDR_ERR_BASE + 34)
/** No image matching the requested CPU. */
#define KLDR_ERR_CPU_ARCH_MISMATCH                      (KLDR_ERR_BASE + 35)
/** Invalid FAT image header. */
#define KLDR_ERR_FAT_INVALID                            (KLDR_ERR_BASE + 36)
/** Unsupported CPU subtype found in a FAT entry. */
#define KLDR_ERR_FAT_UNSUPPORTED_CPU_SUBTYPE            (KLDR_ERR_BASE + 37)
/** The image has no UUID. */
#define KLDR_ERR_NO_IMAGE_UUID                          (KLDR_ERR_BASE + 38)
/** Duplicate segment name. */
#define KLDR_ERR_DUPLICATE_SEGMENT_NAME                 (KLDR_ERR_BASE + 39)
/** @} */

/** @name kLdrModPE Specific
 * @{
 */
/** The base of the kLdrModPE specific status codes. */
#define KLDR_ERR_PE_BASE                                (KLDR_ERR_BASE + 40)
/** The machine isn't supported by the interpreter. */
#define KLDR_ERR_PE_UNSUPPORTED_MACHINE                 (KLDR_ERR_PE_BASE + 0)
/** The file handler isn't valid. */
#define KLDR_ERR_PE_BAD_FILE_HEADER                     (KLDR_ERR_PE_BASE + 1)
/** The the optional headers isn't valid. */
#define KLDR_ERR_PE_BAD_OPTIONAL_HEADER                 (KLDR_ERR_PE_BASE + 2)
/** One of the section headers aren't valid. */
#define KLDR_ERR_PE_BAD_SECTION_HEADER                  (KLDR_ERR_PE_BASE + 3)
/** Bad forwarder entry. */
#define KLDR_ERR_PE_BAD_FORWARDER                       (KLDR_ERR_PE_BASE + 4)
/** Forwarder module not found in the import descriptor table. */
#define KLDR_ERR_PE_FORWARDER_IMPORT_NOT_FOUND          (KLDR_ERR_PE_BASE + 5)
/** Bad PE fixups. */
#define KLDR_ERR_PE_BAD_FIXUP                           (KLDR_ERR_PE_BASE + 6)
/** Bad PE import (thunk). */
#define KLDR_ERR_PE_BAD_IMPORT                          (KLDR_ERR_PE_BASE + 7)
/** @} */

/** @name kLdrModLX Specific
 * @{
 */
/** The base of the kLdrModLX specific status codes. */
#define KLDR_ERR_LX_BASE                                (KLDR_ERR_PE_BASE + 8)
/** validation of LX header failed. */
#define KLDR_ERR_LX_BAD_HEADER                          (KLDR_ERR_LX_BASE + 0)
/** validation of the loader section (in the LX header) failed. */
#define KLDR_ERR_LX_BAD_LOADER_SECTION                  (KLDR_ERR_LX_BASE + 1)
/** validation of the fixup section (in the LX header) failed. */
#define KLDR_ERR_LX_BAD_FIXUP_SECTION                   (KLDR_ERR_LX_BASE + 2)
/** validation of the LX object table failed. */
#define KLDR_ERR_LX_BAD_OBJECT_TABLE                    (KLDR_ERR_LX_BASE + 3)
/** A bad page map entry was encountered. */
#define KLDR_ERR_LX_BAD_PAGE_MAP                        (KLDR_ERR_LX_BASE + 4)
/** Bad iterdata (EXEPACK) data. */
#define KLDR_ERR_LX_BAD_ITERDATA                        (KLDR_ERR_LX_BASE + 5)
/** Bad iterdata2 (EXEPACK2) data. */
#define KLDR_ERR_LX_BAD_ITERDATA2                       (KLDR_ERR_LX_BASE + 6)
/** Bad bundle data. */
#define KLDR_ERR_LX_BAD_BUNDLE                          (KLDR_ERR_LX_BASE + 7)
/** No soname. */
#define KLDR_ERR_LX_NO_SONAME                           (KLDR_ERR_LX_BASE + 8)
/** Bad soname. */
#define KLDR_ERR_LX_BAD_SONAME                          (KLDR_ERR_LX_BASE + 9)
/** Bad forwarder entry. */
#define KLDR_ERR_LX_BAD_FORWARDER                       (KLDR_ERR_LX_BASE + 10)
/** internal fixup chain isn't implemented yet. */
#define KLDR_ERR_LX_NRICHAIN_NOT_SUPPORTED              (KLDR_ERR_LX_BASE + 11)
/** @} */

/** @name kLdrModMachO Specific
 * @{
 */
/** The base of the kLdrModMachO specific status codes. */
#define KLDR_ERR_MACHO_BASE                             (KLDR_ERR_LX_BASE + 12)
/** Only native endian Mach-O files are supported. */
#define KLDR_ERR_MACHO_OTHER_ENDIAN_NOT_SUPPORTED       (KLDR_ERR_MACHO_BASE + 0)
/** The Mach-O header is bad or contains new and unsupported features. */
#define KLDR_ERR_MACHO_BAD_HEADER                       (KLDR_ERR_MACHO_BASE + 1)
/** The file type isn't supported. */
#define KLDR_ERR_MACHO_UNSUPPORTED_FILE_TYPE            (KLDR_ERR_MACHO_BASE + 2)
/** The machine (cputype / cpusubtype combination) isn't supported. */
#define KLDR_ERR_MACHO_UNSUPPORTED_MACHINE              (KLDR_ERR_MACHO_BASE + 3)
/** Bad load command(s). */
#define KLDR_ERR_MACHO_BAD_LOAD_COMMAND                 (KLDR_ERR_MACHO_BASE + 4)
/** Encountered an unknown load command.*/
#define KLDR_ERR_MACHO_UNKNOWN_LOAD_COMMAND             (KLDR_ERR_MACHO_BASE + 5)
/** Encountered a load command that's not implemented.*/
#define KLDR_ERR_MACHO_UNSUPPORTED_LOAD_COMMAND         (KLDR_ERR_MACHO_BASE + 6)
/** Bad section. */
#define KLDR_ERR_MACHO_BAD_SECTION                      (KLDR_ERR_MACHO_BASE + 7)
/** Encountered a section type that's not implemented.*/
#define KLDR_ERR_MACHO_UNSUPPORTED_SECTION              (KLDR_ERR_MACHO_BASE + 8)
/** Encountered a init function section.   */
#define KLDR_ERR_MACHO_UNSUPPORTED_INIT_SECTION         (KLDR_ERR_MACHO_BASE + 9)
/** Encountered a term function section.   */
#define KLDR_ERR_MACHO_UNSUPPORTED_TERM_SECTION         (KLDR_ERR_MACHO_BASE + 10)
/** Encountered a section type that's not known to the loader. (probably invalid) */
#define KLDR_ERR_MACHO_UNKNOWN_SECTION                  (KLDR_ERR_MACHO_BASE + 11)
/** The sections aren't ordered by segment as expected by the loader. */
#define KLDR_ERR_MACHO_BAD_SECTION_ORDER                (KLDR_ERR_MACHO_BASE + 12)
/** The image is 32-bit and contains 64-bit load commands or vise versa. */
#define KLDR_ERR_MACHO_BIT_MIX                          (KLDR_ERR_MACHO_BASE + 13)
/** Bad MH_OBJECT file. */
#define KLDR_ERR_MACHO_BAD_OBJECT_FILE                  (KLDR_ERR_MACHO_BASE + 14)
/** Bad symbol table entry. */
#define KLDR_ERR_MACHO_BAD_SYMBOL                       (KLDR_ERR_MACHO_BASE + 15)
/** Unsupported fixup type. */
#define KLDR_ERR_MACHO_UNSUPPORTED_FIXUP_TYPE           (KLDR_ERR_MACHO_BASE + 16)
/** Both debug and non-debug sections in segment. */
#define KLDR_ERR_MACHO_MIXED_DEBUG_SECTION_FLAGS        (KLDR_ERR_MACHO_BASE + 17)
/** The segment bits are non-contiguous in the file. */
#define KLDR_ERR_MACHO_NON_CONT_SEG_BITS                (KLDR_ERR_MACHO_BASE + 18)
/** @} */

/** @name kCpu Specific
 * @{
 */
/** The base of the kCpu specific status codes. */
#define KCPU_ERR_BASE                                   (KLDR_ERR_MACHO_BASE + 19)
/** The specified ARCH+CPU pairs aren't compatible. */
#define KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE                (KCPU_ERR_BASE + 0)
/** @} */

/** End of the valid status codes. */
#define KERR_END                                        (KCPU_ERR_BASE + 1)
/** @}*/

#endif

