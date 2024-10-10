cmake_minimum_required(VERSION 3.10)

project(test)

find_package(SDL2)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-O3 -march=native -Wall -Wpedantic -Wextra -Wshadow")
set(CMAKE_CPP_COMPILER clang++)

add_executable(test main.cpp imgui/backends/imgui_impl_sdl2.cpp progopt.cpp commodore/c64.cpp commodore/TAPFile.cpp 
atari/a26.cpp atari/a52.cpp atari/antic.cpp atari/tia.cpp cpu/6502.cpp nintendo/dmg.cpp
cpu/sm83.cpp nintendo/nes.cpp nintendo/nes_ppu.cpp nintendo/nes_apu.cpp nintendo/nes_mappers.cpp nintendo/dmg_gfx.cpp 
mattel/intellivision.cpp cpu/cp1600.cpp cpu/z80.cpp ibm/ibmpc.cpp cpu/80286.cpp sony/psx_mdec.cpp sony/psx_timer.cpp
ibm/ibmpc_fdc.cpp  imgui/backends/imgui_impl_sdlrenderer2.cpp ibm/ibmpc_pit.cpp ibm/ibmpc_pic.cpp ibm/ibmpc_vga.cpp 
cpu/68k.cpp sony/psx_pad.cpp sony/psx_spu.cpp sega/sms.cpp sony/psx.cpp cpu/r3000.cpp cpu/r3000_gte.cpp 
Scheduler.cpp sony/psx_gpu.cpp sony/psx_cd.cpp imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp 
imgui/imgui_widgets.cpp other/SN79489.cpp sega/genesis.cpp sega/gen_vdp.cpp sega/gen_snd.cpp 
other/casio_pv1k.cpp sega/gen_pad.cpp Settings.cpp cpu/SH2.cpp sega/gen_32x.cpp atari/jag.cpp
coleco/colecovision.cpp other/TMS9918A.cpp other/msx.cpp other/AY-3-8910.cpp
other/macplus.cpp
)

target_include_directories(test PUBLIC . ./imgui ./include ./sony ./other ./coleco
	./cpu ./atari ./commodore ./sega ./ibm ./nintendo ./mattel ${SDL2_INCLUDE_DIRS} )
target_link_libraries(test PRIVATE dl GL GLEW ${SDL2_LIBRARIES})
#${CMAKE_SOURCE_DIR}/libfmt.a 

