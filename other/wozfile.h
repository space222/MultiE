#pragma once
#include <cstdio>
#include <vector>
#include <string_view>
#include <string>
#include "itypes.h"

class wozfile
{
public:
	static wozfile* loadWOZ(const std::string);
	
	std::vector<u32> track_bitlen;
	std::vector<std::vector<u8> > tracks;
	u8 tmap[160];
	u8 info[60];
	
	u8 getBit(u32 track, u32 bit)
	{
		if( track >= 160 ) return 0;
		if( tmap[track] == 0xff ) return 0; //todo: silliness
		if( track_bitlen[tmap[track]] == 0 ) return 0; //??
		
		bit %= track_bitlen[tmap[track]];		
		u32 byte = bit >> 3;
		bit = (bit&7) ^ 7;
		return (tracks[tmap[track]][byte]>>bit)&1;	
	}
	
private:
	static u32 find_chunk(const std::vector<u8>&, std::string_view);
};

