#include <print>
#include <string>
#include <SDL.h>
#include "dreamcast.h"

void dreamcast::maple_port1(u32 dest, u32* msg, u32 len)
{
	u32 frame_header = __builtin_bswap32(msg[0]);
	msg += 1;

	u8 cmd = frame_header>>24;
	u8 recip = frame_header>>16;
	u8 sender = frame_header>>8;
	u8 words = frame_header;
	
	//std::println("maple port 1 cmd${:X}, recip${:X}, sender${:X}, len${:X}", cmd, recip, sender, words);	

	if( cmd == 1 )
	{
		if( recip != 0x20 ) { *(u32*)&RAM[dest] = 0xffffFFFFu; return; }
		const char* prod = "Dreamcast Controller           ";
		const char* lic = "Produced By or Under License From SEGA ENTERPRISES,LTD.     ";
		//std::println("Trying to return maple-attached gamepad.");
		*(u32*)&RAM[dest] = __builtin_bswap32(28|(5<<24)|(0x20<<16)); dest += 4;
		*(u32*)&RAM[dest] = 0x01000000; dest+=4; // func
		*(u32*)&RAM[dest] = 0xFE0F0600; dest+=4; // func data[0-2];
		*(u32*)&RAM[dest] = 0; dest+=4; // func data[0-2];
		*(u32*)&RAM[dest] = 0; dest+=6; // func data[0-2];
		memcpy(&RAM[dest], prod, strlen(prod)+1); // func data[0-2];
		dest += strlen(prod);
		memcpy(&RAM[dest], lic, strlen(lic)+1);
		return;
	}

	if( cmd == 9 )
	{
		*(u32*)&RAM[dest] = __builtin_bswap32((8<<24)|(0x20<<16)|3);
		*(u32*)&RAM[dest+4] = __builtin_bswap32(1);
		*(u32*)&RAM[dest+8] = (maple_keys1());

		auto keys = SDL_GetKeyboardState(nullptr);
		u32 analog = keys[SDL_SCANCODE_S] ? 0xf0 : (keys[SDL_SCANCODE_W] ? 0x10 : 0x80);
		analog <<= 8;
		analog |= keys[SDL_SCANCODE_D] ? 0xf0 : (keys[SDL_SCANCODE_A] ? 0x10 : 0x80);
		
		*(u32*)&RAM[dest+12]= (0x80800000u|analog);
		return;
	}
	
	std::println("Unimpl maple cmd = ${:X}", cmd);
}

void dreamcast::maple_port2(u32 dest, u32* msg, u32 len)
{
	*(u32*)&RAM[dest&0xffFFff] = 0xffffFFFFu;
}

u32 dreamcast::maple_keys1()
{
	u32 btn = 0xffffFFFFu;
	auto keys = SDL_GetKeyboardState(nullptr);

	if( keys[SDL_SCANCODE_UP] ) btn ^= BIT(4);
	if( keys[SDL_SCANCODE_DOWN] ) btn ^= BIT(5);
	if( keys[SDL_SCANCODE_LEFT] ) btn ^= BIT(6);
	if( keys[SDL_SCANCODE_RIGHT] ) btn ^= BIT(7);
	if( keys[SDL_SCANCODE_Z] ) btn ^= BIT(2);
	if( keys[SDL_SCANCODE_RETURN] ) btn ^= BIT(3);

	return btn;
}











