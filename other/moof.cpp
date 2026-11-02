#include <print>
#include <string>
#include "moof.h"

static u32 find_chunk(std::vector<u8>& D, char* id)
{
	u32 pos = 12;
	for(; pos+3 < D.size(); ++pos )
	{
		if( D[pos] == id[0] && D[pos+1] == id[1] && D[pos+2] == id[2] && D[pos+3] == id[3] )
		{
			return pos;
		}
	}
	return 0;
}

MOOF* MOOF::load(const std::string& fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp )
	{
		return nullptr;
	}
	
	MOOF* M = new MOOF;
	
	fseek(fp, 0, SEEK_END);
	M->data.resize(ftell(fp));
	fseek(fp, 0, SEEK_SET);
	
	[[maybe_unused]] int unu = fread(M->data.data(), M->data.size(), 1, fp);
	fclose(fp);
	
	u32 info = find_chunk(M->data, (char*)"INFO") + 8;
	M->sides = M->data[info+1];
	u32 tmap = find_chunk(M->data, (char*)"TMAP") + 8;
	u32 trks = find_chunk(M->data, (char*)"TRKS") + 8;
	std::println("info ${:X}, tmap ${:X}, trks ${:X}", info, tmap, trks);
	std::println("sides = {}", M->sides);
	
	for(int i = 0; i < 80*2; ++i)
	{
		u8 A = M->data[tmap+i];
		
		if( A == 0xFF )
		{
			//todo: something more clever than nothing
			M->track_offsets[i&1].push_back(0);
			M->track_lengths[i&1].push_back(512);
			continue;
		}
		
		u32 offset = *(u16*)&M->data[trks+A*8];
		u32 bitcount = *(u32*)&M->data[trks+A*8+4];
		M->track_offsets[i&1].push_back(offset*512);
		M->track_lengths[i&1].push_back(bitcount);
		//std::println("track {} length = {} bytes", i, float(bitcount)/8);
	}	

	return M;
}


