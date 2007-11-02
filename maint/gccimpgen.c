/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*  
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */
#include <stdio.h>
#include <windows.h>

unsigned int read16(HANDLE fd, int offset)
{
    DWORD num_read;
    unsigned char b[2];

    SetFilePointer(fd, offset, NULL, FILE_BEGIN);
    if (!ReadFile(fd, b, 2, &num_read, NULL))
    {
	ExitProcess(-1);
    }
    return b[0] + (b[1] << 8);
}

unsigned int read32(HANDLE fd, int offset)
{
    DWORD num_read;
    unsigned char b[4];

    SetFilePointer(fd, offset, NULL, FILE_BEGIN);
    if (!ReadFile(fd, b, 4, &num_read, NULL))
    {
	ExitProcess(-1);
    }
    return b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24);
}

unsigned int to32(void *p)
{
    unsigned char *b = p;
    return b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24);
}

int main(int argc, char *argv[])
{
    DWORD num_read;
    HANDLE fd;
    unsigned long pe_header_offset, opthdr_ofs, num_entries, i;
    unsigned long export_rva, export_size, nsections, secptr, expptr;
    unsigned long name_rvas, nexp;
    unsigned char *expdata, *erva;
    unsigned long name_rva;
    unsigned long secptr1;
    unsigned long vaddr;
    unsigned long vsize;
    unsigned long fptr;
    char sname[8];

    if (argc < 2)
    {
	printf("usage: %s <filename>.dll\n", argv[0]);
	return 0;
    }

    fd = CreateFile(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (fd == INVALID_HANDLE_VALUE)
    {
	return 1;
    }

    pe_header_offset = read32(fd, 0x3c);
    opthdr_ofs = pe_header_offset + 4 + 20;
    num_entries = read32(fd, opthdr_ofs + 92);

    if (num_entries < 1)
    {
	/* no exports */
	CloseHandle(fd);
	return 1;
    }

    export_rva = read32(fd, opthdr_ofs + 96);
    export_size = read32(fd, opthdr_ofs + 100);
    nsections = read16(fd, pe_header_offset + 4 + 2);
    secptr = (pe_header_offset + 4 + 20 + read16(fd, pe_header_offset + 4 + 16));

    expptr = 0;
    for (i = 0; i < nsections; i++)
    {
	secptr1 = secptr + 40 * i;
	vaddr = read32(fd, secptr1 + 12);
	vsize = read32(fd, secptr1 + 16);
	fptr = read32(fd, secptr1 + 20);
	SetFilePointer(fd, secptr1, NULL, FILE_BEGIN);
	if (!ReadFile(fd, sname, 8, &num_read, NULL))
	{
	    ExitProcess(-1);
	}
	if (vaddr <= export_rva && vaddr+vsize > export_rva)
	{
	    expptr = fptr + (export_rva - vaddr);
	    if (export_rva + export_size > vaddr + vsize)
	    {
		export_size = vsize - (export_rva - vaddr);
	    }
	    break;
	}
    }

    expdata = (unsigned char*)malloc(export_size);
    SetFilePointer(fd, expptr, NULL, FILE_BEGIN);
    if (!ReadFile(fd, expdata, export_size, &num_read, NULL))
    {
	ExitProcess(-1);
    }
    erva = expdata - export_rva;

    nexp = to32(expdata + 24);
    name_rvas = to32(expdata + 32);

    printf ("EXPORTS\n");
    for (i = 0; i<nexp; i++)
    {
	name_rva = to32(erva + name_rvas + i * 4);
	printf ("\t%s @ %ld ;\n", erva + name_rva, 1 + i);
    }

    CloseHandle(fd);
    return 0;
}
