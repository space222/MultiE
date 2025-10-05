#include <vector>
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


