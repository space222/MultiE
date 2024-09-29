#include "AY-3-8910.h"

void AY_3_8910::clock()
{




}

u8 AY_3_8910::cycles(u64 cc)
{
	stamp += cc;
	while( stamp > 16 )
	{
		stamp -= 16;
		clock();
	}
	return out;
}


