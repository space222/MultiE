#include <print>
#include "dreamcast.h"

void dreamcast::snd_write(u32 addr, u64 v, u32 sz)
{
	std::println("snd-w{} ${:X} = ${:X}", sz, addr, v);
}

u32 dreamcast::snd_read(u32 addr, u32 sz)
{
	std::println("snd-r{} ${:X}", sz, addr);
	return 0;
}


