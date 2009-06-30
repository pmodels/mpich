/* -*- Mode: C++; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include <windows.h>
#include <stdio.h>
#include <time.h>

/* This function returns a pointer to the section header containing the RVA. Return NULL on error */
template <typename NTHeaderType> static inline PIMAGE_SECTION_HEADER GetSecHeader(DWORD rva, NTHeaderType* pNTHeader)
{
    PIMAGE_SECTION_HEADER section;
    section = IMAGE_FIRST_SECTION(pNTHeader);
    for (int i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++){
		DWORD size = section->Misc.VirtualSize;
        DWORD secRVA = section->VirtualAddress;
        if((secRVA <= rva)  && (rva < (secRVA+size))){
            return section;
        }
    }
    return NULL;
}

/* This function returns the real address from the RVA. Returns NULL on error */
template <typename NTHeaderType> static inline LPVOID GetRealAddrFromRVA(DWORD rva, NTHeaderType* pNTHeader, PBYTE imageBase )
{
	PIMAGE_SECTION_HEADER pSectionHdr = GetSecHeader(rva, pNTHeader);
	if (!pSectionHdr){
		return NULL;
    }
    /* Since we map the DLL manually we need to calculate mapShift to get the real addr */
	INT mapShift = (INT)(pSectionHdr->VirtualAddress - pSectionHdr->PointerToRawData);

	return (PVOID)(imageBase + rva - mapShift);
}

/* This function reads the exported symbols from a dll/exe and prints a DLL definition file, compatible with gcc,
   with those symbols.
    INPUT:
        pImageBase  : Pointer to the base of the dll image, offset = 0
        pNTHeader   : Pointer to the NT header (32-bit/64-bit) section of image.
*/
template <typename NTHeaderType> void PrintDefFileWithExpSymbols(PBYTE pImageBase, NTHeaderType *pNTHeader)
{
    /* Export Section RVA */
    DWORD expSecRVA = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    /* From RVA get the real addrs for the various components of the exports section */
    PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY )GetRealAddrFromRVA(expSecRVA, pNTHeader, pImageBase);
    PDWORD pszFuncNames =	(PDWORD ) GetRealAddrFromRVA( pExportDir->AddressOfNames, pNTHeader, pImageBase );

    printf("EXPORTS\n");

    for (unsigned int i=0; i < pExportDir->NumberOfFunctions; i++){
        /* FIXME: Assumes that the current DLL does not have ordinal nums */
        if(i < pExportDir->NumberOfNames){
            /* Ordinal numbers start at 1 - not 0*/
            printf("\t%s @ %d ;\n", GetRealAddrFromRVA(pszFuncNames[i], pNTHeader, pImageBase), i+1);
        }
        else{
            break;
        }
    }
}

static void HelpUsage(char *exe_name)
{
    printf("Usage: %s <DLLNAME>\n", exe_name);
    printf("\t\t The tool reads the export section from <DLLNAME> and prints the definition file for gcc to stdout\n");
}

int main(int argc, char *argv[])
{
    HANDLE hFile, hFileMapping;
    LPVOID lpFileBase;
    PIMAGE_DOS_HEADER   pImgHeader;
    PIMAGE_NT_HEADERS   pImgNTHeader;
    PIMAGE_NT_HEADERS64 pImgNTHeader64;

    if((argc < 2) || (strcmp(argv[1], "/?") == 0) || (strcmp(argv[1], "--help") == 0)){
        HelpUsage(argv[0]);
        exit(1);
    }
    
    /* Open the DLL and map it to memory */
    hFile = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if(hFile == INVALID_HANDLE_VALUE){
        printf("Unable to open dll, %s\n", argv[1]);
        exit(-1);
    }
    
    hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if(hFileMapping == 0){
        CloseHandle(hFile);
        printf("Cannot open file mapping for dll, %s\n", argv[1]);
        exit(-1);
    }

    lpFileBase = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
    if(lpFileBase == 0){
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        printf("Cannot map view of dll, %s\n", argv[1]);
        exit(-1);
    }

    /* pImgHeader points to the MS-DOS HEADER section, offset=0, of dll */
    pImgHeader = (PIMAGE_DOS_HEADER )lpFileBase;
    
    /* pImgNTHeader points to the NT HEADER section of the dll, pImgHeader + <File address of new exe header> */
    pImgNTHeader = (PIMAGE_NT_HEADERS )((DWORD_PTR )pImgHeader + (DWORD_PTR )pImgHeader->e_lfanew);

    /* In the case of the 64-bit dlls, the Optional header section within NT header has 64-bit fields */
    pImgNTHeader64 = (PIMAGE_NT_HEADERS64) pImgNTHeader;

    /* The Magic number is always a WORD. This number is used to determine if the dll is 32-bit or 64-bit */
    if(pImgNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC){
        /* 64-bit dll */
        PrintDefFileWithExpSymbols((PBYTE )pImgHeader, pImgNTHeader64);
    }
    else{
        /* 32-bit dll */
        PrintDefFileWithExpSymbols((PBYTE )pImgHeader, pImgNTHeader);
    }

    UnmapViewOfFile(lpFileBase);
    CloseHandle(hFileMapping);
    CloseHandle(hFile);
}

