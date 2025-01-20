#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "aes.h"
extern console* sys;

void AES::write(u32 addr, u32 v, int size)
{
}

u32 AES::read(u32 addr, int size)
{
	return 0;
}

bool AES::loadROM(const std::string fname)
{
	std::string prefix = fname.substr(0, fname.rfind('.'));
	std::string promfile = prefix + ".p1";
	FILE* fp = fopen(promfile.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to open <%s>\n", promfile.c_str());
		return false;
	}
	
	fseek(fp, 0, SEEK_END);
	auto fsz = ftell(fp);
	if( fsz < 0x100000 ) fsz = 0x100000;
	if( fsz & 1 ) fsz += 1;
	fseek(fp, 0, SEEK_SET);
	p1.resize(fsz);
	[[maybe_unused]] int unu = fread(p1.data(), 1, fsz, fp);
	fclose(fp);
	
	for(u32 i = 0; i < p1.size(); i+=2)
	{
		*(u16*)&p1[i] = __builtin_bswap16(*(u16*)&p1[i]);
	}
	
	std::string zfile = prefix + ".m1";
	fp = fopen(zfile.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to open <%s>\n", zfile.c_str());
		return false;
	}
	fseek(fp, 0, SEEK_END);
	fsz = ftell(fp);
	if( fsz < 0x10000 ) fsz = 0x10000;
	fseek(fp, 0, SEEK_SET);
	m1.resize(fsz);
	unu = fread(m1.data(), 1, fsz, fp);
	fclose(fp);
	
	std::string fixfile = prefix + ".s1";
	fp = fopen(fixfile.c_str(), "rb");
	if( !fp )
	{
		printf("Unable to open <%s>\n", fixfile.c_str());
		return false;
	}
	fseek(fp, 0, SEEK_END);
	fsz = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	s1.resize(fsz);
	unu = fread(s1.data(), 1, fsz, fp);
	fclose(fp);
	return true;
}

void AES::run_frame()
{



}

void AES::reset()
{
	memset(&cpu, 0, sizeof(cpu));
	cpu.mem_read8 = [](u32 addr)->u8 {return dynamic_cast<AES*>(sys)->read(addr, 8); };
	cpu.mem_read16 =[](u32 addr)->u16{return dynamic_cast<AES*>(sys)->read(addr, 16); };
	cpu.mem_read32 =[](u32 addr)->u32{return(dynamic_cast<AES*>(sys)->read(addr, 16)<<16)|(dynamic_cast<AES*>(sys)->read(addr+2, 16)); };
	cpu.mem_write8 =[](u32 addr, u8 v){ dynamic_cast<AES*>(sys)->write(addr, v, 8); };
	cpu.mem_write16 =[](u32 addr, u16 v){dynamic_cast<AES*>(sys)->write(addr, v, 16); };
	cpu.mem_write32 =[](u32 addr, u32 v){dynamic_cast<AES*>(sys)->write(addr, v>>16, 16); dynamic_cast<AES*>(sys)->write(addr+2, v, 16); };
		
	spu.reset();
}

