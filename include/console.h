#pragma once
#include <string>
#include <array>
#include <SDL.h>
#include "itypes.h"

class console
{
public:
	virtual void reset() =0;
	virtual void run_frame() =0;
	virtual bool loadROM(std::string) =0;
	virtual u32 fb_width() =0;
	virtual u32 fb_height() =0;
	virtual u32 fb_bpp() { return 32; }
	virtual u32 fb_scale_w() { return fb_width(); }
	virtual u32 fb_scale_h() { return fb_height(); }
	virtual u8* framebuffer() =0;
	
	virtual void key_down(int) {}
	virtual void key_up(int) {}
	
	virtual float pop_sample() { return 0; }
	
	void setVsync(bool);
	
	virtual void event(u64, u32) { return; }
	
	virtual std::array<std::string, 8>& media_names() 
	{
		static std::array<std::string, 8> a;
		return a;
	}
	
	virtual bool load_media(int, std::string) { return false; }
	
	virtual ~console()
	{
		setVsync(true);
	}
	
	//static void setDialogMsg(const std::string);
};

void audio_add(float, float);

