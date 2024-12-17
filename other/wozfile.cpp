#include <cstring>
#include "wozfile.h"

u32 wozfile::find_chunk(const std::vector<u8>& file, std::string_view cn)
{
	u32 pos = 12;
	while( pos < file.size() )
	{
		if( file[pos+0] == cn[0] 
		 && file[pos+1] == cn[1] 
		 && file[pos+2] == cn[2] 
		 && file[pos+3] == cn[3] )
		{
			return pos;
		}
		pos += 4;
		u32 incr = file[pos];
		incr |= file[pos+1]<<8;
		incr |= file[pos+2]<<16;
		incr |= file[pos+3]<<24;
		pos += 4 + incr;
	}
	return 0;
}

wozfile* wozfile::loadWOZ(const std::string fname)
{
	FILE* fp = fopen(fname.c_str(), "rb");
	if( !fp ) return nullptr;
	
	std::vector<u8> data;
	fseek(fp, 0, SEEK_END);
	data.resize(ftell(fp));
	fseek(fp, 0, SEEK_SET);
	
	[[maybe_unused]] int unu = fread(data.data(), 1, data.size(), fp);
	fclose(fp);
	
	wozfile* W = new wozfile;
	u32 ipos = find_chunk(data, "INFO") + 8;
	if( ipos ) memcpy(W->info, &data[ipos], 60);
	memcpy(W->tmap, &data[find_chunk(data, "TMAP")+8], 160);
	
	u32 p = find_chunk(data, "TRKS")+8;
	if( p == 8 )
	{
		delete W;
		return nullptr;
	}
	
	for(u32 track = 0; track < 160; ++track)
	{	
		u32 offset = 512 * (data[p]|(data[p+1]<<8));
		u32 bitcnt = data[p+4]|(data[p+5]<<8)|(data[p+6]<<16)|(data[p+7]<<24);
		u32 len = (bitcnt>>3) + ((bitcnt&7)!=0);
		//printf("len = %i bytes. bitcnt = $%X\n", len, bitcnt);
		
		if( len != 0 ) 
		{
			std::vector<u8>& bits = W->tracks.emplace_back(len);
			memcpy(bits.data(), &data[offset], len);
		} else {
			W->tracks.emplace_back(0);
		}
		
		W->track_bitlen.push_back(bitcnt);
		p += 8;	
	}
	
	return W;
}

