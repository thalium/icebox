/*
    MIT License

    Copyright (c) 2015 Nicolas Couffin ncouffin@gmail.com

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "GDB.h"
#include "FDP.h"
#include "utils.h"

#define GDB_DEBUG

#define GDB_RAX_REGISTER 0x00
#define GDB_RBX_REGISTER 0x01
#define GDB_RCX_REGISTER 0x02
#define GDB_RDX_REGISTER 0x03
#define GDB_RSI_REGISTER 0x04
#define GDB_RDI_REGISTER 0x05

#define GDB_RBP_REGISTER 0x06
#define GDB_RSP_REGISTER 0x07
#define GDB_R8_REGISTER     0x08
#define GDB_R9_REGISTER     0x09

#define GDB_RIP_REGISTER 0x0A
#define GDB_CS_REGISTER  0x0C
#define GDB_SS_REGISTER  0x0d
#define GDB_DS_REGISTER  0x0E 
#define GDB_ES_REGISTER  0x0F
#define GDB_FS_REGISTER  0x10
#define GDB_GS_REGISTER  0x11

#define GDB_EFLAGS_REGISTER 0x0B

#define GDB_BAD_REGISTER 0xFFFFFFFF

typedef struct GENERAL_REGISTERS64_T_{
    char Rax[16];
    char Rbx[16];
    char Rcx[16];
    char Rdx[16];
    char Rsi[16];
    char Rdi[16];
    char Rbp[16];
    char Rsp[16];
    char R16[16];
    char R9[16];
    char R10[16];
    char R11[16];
    char R12[16];
    char R13[16];
    char R14[16];
    char R15[16];
    char Rip[16];
    char Rflags[8];
    char Cs[8];
    char Ss[8];
    char Ds[8];
    char Es[8];
    char Fs[8];
    char Gs[8];
}GENERAL_REGISTERS64_T;

typedef struct GENERAL_REGISTERS32_T_ {
    char Rax[8];
    char Rcx[8];
    char Rdx[8];
    char Rbx[8];
    char Rsp[8];
    char Rbp[8];
    char Rsi[8];
    char Rdi[8];
    char Rip[8];
    char Rflags[8];
    char Cs[8];
    char Ss[8];
    char Ds[8];
    char Es[8];
    char Fs[8];
    char Gs[8];
}GENERAL_REGISTERS32_T;

void *g_GeneralRegisters;

#define READ_REGISTER32(RegisterName)                                                                            \
                GENERAL_REGISTERS32_T *pGeneralRegisters32 = (GENERAL_REGISTERS32_T *)g_GeneralRegisters;        \
                char Temp[sizeof(pGeneralRegisters32->RegisterName) + 1];                                        \
                memcpy(Temp, pGeneralRegisters32->RegisterName, sizeof(pGeneralRegisters32->RegisterName));        \
                Temp[sizeof(pGeneralRegisters32->RegisterName)] = 0;                                            \
                *pRegisterValue = RegisterStrToU64(Temp);

#define READ_REGISTER64(RegisterName)                                                                            \
                GENERAL_REGISTERS64_T *pGeneralRegisters64 = (GENERAL_REGISTERS64_T *)g_GeneralRegisters;        \
                char Temp[sizeof(pGeneralRegisters64->RegisterName)+1];                                            \
                memcpy(Temp, pGeneralRegisters64->RegisterName, sizeof(pGeneralRegisters64->RegisterName));        \
                Temp[sizeof(pGeneralRegisters64->RegisterName)] = 0;                                            \
                *pRegisterValue = RegisterStrToU64(Temp);

#define READ_REGISTER(RegisterName) {\
                if(pGDB->bIsX86 == true) {                                                                            \
                        READ_REGISTER32(RegisterName)                                                                \
                }else{                                                                                                \
                        READ_REGISTER64(RegisterName)                                                                \
                }                                                                                                    \
}

uint64_t Hex2U64(char *pHex){
    return strtoull(pHex, NULL, 16);
}

uint32_t FDPRegisterToGDBRegister(FDP_Register RegisterId)
{
    uint32_t RegisterNumber = 0;
    switch (RegisterId){
    case FDP_RAX_REGISTER:        RegisterNumber = GDB_RAX_REGISTER; break;
    case FDP_RBX_REGISTER:        RegisterNumber = GDB_RBX_REGISTER; break;
    case FDP_RCX_REGISTER:        RegisterNumber = GDB_RCX_REGISTER; break;
    case FDP_RDX_REGISTER:        RegisterNumber = GDB_RDX_REGISTER; break;
    case FDP_RSI_REGISTER:        RegisterNumber = GDB_RSI_REGISTER; break;
    case FDP_RDI_REGISTER:        RegisterNumber = GDB_RDI_REGISTER; break;
    case FDP_RSP_REGISTER:        RegisterNumber = GDB_RSP_REGISTER; break;
    case FDP_RBP_REGISTER:        RegisterNumber = GDB_RBP_REGISTER; break;
    case FDP_R8_REGISTER:        RegisterNumber = GDB_R8_REGISTER; break;
    case FDP_R9_REGISTER:        RegisterNumber = GDB_R9_REGISTER; break;
    case FDP_RIP_REGISTER:        RegisterNumber = GDB_RIP_REGISTER; break;
    case FDP_CS_REGISTER:        RegisterNumber = GDB_CS_REGISTER; break;
    case FDP_DS_REGISTER:        RegisterNumber = GDB_DS_REGISTER; break;
    case FDP_ES_REGISTER:        RegisterNumber = GDB_ES_REGISTER; break;
    case FDP_FS_REGISTER:        RegisterNumber = GDB_FS_REGISTER; break;
    case FDP_SS_REGISTER:        RegisterNumber = GDB_SS_REGISTER; break;
    case FDP_GS_REGISTER:        RegisterNumber = GDB_GS_REGISTER; break;
    case FDP_RFLAGS_REGISTER:    RegisterNumber = GDB_EFLAGS_REGISTER; break;
    default:                    RegisterNumber = GDB_BAD_REGISTER; break;
    }
    return RegisterNumber;
}

GDB_TYPE_T* GDB_Open(char *pArgument)
{
    GDB_TYPE_T* pGDB = NULL;
    WSADATA wsa;
    SOCKET sSocket;
    char *pPort = NULL;
    USHORT uPort = 0;
    char *pHostPort;
    pHostPort = malloc(strlen(pArgument)+1);
    strcpy(pHostPort, pArgument);

    if (pHostPort == NULL) {
        return NULL;
    }

    //Parse argument
    pPort = strchr(pHostPort, ':');
    if (pPort == NULL) {
        return NULL;
    }
    pPort[0] = 0;
    pPort++;
    uPort = atoi(pPort);


    char *pHostName = pHostPort;

    pGDB = (GDB_TYPE_T*)malloc(sizeof(GDB_TYPE_T));
    if (pGDB == NULL) {
        return NULL;
    }

    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err < 0) {
        puts("WSAStartup failed !");
        return NULL;
    }

    sSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (sSocket == INVALID_SOCKET) {
        perror("socket()");
        return NULL;
    }

    struct hostent *pHostInfo = NULL;
    SOCKADDR_IN sin = { 0 };


    pHostInfo = gethostbyname(pHostName);
    if (pHostInfo == NULL) {
        printf("Failed to gethostbyname (%s)\n", pHostName);
        return NULL;
    }

    sin.sin_addr = *(IN_ADDR *)pHostInfo->h_addr;
    sin.sin_port = htons(uPort);
    sin.sin_family = AF_INET;

    if (connect(sSocket, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR) {
        printf("Failed to connect\n");
        return NULL;
    }

    pGDB->Sock = sSocket;

    g_GeneralRegisters = malloc(4096); //TODO: compute size 
    if (g_GeneralRegisters == NULL) {
        return NULL;
    }

    //Never remove that for VMware !, BSOD due to instruction cache and code injection
    GDB_SingleStep(pGDB);

    return pGDB;
}

bool SendGDBPacket(SOCKET Sock, char *pData)
{
    size_t szDataLen = strlen(pData);
    uint32_t uChecksum = 0;
    for (size_t i = 0; i < szDataLen; i++){
        uChecksum += pData[i];
    }
    uChecksum = uChecksum % 256;

    char Command[1024];
    sprintf_s(Command, sizeof(Command), "$%s#%02x", pData, uChecksum);

#ifdef GDB_DEBUG
    printf("PUT %s\n", Command);
#endif

    if (send(Sock, Command, (int)strlen(Command), 0) < 0){
        printf("Failed to send\n");
        return false;
    }

    //Wait for Ack
    char cAck;
    if (recv(Sock, &cAck, 1, 0) != 1){
        printf("Failed to recv\n");
        return false;
    }

    if (cAck != '+'){
        printf("Not a Ack (%c)!\n", cAck);
        return false;
    }
    return true;
}

bool RecvGDBPacket(SOCKET Sock, char *pCommand)
{
    //char Command[1024];
    int i = 0;
    char cCurrentChar;
    char cChecksum1;
    char cChecksum2;

    //Read first char
    if (recv(Sock, &cCurrentChar, 1, 0) != 1) {
        printf("Failed to recv\n");
        return false;
    }

    //Check first $
    if (cCurrentChar != '$') {
        printf("Not a start (%c)\n", cCurrentChar);
        //Buggy GDB VMWare...
        //return false;
    }

    while (cCurrentChar != '#'){
        if (recv(Sock, &cCurrentChar, 1, 0) != 1){
            printf("Failed to recv\n");
            return false;
        }
        pCommand[i] = cCurrentChar;
        i++;
    }

    //Read Checksum
    if (recv(Sock, &cChecksum1, 1, 0) != 1){
        printf("Failed to recv\n");
        return false;
    }
    if (recv(Sock, &cChecksum2, 1, 0) != 1){
        printf("Failed to recv\n");
        return false;
    }

    //Send Ack
    if (send(Sock, "+", 1, 0) < 0){
        printf("Failed to send\n");
        return false;
    }

    pCommand[i - 1] = 0;
#ifdef GDB_DEBUG
    printf("GET $%s#%c%c\n", pCommand, cChecksum1, cChecksum2);
#endif
    return true;
}

void ReverseRegisterStr(char *pRegisterStr)
{
    size_t iLen = strlen(pRegisterStr);
    for (size_t i = 0; i < iLen/2; i++){
        char cTemp = pRegisterStr[i];
        pRegisterStr[i] = pRegisterStr[(iLen-1) - i];
        pRegisterStr[(iLen - 1) - i] = cTemp;
    }
}

void SwapRegisterStr(char *pRegisterStr)
{
    for (size_t i = 0; i < strlen(pRegisterStr)/2; i++){
        char cTemp = pRegisterStr[(i * 2)];
        pRegisterStr[(i * 2)] = pRegisterStr[(i * 2) + 1];
        pRegisterStr[(i * 2) + 1] = cTemp;
    }
}

uint64_t RegisterStrToU64(char *pRegisterStr)
{
    char TempBuffer[1024];
    strcpy_s(TempBuffer, sizeof(TempBuffer), pRegisterStr);
    //Swap
    SwapRegisterStr(TempBuffer);
    //Reverse
    ReverseRegisterStr(TempBuffer);

    return Hex2U64(TempBuffer);
}

bool GDB_ReadRegisterInternal(GDB_TYPE_T *pGDB, uint32_t CpuId, uint32_t RegisterNumber, uint64_t *pRegisterValue){
    //Construction command
    //$p0#FF
    char Command[1024];
    sprintf_s(Command, sizeof(Command), "p%d", RegisterNumber);
    SendGDBPacket(pGDB->Sock, Command);
    //Receive 
    RecvGDBPacket(pGDB->Sock, Command);

    *pRegisterValue = RegisterStrToU64(Command);
    return false;
}

bool GDB_WriteRegisterInternal(GDB_TYPE_T *pGDB, uint32_t CpuId, uint32_t RegisterNumber, uint64_t RegisterValue)
{
    char RegisterStr[17];
    char Command[1024];

    sprintf_s(RegisterStr, sizeof(RegisterStr), "%llx", RegisterValue);
    ReverseRegisterStr(RegisterStr);
    SwapRegisterStr(RegisterStr);
    sprintf_s(Command, sizeof(Command), "P%d=%s", RegisterNumber, RegisterStr);

    //Send
    SendGDBPacket(pGDB->Sock, Command);
    //Receive 
    RecvGDBPacket(pGDB->Sock, Command);

    return true;
}

bool GDB_RefreshGeneralRegister(GDB_TYPE_T *pGDB) {
    //Get General Registers
    SendGDBPacket(pGDB->Sock, "g");
    RecvGDBPacket(pGDB->Sock, (char*)g_GeneralRegisters);
    return 0;
}

bool GDB_ReadX(GDB_TYPE_T *pGDB, uint64_t *pRegisterValue, uint8_t *pMovRaxXInst, uint8_t MovRaxXInstSize)
{
    uint64_t    OriginalRipValue;
    uint64_t    OriginalRaxValue;
    uint8_t        OriginalInstructions[10];

    *pRegisterValue = 0;

    GDB_ReadRegisterInternal(pGDB, 0, GDB_RIP_REGISTER, &OriginalRipValue);

    GDB_ReadRegisterInternal(pGDB, 0, GDB_RAX_REGISTER, &OriginalRaxValue);

    GDB_ReadVirtualMemory(pGDB, 0, OriginalInstructions, MovRaxXInstSize, OriginalRipValue);

    //Inject Shellcode
    GDB_WriteVirtualMemory(pGDB, 0, pMovRaxXInst, MovRaxXInstSize, OriginalRipValue);

    //SingleStep
    GDB_SingleStep(pGDB);

    GDB_ReadRegisterInternal(pGDB, 0, GDB_RAX_REGISTER, pRegisterValue);

    //Restore Orignal instruction
    GDB_WriteVirtualMemory(pGDB, 0, OriginalInstructions, MovRaxXInstSize, OriginalRipValue);

    //Restore Original Rax, Rip
    GDB_WriteRegisterInternal(pGDB, 0, GDB_RAX_REGISTER, OriginalRaxValue);
    GDB_WriteRegisterInternal(pGDB, 0, GDB_RIP_REGISTER, OriginalRipValue);

    return true;
}

bool GDB_ReadR10(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER64(R10);

    return true;
}

bool GDB_ReadR11(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER64(R11);

    return true;
}

bool GDB_ReadR12(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER64(R12);

    return true;
}

bool GDB_ReadR13(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER64(R13);

    return true;
}

bool GDB_ReadR14(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER64(R14);

    return true;
}

bool GDB_ReadR15(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER64(R15);

    return true;
}


//TODO: Check Ring !!!
bool GDB_ReadCr0(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    uint8_t MovRaxR10[] = { 0x0F, 0x20, 0xC0 };
    return GDB_ReadX(pGDB, pRegisterValue, MovRaxR10, sizeof(MovRaxR10));
}

bool GDB_ReadCr2(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    uint8_t MovRaxR10[] = { 0x0F, 0x20, 0xD0 };
    return GDB_ReadX(pGDB, pRegisterValue, MovRaxR10, sizeof(MovRaxR10));
}

bool GDB_ReadCr4(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    uint8_t MovRaxR10[] = { 0x0F, 0x20, 0xE0 };
    return GDB_ReadX(pGDB, pRegisterValue, MovRaxR10, sizeof(MovRaxR10));
}

//Dont read Cr8, Cr8 read => VM freeze
bool GDB_ReadCr8(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    *pRegisterValue = 0xDEADDEADDEADDEAD;
    return true;
}



bool GDB_ReadRax(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rax);

    return true;
}

bool GDB_ReadRbx(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rbx);

    return true;
}

bool GDB_ReadRcx(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rcx);

    return true;
}

bool GDB_ReadRdx(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rdx);

    return true;
}

bool GDB_ReadRsp(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rsp);

    return true;
}

bool GDB_ReadRbp(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rbp);

    return true;
}

bool GDB_ReadRip(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rip);

    return true;
}

bool GDB_ReadRsi(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rsi);

    return true;
}

bool GDB_ReadRdi(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rdi);

    return true;
}

bool GDB_ReadCs(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Cs);

    return true;
}

bool GDB_ReadSs(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Ss);

    return true;
}
bool GDB_ReadGs(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Gs);

    return true;
}
bool GDB_ReadFs(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Fs);

    return true;
}

bool GDB_ReadDs(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Ds);

    return true;
}

bool GDB_ReadEs(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Es);

    return true;
}

bool GDB_ReadRflags(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    READ_REGISTER(Rflags);

    return true;
}

bool GDB_ReadRegister(GDB_TYPE_T *pGDB, uint32_t CpuId, FDP_Register RegisterId, uint64_t *pRegisterValue)
{
    //Special case for unavailable register
    switch (RegisterId){
    case FDP_RAX_REGISTER: return GDB_ReadRax(pGDB, CpuId, pRegisterValue);
    case FDP_RBX_REGISTER: return GDB_ReadRbx(pGDB, CpuId, pRegisterValue);
    case FDP_RCX_REGISTER: return GDB_ReadRcx(pGDB, CpuId, pRegisterValue);
    case FDP_RDX_REGISTER: return GDB_ReadRdx(pGDB, CpuId, pRegisterValue);
    case FDP_RSP_REGISTER: return GDB_ReadRsp(pGDB, CpuId, pRegisterValue);
    case FDP_RBP_REGISTER: return GDB_ReadRbp(pGDB, CpuId, pRegisterValue);
    case FDP_RSI_REGISTER: return GDB_ReadRsi(pGDB, CpuId, pRegisterValue);
    case FDP_RDI_REGISTER: return GDB_ReadRdi(pGDB, CpuId, pRegisterValue);
    case FDP_RIP_REGISTER: return GDB_ReadRip(pGDB, CpuId, pRegisterValue);

    case FDP_CS_REGISTER: return GDB_ReadCs(pGDB, CpuId, pRegisterValue);
    case FDP_DS_REGISTER: return GDB_ReadDs(pGDB, CpuId, pRegisterValue);
    case FDP_ES_REGISTER: return GDB_ReadEs(pGDB, CpuId, pRegisterValue);
    case FDP_FS_REGISTER: return GDB_ReadFs(pGDB, CpuId, pRegisterValue);
    case FDP_SS_REGISTER: return GDB_ReadSs(pGDB, CpuId, pRegisterValue);
    case FDP_GS_REGISTER: return GDB_ReadGs(pGDB, CpuId, pRegisterValue);

    case FDP_RFLAGS_REGISTER: return GDB_ReadRflags(pGDB, CpuId, pRegisterValue);

    //x64 only
    case FDP_R10_REGISTER: return GDB_ReadR10(pGDB, CpuId, pRegisterValue);
    case FDP_R11_REGISTER: return GDB_ReadR11(pGDB, CpuId, pRegisterValue);
    case FDP_R12_REGISTER: return GDB_ReadR12(pGDB, CpuId, pRegisterValue);
    case FDP_R13_REGISTER: return GDB_ReadR13(pGDB, CpuId, pRegisterValue);
    case FDP_R14_REGISTER: return GDB_ReadR14(pGDB, CpuId, pRegisterValue);
    case FDP_R15_REGISTER: return GDB_ReadR15(pGDB, CpuId, pRegisterValue);
    //TODO: Check ring and avoid code injection 
    /*case FDP_CR0_REGISTER: return GDB_ReadCr0(pGDB, CpuId, pRegisterValue);
    case FDP_CR2_REGISTER: return GDB_ReadCr2(pGDB, CpuId, pRegisterValue);
    case FDP_CR3_REGISTER: return GDB_ReadCr3(pGDB, CpuId, pRegisterValue);
    case FDP_CR4_REGISTER: return GDB_ReadCr4(pGDB, CpuId, pRegisterValue);
    case FDP_CR8_REGISTER: return GDB_ReadCr8(pGDB, CpuId, pRegisterValue);*/
    //TODO !
    case FDP_GDTRB_REGISTER: *pRegisterValue = pGDB->Gdtrb; return true;
    case FDP_GDTRL_REGISTER: *pRegisterValue = 0xffff; return true;
    case FDP_IDTRB_REGISTER: *pRegisterValue = pGDB->Idtrb; return true;
    case FDP_IDTRL_REGISTER: *pRegisterValue = 0xff; return true;
    }

    uint32_t RegisterNumber = FDPRegisterToGDBRegister(RegisterId);
    if (RegisterNumber != GDB_BAD_REGISTER){
        return GDB_ReadRegisterInternal(pGDB, CpuId, RegisterNumber, pRegisterValue);
    }
    else{
        *pRegisterValue = 0xBAD0BAD0BAD0BAD0;
        return false;
    }
}

bool GDB_WriteRegister(GDB_TYPE_T *pGDB, uint32_t CpuId, FDP_Register RegisterId, uint64_t RegisterValue)
{
    uint32_t RegisterNumber = FDPRegisterToGDBRegister(RegisterId);
    //TODO: more register !
    if (RegisterNumber < GDB_RIP_REGISTER){
        return GDB_WriteRegisterInternal(pGDB, CpuId, RegisterNumber, RegisterValue);
    }
    else{
        return false;
    }
}

bool GDB_SingleStep(GDB_TYPE_T *pGDB)
{
    char Command[1024];
    //$s#FF
    SendGDBPacket(pGDB->Sock, "s");
    //Receive 
    RecvGDBPacket(pGDB->Sock, Command);

    GDB_RefreshGeneralRegister(pGDB);
    return true;
}

bool ReadVirtualMemoryChunk(GDB_TYPE_T *pGDB, uint64_t VirtualAddress, uint16_t ReadLength, uint8_t *pDstBuffer)
{
    char Command[1024];
    sprintf_s(Command, sizeof(Command), "m%llx,%04x", VirtualAddress, ReadLength);

    //Send
    SendGDBPacket(pGDB->Sock, Command);
    //Receive 
    RecvGDBPacket(pGDB->Sock, Command);

    if (strcmp(Command, "E00") == 0){
        return false;
    }

    //Convert to binary
    for (int i = 0; i < ReadLength; i++){
        char Temp[3];
        Temp[0] = Command[(i * 2)];
        Temp[1] = Command[(i * 2) + 1];
        Temp[2] = 0;
        pDstBuffer[i] = (Hex2U64(Temp) & 0xFF);
    }
    return true;
}

bool GDB_ReadVirtualMemory(GDB_TYPE_T* pGDB, uint32_t CpuId, uint8_t *pDstBuffer, uint16_t ReadLength, uint64_t VirtualAddress)
{
    uint16_t ReadDone = 0;
    do{
        uint16_t ChunkSize = MIN(ReadLength, 0x1FF);
        if (ReadVirtualMemoryChunk(pGDB, VirtualAddress + ReadDone, ChunkSize, pDstBuffer + ReadDone) == false){
            return false;
        }
        ReadDone += ChunkSize;
    } while (ReadDone < ReadLength);
    return true;
}

bool GDB_WriteVirtualMemory(GDB_TYPE_T *pGDB, uint32_t CpuId, uint8_t *pSrcBuffer, uint16_t WriteLength, uint64_t VirtualAddress)
{
    char Command[1024];
    sprintf_s(Command, sizeof(Command), "M%llx,%04x:", VirtualAddress, WriteLength);
    for (int i = 0; i < WriteLength; i++){
        sprintf_s(Command, sizeof(Command), "%s%02x", Command, pSrcBuffer[i]);
    }

    //Send
    SendGDBPacket(pGDB->Sock, Command);
    //Receive 
    RecvGDBPacket(pGDB->Sock, Command);

    return true;
}

bool GDB_ReadMsr(GDB_TYPE_T *pGDB, uint32_t CpuId, uint32_t MsrId, uint64_t *pMsrValue)
{
    uint64_t    TempValue;
    uint64_t    OriginalRipValue;
    uint64_t    OriginalRaxValue;
    uint64_t    OriginalRcxValue;
    uint64_t    OriginalRdxValue;
    uint8_t        RdmsrOpcode[] = { 0x0F, 0x32 };
    uint8_t        OriginalInstructions[sizeof(RdmsrOpcode)];

    *pMsrValue = 0;

    GDB_ReadRegisterInternal(pGDB, 0, GDB_RIP_REGISTER, &OriginalRipValue);
    GDB_ReadRegisterInternal(pGDB, 0, GDB_RAX_REGISTER, &OriginalRaxValue);
    GDB_ReadRegisterInternal(pGDB, 0, GDB_RCX_REGISTER, &OriginalRcxValue);
    GDB_ReadRegisterInternal(pGDB, 0, GDB_RDX_REGISTER, &OriginalRdxValue);

    //Check kernel
    if (OriginalRipValue < 0xFFFFF00000000000){
        return false;
    }

    GDB_ReadVirtualMemory(pGDB, 0, OriginalInstructions, sizeof(RdmsrOpcode), OriginalRipValue);

    GDB_WriteRegisterInternal(pGDB, 0, GDB_RCX_REGISTER, MsrId);
    GDB_WriteVirtualMemory(pGDB, 0, RdmsrOpcode, sizeof(RdmsrOpcode), OriginalRipValue);
    GDB_SingleStep(pGDB);

    GDB_ReadRegisterInternal(pGDB, 0, GDB_RDX_REGISTER, &TempValue);
    *pMsrValue = TempValue << 32;
    GDB_ReadRegisterInternal(pGDB, 0, GDB_RAX_REGISTER, &TempValue);
    *pMsrValue = *pMsrValue | TempValue;

    //Restore instructions
    GDB_WriteVirtualMemory(pGDB, 0, OriginalInstructions, sizeof(RdmsrOpcode), OriginalRipValue);

    //Restore registers
    GDB_WriteRegisterInternal(pGDB, 0, GDB_RAX_REGISTER, OriginalRaxValue);
    GDB_WriteRegisterInternal(pGDB, 0, GDB_RCX_REGISTER, OriginalRcxValue);
    GDB_WriteRegisterInternal(pGDB, 0, GDB_RDX_REGISTER, OriginalRdxValue);
    GDB_WriteRegisterInternal(pGDB, 0, GDB_RIP_REGISTER, OriginalRipValue);

    return true;
}

//TODO: Use ReadX
bool GDB_ReadCr3(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pCr3Value)
{
    uint64_t    OriginalRipValue;
    uint64_t    OriginalRaxValue;
    uint8_t        MovRaxCr3Opcode[] = { 0x0F, 0x20, 0xD8 };
    uint8_t        OriginalInstructions[sizeof(MovRaxCr3Opcode)];

    *pCr3Value = 0;

    GDB_ReadRegisterInternal(pGDB, 0, GDB_RIP_REGISTER, &OriginalRipValue);
    //Check kernel
    if (OriginalRipValue < 0xFFFFF00000000000){
        return false;
    }
    GDB_ReadRegisterInternal(pGDB, 0, GDB_RAX_REGISTER, &OriginalRaxValue);

    GDB_ReadVirtualMemory(pGDB, 0, OriginalInstructions, sizeof(MovRaxCr3Opcode), OriginalRipValue);

    GDB_WriteVirtualMemory(pGDB, 0, MovRaxCr3Opcode, sizeof(MovRaxCr3Opcode), OriginalRipValue);
    GDB_SingleStep(pGDB);

    GDB_ReadRegisterInternal(pGDB, 0, GDB_RAX_REGISTER, pCr3Value);

    //Restore Orignal instruction
    GDB_WriteVirtualMemory(pGDB, 0, OriginalInstructions, sizeof(MovRaxCr3Opcode), OriginalRipValue);

    //Restore Original Rax, Rip
    GDB_WriteRegisterInternal(pGDB, 0, GDB_RAX_REGISTER, OriginalRaxValue);
    GDB_WriteRegisterInternal(pGDB, 0, GDB_RIP_REGISTER, OriginalRipValue);

    return true;
}


#define KPCR_SELF_OFFSET_X64           0x18
bool GDB_ReadKPCR(GDB_TYPE_T *pGDB, uint32_t CpuId, uint64_t *pRegisterValue)
{
    uint8_t MovRaxGs0x18[] = { 0X65, 0x48, 0x8B, 0x04, 0x25, KPCR_SELF_OFFSET_X64, 0x00, 0x00, 0x00 };
    return GDB_ReadX(pGDB, pRegisterValue, MovRaxGs0x18, sizeof(MovRaxGs0x18));
}



#define KPCR_IDTRB_OFF          0x38
bool GDB_Init(GDB_TYPE_T *pGDB)
{
    pGDB->bInContinue = false;
    pGDB->HardwareBreakpoint[0] = 0;
    pGDB->HardwareBreakpoint[1] = 0;
    pGDB->HardwareBreakpoint[2] = 0;
    pGDB->HardwareBreakpoint[4] = 0;

    pGDB->bIsX86 = false;
    if (strlen(g_GeneralRegisters) == 344) { //Tricky for VMware 328=>x64  344=>x86
        //pGDB->bIsX86 = true;
    }

    uint32_t KPCR_GDBTB_OFF = 0;
    //Read KPCR
    if (pGDB->bIsX86 == true) {
        KPCR_GDBTB_OFF = 0x3c;
        //TODO GDB_ReadKPCRx86
        pGDB->Kpcr = 0x82971c00; //Vmw
    }
    else {
        KPCR_GDBTB_OFF = 0x00;
        if (GDB_ReadKPCR(pGDB, 0, &pGDB->Kpcr) == false) {
            return false;
        }
    }

    if (GDB_ReadVirtualMemory(pGDB, 0, (uint8_t*)&pGDB->Idtrb, sizeof(pGDB->Idtrb), pGDB->Kpcr + KPCR_IDTRB_OFF) == false) {
        return false;
    }
    if (pGDB->bIsX86) {
        pGDB->Idtrb = pGDB->Idtrb & 0xFFFFFFFF;
    }

    if (GDB_ReadVirtualMemory(pGDB, 0, (uint8_t*)&pGDB->Gdtrb, sizeof(pGDB->Idtrb), pGDB->Kpcr + KPCR_GDBTB_OFF) == false) {
        return false;
    }
    if (pGDB->bIsX86) {
        pGDB->Gdtrb = pGDB->Gdtrb & 0xFFFFFFFF;
    }

    return true;
}

bool GDB_GetPhysicalMemorySize(GDB_TYPE_T* pGDB, uint64_t *pPhysicalMemorySize)
{
    pPhysicalMemorySize = 0;
    return true;
}

bool GDB_Pause(GDB_TYPE_T* pGDB)
{
    if (pGDB->bInContinue == true){
        char Command[1024];
        const char BreakCmd[] = { 0x03 };
        if (send(pGDB->Sock, BreakCmd, sizeof(BreakCmd), 0) < 0){
            printf("Failed to send\n");
            return false;
        }
        char cAck;
        if (recv(pGDB->Sock, &cAck, 1, 0) != 1){
            printf("Failed to recv\n");
            return false;
        }

        //Receive 
        if (RecvGDBPacket(pGDB->Sock, Command) == false){
            printf("Failed to RecvGDBPacket\n");
            return false;
        }
        //TODO: Avoid BSOD, Do not use tricky shitty code injection !
        GDB_SingleStep(pGDB);

        pGDB->bInContinue = false;
    }
    return true;
}

bool GDB_GetFxState64(GDB_TYPE_T* pGDB, uint32_t CpuId, void *pFxState64)
{
    return true;
}

bool GDB_SetFxState64(GDB_TYPE_T* pGDB, uint32_t CpuId, void *pFxState64)
{
    return true;
}

bool GDB_WriteRegisterDummy(GDB_TYPE_T *pGDB, uint32_t CpuId, FDP_Register RegisterId, uint64_t RegisterValue)
{
    return true;
}

bool GDB_ReadPhysicalMemory(GDB_TYPE_T *pGDB, void *pDstBuffer, uint32_t ReadSize, uint64_t PhysicalAddress)
{
    return false;
}

bool GDB_Resume(GDB_TYPE_T *pGDB)
{
    SendGDBPacket(pGDB->Sock, "c");
    pGDB->bInContinue = true;
    return true;
}

bool GDB_GetCpuState(GDB_TYPE_T *pGDB, uint32_t CpuId, FDP_State *pDebuggeeState)
{
    *pDebuggeeState = (FDP_State)0;
    return true;
}

bool GDB_UnsetBreakpoint(GDB_TYPE_T* pGDB, int iBreakpointId)
{
    char Command[1024];
    if (iBreakpointId <= 4){
        sprintf_s(Command, sizeof(Command), "z0,%llx,1", pGDB->HardwareBreakpoint[iBreakpointId]);
        //Send
        SendGDBPacket(pGDB->Sock, Command);
        //Receive 
        RecvGDBPacket(pGDB->Sock, Command);
        if (Command[0] == 'O' && Command[1] == 'K'){
            pGDB->HardwareBreakpoint[iBreakpointId] = 0x0;
            return true;
        }
    }
    return true;
}

int  GDB_SetBreakpoint(GDB_TYPE_T *pGDB, uint32_t CpuId, FDP_BreakpointType BreakpointType, uint8_t BreakpointId, FDP_Access BreakpointAccessType, FDP_AddressType BreakpointAddressType, uint64_t BreakpointAddress, uint64_t BreakpointLength)
{
    char Command[1024];
    int BreakpointIndex;
    for (BreakpointIndex = 0; BreakpointIndex < 4; BreakpointIndex++){
        if (pGDB->HardwareBreakpoint[BreakpointIndex] == 0){
            break;
        }
    }
    if (pGDB->HardwareBreakpoint[BreakpointIndex] != 0){
        return -1;
    }
    sprintf_s(Command, sizeof(Command), "Z0,%llx,1", BreakpointAddress);
    //Send
    SendGDBPacket(pGDB->Sock, Command);
    //Receive 
    RecvGDBPacket(pGDB->Sock, Command);
    if (Command[0] = 'O' && Command[1] == 'K'){
        pGDB->HardwareBreakpoint[BreakpointIndex] = BreakpointAddress;
        return BreakpointIndex;
    }
    //Fail
    return -1;
}

bool GDB_GetStateChanged(GDB_TYPE_T *pGDB){
    FD_SET ReadSet;
    FD_ZERO(&ReadSet);
    FD_SET(pGDB->Sock, &ReadSet);
    TIMEVAL tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10;
    int iSelectReturn = select(0, &ReadSet, 0, 0, &tv);
    //if something on socket, the guest hitted a breakpoint
    if (iSelectReturn == 1){
        char Command[1024];
        //Dont care about the receivced commande
        RecvGDBPacket(pGDB->Sock, Command);
        printf("StateChanged !\n");
        pGDB->bInContinue = false;
        GDB_RefreshGeneralRegister(pGDB);
        return true;
    }
    return false;
}



//----------------------------------TEST----------------------------------------//
bool GDB_Test(GDB_TYPE_T *pGDB)
{
    return true;
}