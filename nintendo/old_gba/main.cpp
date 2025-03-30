#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <span>
#include <string>
//#include <SDL.h>
#include <SFML/Graphics.hpp>
#include "types.h"

std::string romfilename;
u8* ROM;
size_t romsize;
int detected_sram_type = 0;
arm7 cpu;
u32 getreg(arm7& cpu, u32 reg);
int cycles = 0;
int scanline = 0;

//void dma_exec(int);

extern u16 gfxregs[50];
extern u32 framebuffer[240*160];
void gfx_draw_scanline(const int scanline);

extern u16 KEYS;
extern u16 IME, IE, IF;
extern u32 halt_mode;
extern u8 SRAM[128*1024];

sf::Texture* tex;

u8 BIOS[16*1024];

void arm7_step(arm7&);
void arm7_reset(arm7&);
void arm7_dumpregs(arm7&);
bool bfind(u8*, u32, const char*);

//void timer_cycles(u32);

int main(int argc, char** args)
{
	FILE* fbs = fopen("GBABIOS.BIN", "rb");
	if( ! fbs )
	{
		printf("Need GBABIOS.BIN to run\n");
		return -1;
	}
	auto bsize = fread(BIOS, 1, 16*1024, fbs);
	fclose(fbs);
	if( bsize != 16*1024 )
	{
		printf("BIOS must be 16*1024 bytes\n");
		return -1;
	}
	
	if( argc < 2 )
	{
		printf("need a ROM to run\n");
		return -1;
	}
		
	FILE* fp = fopen(args[1], "rb");
	if( ! fp )
	{
		printf("Error: unable to open <%s>\n", args[1]);
		return -1;
	}
	romfilename = args[1];
	auto pos = romfilename.rfind('.');
	std::string savname;
	if( pos == std::string::npos )
	{
		savname = romfilename + ".sav";
	} else {
		savname = romfilename.substr(0, pos) + ".sav";
	}
	FILE* svp = fopen(savname.c_str(), "rb");
	if( svp )
	{
		[[maybe_unused]] int U = fread(SRAM, 1, 128*1024, svp);
		fclose(svp);
	} else {
		for(int i = 0; i < 128*1024; ++i) SRAM[i] = 0xff;
	}
		
	fseek(fp, 0, SEEK_END);
	romsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	printf("ROM Size = %i\n", (int)romsize);
	ROM = new u8[romsize];
	[[maybe_unused]] int unu = fread(ROM, 1, romsize, fp);
	fclose(fp);
	
	if( bfind(ROM, romsize, "FLASH1M") )
	{
		printf("Using 128k flash.\n");
		detected_sram_type = 1;
	} else if( bfind(ROM, romsize, "FLASH512") || bfind(ROM, romsize, "FLASH") )  {
		printf("Using 64k flash.\n");
		detected_sram_type = 2;
	} else if( bfind(ROM, romsize, "EEPROM_") ) {
		printf("EEPROM experimental\n");
		detected_sram_type = 3;
	} else {
		printf("Flash detection failed, using basic SRAM\n");
	}
	
	arm7_reset(cpu);
	
	sf::RenderWindow window(sf::VideoMode(240*3, 160*3), "GBA", sf::Style::Titlebar|sf::Style::Close);
	window.setVerticalSyncEnabled(true);
	
	if( !window.setActive() )
	{
		printf("Unable to enable OpenGL on main window.\n");
		exit(1);
	}
	tex = new sf::Texture;
	if( !tex->create(240, 160) )
	{
		printf("Unable to create texture\n");
		exit(1);
	}
	tex->update((u8*)&framebuffer[0]);
	
	sf::Sprite spr1(*tex);
	spr1.setScale(3, 3);
	
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if(event.type == sf::Event::Closed)
				window.close();
			else if(event.type == sf::Event::KeyPressed )
			{
				if( event.key.code == sf::Keyboard::Key::Q && event.key.control )
					window.close();
			}
		}
		
		KEYS = 0x3ff;
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::Z) ) KEYS ^= 1;
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::X) ) KEYS ^= 2;
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::A) ) KEYS ^= 4;
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::S) ) KEYS ^= 8;
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::Right) ) KEYS ^= (1<<4);
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::Left) ) KEYS ^= (1<<5);
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::Up) ) KEYS ^= (1<<6);
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::Down) ) KEYS ^= (1<<7);
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::W) ) KEYS ^= (1<<8);
		if( sf::Keyboard::isKeyPressed(sf::Keyboard::Q) ) KEYS ^= (1<<9);

		window.clear();
		
		auto time_b4 = std::chrono::system_clock::now();
		for(int i = 0; i < 240*160; ++i) framebuffer[i] = 0xffffff;
		for(scanline = 0; scanline < 228; ++scanline)
		{
			if( scanline == 0 )
			{
				gfxregs[2] &= ~1;
			} else if( scanline == 160 ) {
				gfxregs[2] |= 1;
				if( (gfxregs[2] & 8) )
				{
					//printf("VBlank IRQ!\n");
					IF |= 1;
				}
			}
			gfxregs[3] = scanline;
			if( scanline == (gfxregs[2]>>8) )
			{
				gfxregs[2] |= 4;
				if( (gfxregs[2] & 0x20) )
				{
					//printf("VCount IRQ!\n");
					IF |= 4;
				}
			} else {
				gfxregs[2] &= ~4;
			}
			
			if( halt_mode!=0xff && (IF&IE) )
			{
				halt_mode = 0xff;
				//printf("System out of halt mode, PC=%X\n", cpu.regs[15]);
			}
			for(int i = 0; i < 1232 && halt_mode == 0xff; i+=2)  // a guess at an average number of cycles
			{
				arm7_step(cpu);
				//timer_cycles(3);
				if( i == 900 )
				{
					gfxregs[2] |= 2;
					if( (gfxregs[2] & 0x10) ) IF |= 2;
				} else if( i == 0 ) {
					gfxregs[2] &= ~2;
				}
			}
			if( scanline < 160 ) gfx_draw_scanline(scanline);
		}
		
		//while(  std::chrono::system_clock::now() - time_b4 < std::chrono::milliseconds(15) );
		tex->update((u8*)&framebuffer[0]);
		window.draw(spr1);
		window.display();
	}
	
	FILE* svo = fopen(savname.c_str(), "wb");
	if( !svo )
	{
		printf("Unable to write to save file!\n");
	} else {
		[[maybe_unused]] int mu = fwrite(SRAM, 1, 128*1024, svo);
		fclose(svo);
	}
	printf("gfxctrl = %X, IE = %X\n", gfxregs[0], IE);
	return 0;
}

bool bfind(u8* bin, u32 blen, const char* str)
{
	int slen = strlen(str);
	std::span<u8> B(bin, blen);
	std::span<u8> S((u8*)str, slen);
	return std::search(std::begin(B), std::end(B), std::begin(S), std::end(S)) != std::end(B);
}






