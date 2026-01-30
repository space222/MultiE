#include <vector>
#include <print>
#include <functional>
#include <cstdio>
#include "itypes.h"

u32 fsize(FILE* fp)
{
	auto pos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	u32 fsz = ftell(fp);
	fseek(fp, pos, SEEK_SET);
	return fsz;
}


bool freadall(u8* dst, FILE* fp, u32 max)
{
	if( !fp ) return false;
	u32 sz = fsize(fp);
	fseek(fp, 0, SEEK_SET);
	[[maybe_unused]] int unu = fread(dst, 1, (max == 0 || max > sz) ? sz : max, fp);
	fclose(fp);
	return true;
}

bool freadall(std::vector<u8>& dst, FILE* fp)
{
	if( !fp ) return false;
	u32 sz = fsize(fp);
	fseek(fp, 0, SEEK_SET);
	if( dst.size() < sz ) dst.resize(sz);
	[[maybe_unused]] int unu = fread(dst.data(), 1, sz, fp);
	fclose(fp);
	return true;
}

typedef struct
{
  unsigned char	e_ident[16];	/* Magic number and other info */
  u16	e_type;			/* Object file type */
  u16	e_machine;		/* Architecture */
  u32	e_version;		/* Object file version */
  u32	e_entry;		/* Entry point virtual address */
  u32	e_phoff;		/* Program header table file offset */
  u32	e_shoff;		/* Section header table file offset */
  u32	e_flags;		/* Processor-specific flags */
  u16	e_ehsize;		/* ELF header size in bytes */
  u16	e_phentsize;		/* Program header table entry size */
  u16	e_phnum;		/* Program header table entry count */
  u16	e_shentsize;		/* Section header table entry size */
  u16	e_shnum;		/* Section header table entry count */
  u16	e_shstrndx;		/* Section header string table index */
} PACKED Elf32_Ehdr;

typedef struct
{
  u32	p_type;			/* Segment type */
  u32	p_offset;		/* Segment file offset */
  u32	p_vaddr;		/* Segment virtual address */
  u32	p_paddr;		/* Segment physical address */
  u32	p_filesz;		/* Segment size in file */
  u32	p_memsz;		/* Segment size in memory */
  u32	p_flags;		/* Segment flags */
  u32	p_align;		/* Segment alignment */
} PACKED Elf32_Phdr;

const u32 PT_LOAD = 1;

u32 loadELF(FILE* fp, std::function<void(u32 addr, u8 v)> writer)
{
	if( fp == nullptr ) return false;
	Elf32_Ehdr head;
	[[maybe_unused]] int unu = fread(&head, 1, sizeof(head), fp);
	if( head.e_ident[0] != 0x7F || head.e_ident[1] != 'E' || head.e_ident[2] != 'L' || head.e_ident[3] != 'F' )
	{
		std::println("BAD ELF!");
		exit(1);	
		return 0xbad0e1f;
	}
	
	fseek(fp, head.e_phoff, SEEK_SET);
	
	for(u32 i = 0; i < head.e_phnum; ++i)
	{
		Elf32_Phdr seg;
		auto next = ftell(fp) + head.e_phentsize;
		unu = fread(&seg, 1, sizeof(seg), fp);
		if( seg.p_type == PT_LOAD )
		{
			u32 n = 0;
			u32 addr = seg.p_vaddr;
			fseek(fp, seg.p_offset, SEEK_SET);
			for(; n < seg.p_filesz; ++n, ++addr)
			{
				writer(addr, fgetc(fp));
			}
			for(; n < seg.p_memsz; ++n, ++addr)
			{
				writer(addr, 0);
			}
		}		
		fseek(fp, next, SEEK_SET);
	}
	
	return head.e_entry;
}









