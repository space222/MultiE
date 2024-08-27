#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include "psx.h"

int zagzig[] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

inline s16 se10to16(u16 v)
{
	s16 res = (v<<6);
	return res>>6;
}

void psx::decode_block(u32 index, u8* qt)
{
	u32 n = 0;
	u32 k = 0;
	do {
		n = mdec_data[mdec_index++];
	} while( mdec_index < mdec_data.size() && n == 0xfe00 );
	if( mdec_index >= mdec_data.size() ) return;
	mb& B = blocks[index];
	for(u32 i = 0; i < 64; ++i) B.v[i] = B.output[i] = 0;
	
	B.quant = (n>>10)&0x3F;
	s16 val = se10to16(n)*qt[0];
	do {
		if( B.quant == 0 ) val = se10to16(n)*2; 
		val = std::clamp(val+0,-0x400,0x3ff);
		if( B.quant > 0 ) B.output[zagzig[k]] = val;
		if( B.quant==0 )  B.output[k] = val;
		n = mdec_data[mdec_index++];
		k += ((n>>10)&0x3F)+1;
		val = (se10to16(n)*qt[k]*B.quant+4)/8;
	} while( k < 64 ); 
	
	int sum = 0;
	for(u32 x = 0; x < 8; ++x)
	{
		for(u32 y = 0; y < 8; ++y)
		{
			sum = 0;
			for(u32 z = 0; z < 8; ++z)
			{
				sum += B.output[y+z*8]*(mdec_scale[x+z*8]/8);
			}
		        B.v[x+y*8] = (sum+0x0fff)/0x2000; //<-- or so?
		}
	}
	for(u32 x = 0; x < 8; ++x)
	{
		for(u32 y = 0; y < 8; ++y)
		{
			sum = 0;
			for(u32 z = 0; z < 8; ++z)
			{
				sum += B.v[y+z*8]*(mdec_scale[x+z*8]/8);
			}
		        B.output[x+y*8] = (sum+0x0fff)/0x2000; //<-- or so?
		}
	}
}

#define Cr blocks[0].output
#define Cb blocks[1].output
#define Y(a) blocks[2+(a)].output

void psx::mdec_run()
{
	u32 cmd = mdec_in[0];
	
	switch( cmd>>29 )
	{
	case 1:
		if( mdec_in.size() < 1 + (cmd&0xffff) )
		{
			mdec_stat &= ~0xffff;
			mdec_stat &= ~(0xFu<<25);
			//todo: fix the words remaining thingy
			mdec_stat |= (cmd&0xffff) - (mdec_in.size() - 1);
			mdec_stat |= ((mdec_in[0]>>2)&(0xFu<<25));
			return;		
		}
		//printf("MDEC: About to decomp %i words\n", (int) mdec_in.size());
		mdec_stat |= 0xffff;
		mdec_index = 0;
		mdec_in.pop_front();
		//time to decompress
		for(u32 i = 0; i < (cmd&0xffff); ++i) 
		{
			u32 v = mdec_in.front();
			mdec_data.push_back(v);
			mdec_data.push_back(v>>16);
			mdec_in.pop_front();
		}
decomp:
		if( cmd & (1u<<28) )
		{
			decode_block(0, &qtabc[0]);
			decode_block(1, &qtabc[0]);
			decode_block(2, &qtaby[0]);
			decode_block(3, &qtaby[0]);
			decode_block(4, &qtaby[0]);
			decode_block(5, &qtaby[0]);
			//fprintf(stderr, "MDEC: %i (blocks)\n", mdec_cur_block);
			for(u32 y = 0; y < 16; ++y)
			{
				for(u32 x = 0; x < 16; ++x)
				{
					int fR = Cr[(y/2)*8 + (x/2)];
					int fB = Cb[(y/2)*8 + (x/2)];
					int fY = Y( (y/8)*2 + (x/8) )[(y&7)*8 + (x&7)];
					int R = fY + (1.402 * (fR));
					int G = fY - (0.334136 * (fB)) - (0.714136 * (fR));
					int B = fY + (1.772 * (fB));

					uint8_t r = std::clamp(R + 128, 0, 255);
					uint8_t g = std::clamp(G + 128, 0, 255);
					uint8_t b = std::clamp(B + 128, 0, 255);
					if( (cmd&(1u<<27)) )
					{
						r >>= 3;
						g >>= 3;
						b >>= 3;
						r &= 0x1f;
						g &= 0x1f;
						b &= 0x1f;
						if( !(cmd&(1u<<26)) ) 
						{
							//r ^= 0x10; g ^= 0x10; b ^= 0x10;
						}
						u16 v = (b<<10)|(g<<5)|r;
						if( cmd & (1u<<25) ) v |= 0x8000;
						mdec_out.push_back(v);
						mdec_out.push_back(v>>8);
					} else {
						if( !(cmd&(1u<<26)) ) 
						{
							//r ^= 0x80; g ^= 0x80; b ^= 0x80;
						}
						mdec_out.push_back(b);
						mdec_out.push_back(g);
						mdec_out.push_back(r);
					}
				}
			}
		} else {
			u8 f = 0;
			bool flatch = false;
			decode_block(0, &qtaby[0]);
			for(u32 i = 0; i < 64; ++i)
			{
				s16 v = blocks[0].output[i];
				u8 b = std::clamp(se10to16(v)+0, -128, 127);
				if( !(cmd&(1u<<26)) ) b ^= 0x80;
				if( cmd&(1u<<27) ) mdec_out.push_back(b);
				else {
					if( flatch )
					{
						mdec_out.push_back(f|(b&0xf0));
					} else {
						f = (b>>4)&0xf;
					}
					flatch = !flatch;
				}
			}
		}
		if( mdec_index < mdec_data.size() ) goto decomp;
		//printf("MDEC: got %i words left in mdec_in\n", (int)mdec_in.size());
		mdec_data.clear();
		break;
	case 2:
		if( mdec_in.size() < 17u + ((cmd&1)?16u:0) )
		{
			mdec_stat &= ~0xffff;
			mdec_stat &= ~(0xFu<<25);
			//todo: fix the words remaining thingy
			mdec_stat |= ((17 + ((mdec_in[0]&1)?16:0))-(mdec_in.size()-1))&0xffff;
			mdec_stat |= ((mdec_in[0]>>2)&(0xFu<<25));
			return;
		}
		mdec_in.pop_front();
		mdec_stat |= 0xffff;
		for(u32 i = 0; i < 16; ++i)
		{
			qtaby[i*4] = mdec_in.front();
			qtaby[i*4+1] = mdec_in.front()>>8;
			qtaby[i*4+2] = mdec_in.front()>>16;
			qtaby[i*4+3] = mdec_in.front()>>24;
			mdec_in.pop_front();		
		}
		if( cmd&1 )
		{
			for(u32 i = 0; i < 16; ++i)
			{
				qtabc[i*4] = mdec_in.front();
				qtabc[i*4+1] = mdec_in.front()>>8;
				qtabc[i*4+2] = mdec_in.front()>>16;
				qtabc[i*4+3] = mdec_in.front()>>24;
				mdec_in.pop_front();		
			}
		}
		//printf("MDEC: Got qtable, insize = %i\n", mdec_in.size());
		break;
	case 3:
		if( mdec_in.size() < 33 )
		{
			mdec_stat &= ~0xffff;
			mdec_stat &= ~(0xFu<<25);
			mdec_stat |= (32-(mdec_in.size()-1))&0xffff;
			mdec_stat |= ((mdec_in[0]>>2)&(0xFu<<25));
			return;
		}
		mdec_in.pop_front();
		mdec_stat |= 0xffff;
		for(u32 i = 0; i < 32; ++i)
		{
			mdec_scale[i*2] = mdec_in.front();
			mdec_scale[i*2+1] = mdec_in.front()>>16;
			mdec_in.pop_front();		
		}
		//printf("MDEC: Got scaletable, insize = %i\n", mdec_in.size());
		//printf("mdec scale table:\n");
		//for(u32 i = 0; i < 64; ++i) printf("$%X ", (u16)mdec_scale[i]);
		//printf("\n-------\n");
		break;
	case 0:
	default:
		fprintf(stderr, "MDEC: Crap, an unknown cmd $%X\n", cmd);
		mdec_stat &= ~0xffff;
		mdec_stat &= ~(0xFu<<25);
		mdec_stat |= (mdec_in[0]&0xffff);
		mdec_stat |= ((mdec_in[0]>>2)&(0xFu<<25));
		mdec_in.pop_front();
		break;
	}
}

void psx::mdec_cmd(u32 val)
{
	//printf("MDEC: cmd/param = $%X\n", val);
	mdec_in.push_back(val);
	mdec_run();
}

void psx::mdec_write(u32 addr, u32 val, int)
{
	if( addr == 0x1F801820 )
	{
		mdec_cmd(val);
		return;
	}
	if( addr == 0x1F801824 )
	{
		if( val & (1u<<31) )
		{
			mdec_stat = 0x80040000u;
			mdec_in.clear();
			mdec_out.clear();
		}
		return;
	}
	printf("MDEC: Write $%X = $%X\n", addr, val);
}

u32 psx::mdec_read(u32 addr, int)
{
	if( addr == 0x1F801820 )
	{
		if( mdec_out.empty() ) 
		{
			//printf("read from empty mdec_out\n");
			return 0;
		}
		u32 v = mdec_out.front(); mdec_out.pop_front();
		v |= mdec_out.front()<<8; mdec_out.pop_front();
		v |= mdec_out.front()<<16; mdec_out.pop_front();
		v |= mdec_out.front()<<24; mdec_out.pop_front();
		return v;
	}
	if( addr == 0x1F801824 )
	{
		u32 v = mdec_stat;
		v &= ~(1u<<31);
		//v |= (mdec_out.empty() ? (1u<<31) : 0);
		//printf("MDEC: stat read = $%X\n", v);
		return v;
	}
	printf("MDEC: Read $%X\n", addr);
	return 0;

}





