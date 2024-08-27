#include <string>
#include <vector>
#include <cstdio>
#include "itypes.h"
#include "TAPFile.h"

TAPFile* TAPFile::load(const std::string& fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( ! fp )
	{
		return nullptr;
	}
	
	fseek(fp, 0, SEEK_END);
	auto fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	if( fsize < 20 ) 
	{
		fclose(fp);
		return nullptr;
	}

	u8 hdr[20];
	[[maybe_unused]] int unu = fread(hdr, 1, 20, fp);
	if( hdr[0] != 'C' || hdr[1] != '6' || hdr[2] != '4' || hdr[4] != 'T' || hdr[5] != 'A' || hdr[6] != 'P' )
	{
		fclose(fp);
		return nullptr;
	}
	
	int version = hdr[0xc];
	
	TAPFile* tap = new TAPFile;
	
	while( !feof(fp) )
	{
		int c = fgetc(fp);
		u32 val = 0;
		if( c != 0 )
		{
			val = c*8;
		} else {
			if( version )
			{
				u32 n = fgetc(fp);
				n |= fgetc(fp)<<8;
				n |= fgetc(fp)<<16;
				val = n;
			} else {
				val = 256*8;
			}		
		}
		tap->stamps.push_back(val);
	}
	return tap;
}





