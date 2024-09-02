#include "genesis.h"


u16 genesis::getpad1()
{
	u8 val = 0x80;
	val |= 0x3f & ( (pad1_data&0x40) ? key1 : (key2&~0xc) );
	val |= (pad1_data&0x40);
	if( pcycle == 5 )
	{
		val &= 0xf0;
	}
	if( pcycle == 6 )
	{
		val &= 0xf0;
		val |= key3;
	}
	if( pcycle == 7 ) val |= 0xf;

	return val;
}

u16 genesis::getpad2()
{
	u8 val = 0x80;
	val |= 0x3f & ( (pad2_data&0x40) ? pad2_key1 : (pad2_key2&~0xc) );
	val |= (pad2_data&0x40);
	if( pcycle2 == 5 )
	{
		val &= 0xf0;
	}
	if( pcycle2 == 6 )
	{
		val &= 0xf0;
		val |= pad2_key3;
	}
	if( pcycle2 == 7 ) val |= 0xf;

	return val;
}

void genesis::pad_get_keys()
{
	pcycle = pcycle2 = 0;
	pad1_data = pad2_data = PAD_DATA_DEFAULT;
	key1 = key2 = key3 = 0x3f;
	pad2_key1 = pad2_key2 = pad2_key3 = 0x3f;

	/*if( !conts.empty() )
	{
		if(SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP)
			|| SDL_GameControllerGetAxis(conts[0], SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY) < -3000)
		{
			key1 ^= 1; key2 ^= 1; mp1 ^= 0x100;
			
		}
		if(SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN)
			|| SDL_GameControllerGetAxis(conts[0], SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY) > 3000)
		{
			key1 ^= 2; key2 ^= 2; mp1 ^= 0x200;
		}
		if(SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
			|| SDL_GameControllerGetAxis(conts[0], SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX) > 3000)
		{
			key1 ^= 8; mp1 ^= 0x400;
		}
		if(SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT)
			|| SDL_GameControllerGetAxis(conts[0], SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX) < -3000)
		{
			key1 ^= 4; mp1 ^= 0x800;
		}
		if(SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START))
		{
			key2 ^= 0x20; mp1 ^= 0x80;
		}
		if( SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X) ) { key2 ^= 0x10; mp1 ^= 0x40; }
		if( SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A) ) { key1 ^= 0x10; mp1 ^= 0x10; }
		if( SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B) ) { key1 ^= 0x20; mp1 ^= 0x20; }
	
		if( SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ) { key3 ^= 0x4; mp1 ^= 4; }
		if( SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y) ) { key3 ^= 0x2; mp1 ^= 2; }
		if( SDL_GameControllerGetButton(conts[0], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ) { key3 ^= 0x1; mp1 ^= 1; }
	} else { */
		auto K = SDL_GetKeyboardState(nullptr);
		if( K[SDL_SCANCODE_UP] )  { key1 ^= 1; key2 ^= 1; }
		if( K[SDL_SCANCODE_DOWN]) { key1 ^= 2; key2 ^= 2; }
		if( K[SDL_SCANCODE_RIGHT]) { key1 ^= 8; }
		if( K[SDL_SCANCODE_LEFT])  { key1 ^= 4; }

		if( K[SDL_SCANCODE_Z] ) key2 ^= 0x10;
		if( K[SDL_SCANCODE_X] ) key1 ^= 0x10;
		if( K[SDL_SCANCODE_C] ) key1 ^= 0x20;
		
		if( K[SDL_SCANCODE_A] ) key3 ^= 0x4;
		if( K[SDL_SCANCODE_S] ) key3 ^= 0x2;
		if( K[SDL_SCANCODE_D] ) key3 ^= 0x1;

		if( K[SDL_SCANCODE_RETURN] ) key2 ^= 0x20;
	//}
	
	/*if( conts.size() > 1 )
	{
		if(SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_UP)
			|| SDL_GameControllerGetAxis(conts[1], SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY) < -3000)
		{
			pad2_key1 ^= 1; pad2_key2 ^= 1; mp2 ^= 0x100;
		}
		if(SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_DOWN)
			|| SDL_GameControllerGetAxis(conts[1], SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTY) > 3000)
		{
			pad2_key1 ^= 2; pad2_key2 ^= 2; mp2 ^= 0x200;
		}
		if(SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
			|| SDL_GameControllerGetAxis(conts[1], SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX) > 3000)
		{
			pad2_key1 ^= 8; mp2 ^= 0x400;
		}
		if(SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_DPAD_LEFT)
			|| SDL_GameControllerGetAxis(conts[1], SDL_GameControllerAxis::SDL_CONTROLLER_AXIS_LEFTX) < -3000)
		{
			pad2_key1 ^= 4; mp2 ^= 0x800;
		}
		if(SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_START))
		{
			pad2_key2 ^= 0x20; mp1 ^= 0x80;
		}
		if( SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_X) ) { pad2_key2 ^= 0x10; mp2 ^= 0x40; }
		if( SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_A) ) { pad2_key1 ^= 0x10; mp2 ^= 0x10; }
		if( SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_B) ) { pad2_key1 ^= 0x20; mp2 ^= 0x20; }
	
		if( SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ) { pad2_key3 ^= 0x4; mp2 ^= 4; }
		if( SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_Y) ) { pad2_key3 ^= 0x2; mp2 ^= 2; }
		if( SDL_GameControllerGetButton(conts[1], SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) ) { pad2_key3 ^= 0x1; mp2 ^= 1; }
	}
	*/
}

