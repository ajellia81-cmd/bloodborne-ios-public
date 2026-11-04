#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef uint64_t Uint64;
typedef uint32_t SDL_JoystickID;
typedef struct SDL_Gamepad{SDL_JoystickID id;} SDL_Gamepad;
SDL_JoystickID SDL_GetGamepadID(SDL_Gamepad*);
Uint64 SDL_GetTicksNS(void);
bool SDL_RumbleGamepad(SDL_Gamepad*,uint16_t,uint16_t,uint32_t);
const char* SDL_GetGamepadName(SDL_Gamepad*);
