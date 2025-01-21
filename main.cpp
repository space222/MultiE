#include <cstdio>
#include <cstdlib>
#include <filesystem>
//#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <array>
#include <thread>
#include <SDL.h>
#ifdef _WIN32
// it seems enet pulls in Windows.h, and explicitly doing it results in errors
//#include <Windows.h>
#endif
#include <GL/gl.h>
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl2.h"
#include "imgui/backends/imgui_impl_sdlrenderer2.h"
#include "itypes.h"
#include "progopt.h"
#include "Settings.h"
#include "a26.h"
#include "a52.h"
#include "c64.h"
#include "gbc.h"
#include "nes.h"
#include "sms.h"
#include "genesis.h"
#include "intellivision.h"
#include "ibmpc.h"
#include "psx.h"
#include "casio_pv1k.h"
#include "jag.h"
#include "colecovision.h"
#include "msx.h"
#include "macplus.h"
#include "fc_chanf.h"
#include "gba.h"
#include "virtualboy.h"
#include "WonderSwan.h"
#include "CreatiVision.h"
#include "apple2e.h"
#include "n64.h"
#include "aes.h"
void try_kirq();

namespace fs = std::filesystem;
std::unordered_map<std::string, std::string> cli_options;

SDL_AudioDeviceID audio_dev;
SDL_Window* MainWindow = nullptr;
SDL_Renderer* MainRender = nullptr;
SDL_Texture* Screen = nullptr;
bool Running = true;
bool Paused = false;
std::string modal_msg;
void keys();
void resize_screen();
u32 oldbpp = 32;
u32 oldwidth = 0;
u32 oldheight = 0;
std::string dialog_msg;

console* sys = nullptr;

const int sbuflen = 735*3;
float sample_buf[sbuflen];
std::atomic<int> sample_rd(1), sample_wr(2);

void audio_add(float L, float R)
{
	while( sample_wr == sample_rd ) std::this_thread::yield();
	sample_buf[sample_wr++] = Settings::mute ? 0 : ((L+R)/2);
	sample_wr = sample_wr % sbuflen;
}

float audio_get()
{
	if( (sample_rd + 1) % sbuflen == sample_wr ) return 0;
	float r = sample_buf[sample_rd++];
	sample_rd = sample_rd % sbuflen;
	return r;
}

void audio_callback(void*, Uint8 *stream, int len)
{
	if(! sys )
	{
		memset(stream, 0, len);
		return;
	}
	
	float* buf =(float*) stream;
	len >>= 2;
	for(int i = 0; i < len; ++i)
	{
		buf[i] = audio_get();
	}
}

void copy_fb()
{
	u8* pixels;
	u32 stride = 0;
	SDL_LockTexture(Screen, nullptr, (void**)&pixels, (int*)&stride);
	int mult = sys->fb_bpp() / 8;
	if( stride != sys->fb_width()*mult )
	{
		for(u32 y = 0; y < sys->fb_height(); ++y)
		{
			memcpy(pixels + y*stride, sys->framebuffer() + y*sys->fb_width()*mult, sys->fb_width()*mult);
		}
	} else {
		memcpy(pixels, sys->framebuffer(), sys->fb_width()*sys->fb_height()*mult);
	}
	SDL_UnlockTexture(Screen);
}

std::vector<SDL_GameController*> conts;

void imgui_run();

int main(int argc, char** args)
{
	printf("Starting up\n");
	m68k::init();
	
	ProgramOptions cli(argc, args, {"help", "a26", "a52", "c64", "dmg", "nes", "itv"});
	
	SDL_Init(SDL_INIT_GAMECONTROLLER|SDL_INIT_VIDEO|SDL_INIT_AUDIO);
	MainWindow = SDL_CreateWindow("Multie", 0, 80, 1200, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	MainRender = SDL_CreateRenderer(MainWindow, -1, SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawBlendMode(MainRender, SDL_BLENDMODE_NONE);
	
	{
		SDL_AudioSpec want, got;
		want.freq = 44100;
		want.channels = 1;
		want.callback = audio_callback;
		want.samples = 512;
		want.format = AUDIO_F32;
		audio_dev = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
		SDL_PauseAudioDevice(audio_dev, 0);
	}
	
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	
	ImGui_ImplSDL2_InitForSDLRenderer(MainWindow, MainRender);
	ImGui_ImplSDLRenderer2_Init(MainRender);
	
	for(int i = 0; i < SDL_NumJoysticks(); i++) 
	{
        	if (SDL_IsGameController(i)) 
        	{
        		conts.push_back(SDL_GameControllerOpen(i));
        	}
	}
	
	cli_options = std::move(cli.options);
	
	SDL_Event e;
	while( Running )
	{
		while( SDL_PollEvent(&e) )
		{
			ImGui_ImplSDL2_ProcessEvent(&e);
			switch(e.type)
			{
			case SDL_CLIPBOARDUPDATE:
				//printf("huh? <%s>\n", SDL_GetClipboardText());
				break;
			case SDL_KEYDOWN:
				if( sys ) sys->key_down(e.key.keysym.scancode);
				if( e.key.keysym.scancode == SDL_SCANCODE_X )
				{
					try_kirq();
				}
				if( e.key.keysym.scancode == SDL_SCANCODE_F1 )
				{
					sys->reset();
				}
				break;
			case SDL_KEYUP:
				if( sys ) sys->key_up(e.key.keysym.scancode);
				break;
			case SDL_QUIT:
				Running = false;
				break;
			case SDL_WINDOWEVENT:
				break;
			case SDL_CONTROLLERDEVICEADDED:
				//conts.push_back(SDL_GameControllerOpen(e.cdevice.which));
				//if( conts.size() > 2 ) multitap_active = true;
				break;
			}
		}

		if( sys && !Paused )
		{
			sys->run_frame();
			if( sys->fb_bpp() != oldbpp 
			 || sys->fb_width() != oldwidth
			 || sys->fb_height() != oldheight ) resize_screen();
			copy_fb();
		} else {
			std::this_thread::sleep_for(std::chrono::duration<int, std::milli>(25));
		}

		SDL_SetRenderDrawColor(MainRender, 0x50, 0x50, 0x90, 255);
		SDL_RenderClear(MainRender);
		imgui_run();
		//SDL_RenderCopy(MainRender, Screen, nullptr, nullptr); 
		SDL_RenderPresent(MainRender);
	}
	
	
	//FILE* fi = fopen("dump.data", "wb");
	//fwrite(, 1, 1000, fi);
	//fclose(fi);
	
	//printf("Ended at $%X\n", ((dmg*)sys)->cpu.PC);
	delete sys;
	
	return 0;
}

float crt_scale = 2;

void resize_screen()
{
	if( Screen ) SDL_DestroyTexture(Screen);
	auto format = SDL_PIXELFORMAT_RGBA8888;
	if( sys->fb_bpp() == 16 ) format = SDL_PIXELFORMAT_BGR555;
	else if( sys->fb_bpp() == 24 ) format = SDL_PIXELFORMAT_BGR24;
	oldbpp = sys->fb_bpp();
	oldwidth = sys->fb_width();
	oldheight = sys->fb_height();
	Screen = SDL_CreateTexture(MainRender, format, SDL_TEXTUREACCESS_STREAMING, sys->fb_width(), sys->fb_height());
}

std::string getOpenFile(const std::string&);

void imgui_run()
{
	ImGui_ImplSDL2_NewFrame();
	ImGui_ImplSDLRenderer2_NewFrame();
	ImGui::NewFrame();
	//ImGui::ShowDemoWindow();
	
	bool newinstance = false;
	
	ImGui::BeginMainMenuBar();
		if (ImGui::BeginMenu("Load"))
		{
			if( ImGui::BeginMenu("Atari") )
			{
				if( ImGui::MenuItem("2600") )
				{
					std::string f = getOpenFile("Atari 2600");
					if( !f.empty() )
					{
						delete sys;
						sys = new a26;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
					}
				}
				if( ImGui::MenuItem("5200") )
				{
					std::string f = getOpenFile("Atari 5200");
					if( !f.empty() )
					{
						delete sys;
						sys = new a52;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
					}
				}
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("Commodore") )
			{
				if( ImGui::MenuItem("C64 Tape") )
				{
					std::string f = getOpenFile("C64");
					if( !f.empty() )
					{
						delete sys;
						sys = new c64;
						cli_options["c64-tape"] = f;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
					}
				}
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("Nintendo") )
			{
				if( ImGui::MenuItem("Virtual Boy") )
				{
					std::string f = getOpenFile("Virtual Boy");
					if( !f.empty() )
					{
						delete sys;
						sys = new virtualboy;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 2;
					}				
				}
				if( ImGui::MenuItem("Gameboy") )
				{
					std::string f = getOpenFile("Gameboy [Color]");
					if( !f.empty() )
					{
						delete sys;
						sys = new gbc; // dmg;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 3;
					}
				}
				if( ImGui::MenuItem("NES") )
				{
					std::string f = getOpenFile("NES");
					if( !f.empty() )
					{
						delete sys;
						sys = new nes;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
					}
				}
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("Mattel") )
			{
				if( ImGui::MenuItem("IntelliVision") )
				{
					std::string f = getOpenFile("IntelliVision");
					if( !f.empty() )
					{
						delete sys;
						sys = new intellivision;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
					}
				}
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("IBM") )
			{
				if( ImGui::MenuItem("PC XT") )
				{
					std::string f = getOpenFile("IBM PC BIOS");
					if( !f.empty() )
					{
						delete sys;
						sys = new ibmpc;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 1;
					}				
				}
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("Sega") )
			{
				if( ImGui::MenuItem("SMS") )
				{
					std::string f = getOpenFile("Sega Master System");
					if( !f.empty() )
					{
						delete sys;
						sys = new SMS;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 2;
					}				
				}
				if( ImGui::MenuItem("Genesis") )
				{
					std::string f = getOpenFile("Sega Genesis");
					if( !f.empty() )
					{
						delete sys;
						sys = new genesis;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 2;
					}				
				}
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("Sony") ) 
			{
				if( ImGui::MenuItem("PSX") )
				{
					std::string f = getOpenFile("PlayStation");
					if( !f.empty() )
					{
						delete sys;
						sys = new psx;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 1.5f;
					}				
				}
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("Coleco") ) 
			{
				if( ImGui::MenuItem("ColecoVision") )
				{
					std::string f = getOpenFile("ColecoVision");
					if( !f.empty() )
					{
						delete sys;
						sys = new colecovision;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 2;
					}				
				}
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("Experimental") ) 
			{
				if( ImGui::MenuItem("Neo-Geo AES") )
				{
					std::string f = getOpenFile("Neo-Geo AES");
					if( !f.empty() )
					{
						delete sys;
						sys = new AES;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 2.f;
					}				
				}
				if( ImGui::MenuItem("Nintendo 64") )
				{
					std::string f = getOpenFile("Nintendo 64");
					if( !f.empty() )
					{
						delete sys;
						sys = new n64;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 1.5f;
					}				
				}
				if( ImGui::MenuItem("Apple IIe") )
				{
					std::string f = getOpenFile("Apple IIe");
					if( !f.empty() )
					{
						delete sys;
						sys = new apple2e;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load floppy image\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 3;
					}				
				}
				if( ImGui::MenuItem("V.Tech CreatiVision") )
				{
					std::string f = getOpenFile("CreatiVision");
					if( !f.empty() )
					{
						delete sys;
						sys = new CreatiVision;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 3;
					}				
				}
				if( ImGui::MenuItem("Bandai WonderSwan") )
				{
					std::string f = getOpenFile("WonderSwan");
					if( !f.empty() )
					{
						delete sys;
						sys = new WonderSwan;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 3;
					}				
				}
				if( ImGui::MenuItem("Nintendo GBA") )
				{
					std::string f = getOpenFile("GBA");
					if( !f.empty() )
					{
						delete sys;
						sys = new gba;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 3;
					}				
				}
				if( ImGui::MenuItem("Apple Mac Plus") )
				{
					std::string f = getOpenFile("Mac plus");
					if( !f.empty() )
					{
						delete sys;
						sys = new macplus;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 2;
					}				
				}
				if( ImGui::MenuItem("Casio PV-1000") )
				{
					std::string f = getOpenFile("Casio PV-1000");
					if( !f.empty() )
					{
						delete sys;
						sys = new pv1k;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 2;
					}				
				}
				if( ImGui::MenuItem("Microsoft MSX") )
				{
					std::string f = getOpenFile("Microsoft MSX");
					if( !f.empty() )
					{
						delete sys;
						sys = new msx;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 2;
					}				
				}
				if( ImGui::MenuItem("Fairchild Channel F") )
				{
					std::string f = getOpenFile("Channel F");
					if( !f.empty() )
					{
						delete sys;
						sys = new fc_chanf;
						if( ! sys->loadROM(f) ) 
						{
							printf("unable to load ROM\n");
							exit(1);
						}
						else newinstance = true;
						crt_scale = 5;
					}				
				}
				ImGui::EndMenu();
			}
			//ImGui::BeginDisabled();
			//ImGui::EndDisabled();
			
			if( newinstance )
			{
				sys->reset();
				resize_screen();
			}			
			
			if( ImGui::MenuItem("Exit") ) exit(0);
			ImGui::EndMenu();
		}
		if( ImGui::BeginMenu("System") )
		{
			if( ImGui::MenuItem("Pause", nullptr, &Paused) )
			{
				//if( !Paused ) SDL_ClearQueuedAudio(audio_dev);
			}
			ImGui::MenuItem("Mute", nullptr, &Settings::mute);
			ImGui::Separator();
			if( ImGui::MenuItem("Reset") )
			{
				if( sys ) sys->reset();
			}
			ImGui::EndMenu();
		}
		if( sys && ImGui::BeginMenu("Media") )
		{
			const std::array<std::string, 8>& M = sys->media_names();
			if( M[0].empty() )
			{
				ImGui::BeginDisabled();
				ImGui::MenuItem("No Media");
				ImGui::EndDisabled();
			} else {
				for(int i = 0; i < 8 && !M[i].empty(); ++i)
				{
					if( ImGui::MenuItem(M[i].c_str()) )
					{
						sys->load_media(i, getOpenFile("Load Media: " + M[i]));
					}
				}
			}
			ImGui::EndMenu();
		}
		if( ImGui::BeginMenu("Scale") )
		{
			if( ImGui::MenuItem("1x") ) crt_scale = 1;
			if( ImGui::MenuItem("1.5x") ) crt_scale = 1.5f;
			if( ImGui::MenuItem("2x") ) crt_scale = 2;
			if( ImGui::MenuItem("3x") ) crt_scale = 3;
			if( ImGui::MenuItem("4x") ) crt_scale = 4;
			if( ImGui::MenuItem("5x") ) crt_scale = 5;
			if( ImGui::MenuItem("6x") ) crt_scale = 6;
			if( ImGui::MenuItem("7x") ) crt_scale = 7;
			ImGui::EndMenu();
		}
		/*if( ImGui::BeginMenu("Settings") )
		{
			if( ImGui::BeginMenu("Atari") )
			{
			
				ImGui::EndMenu();
			}
			if( ImGui::BeginMenu("Nintendo") )
			{
			
				ImGui::EndMenu();
			}
		
			ImGui::EndMenu();
		}*/

	ImGui::EndMainMenuBar();
	
	if( !dialog_msg.empty() )
	{
		ImGui::OpenPopup("Error");
		ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_NoResize);
			ImGui::Text("%s", dialog_msg.c_str());
			if( ImGui::Button("OK") ) dialog_msg.clear();
		ImGui::EndPopup();
	}
			
	if( Screen && sys )
	{
		ImGui::SetNextWindowContentSize(ImVec2(sys->fb_scale_w()*crt_scale,sys->fb_scale_h()*crt_scale));
		ImGui::Begin("CRT", nullptr, ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoNav);
			ImGui::Image((void*)Screen, ImVec2(sys->fb_scale_w()*crt_scale,sys->fb_scale_h()*crt_scale));
			//ImGui::Dummy(ImVec2(sys->fb_width()*crt_scale,sys->fb_height()*crt_scale));
		ImGui::End();
	}
	
	ImGui::Render();
	ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
}

void console::setVsync(bool e)
{
	SDL_RenderSetVSync(MainRender, e?1:0); //while?
}

/*void console::setDialogMsg(const std::string m)
{
	dialog_msg = m;
}
*/


bool getKeyState(u32 key)
{
	if( !(key & BIT(31)) )
	{
		auto keys = SDL_GetKeyboardState(nullptr);
		return keys[key];
	}

	//todo: unpack a bitfield that references joystick
	// will need up to 4 sticks + however many buttons on xbox ctrlr
	return false;
}









std::string lastDir;

#ifdef _WIN32
#include <Windows.h>

std::string getOpenFile(const std::string& title)
{


	return "";

}

#else

std::string getOpenFile(const std::string& title)
{	//todo: Windows support
	char filename[1024];
	filename[0] = 0;
	std::string cmd = "zenity --title \"";
	cmd += title + "\" --file-selection 2>/dev/null ";
	if( !lastDir.empty() ) cmd += "--filename=\"" + lastDir + "\"";
	FILE *f = popen(cmd.c_str(), "r"); //2>/dev/null prevents Gtk msg to stderr
	[[maybe_unused]] auto unu = fgets(filename, 1024, f);
	int ret = pclose(f);
	if( !f || ret!=0 )
	{
		return {};
	}
	lastDir = filename;
	if( lastDir.ends_with("\n") ) lastDir.pop_back();
	return lastDir;
}

#endif






