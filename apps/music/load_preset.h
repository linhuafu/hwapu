#ifndef __LOAD_PRESET_H__
#define __LOAD_PRESET_H__


enum sound_effect{
	NORMAL = 0,
	BASS ,
	CLASSICAL ,
	CLUB,
	DANCE,
	FLAT,
	HALL,
	HEADPHONES,
	LIVE,
	PARTY,
	POP,
	REGGAE,
	ROCK,
	SKA,
	SOFT,
	TECHNO,
	TREBLE,
};


const char* effect_string(enum sound_effect effect); 

#endif
