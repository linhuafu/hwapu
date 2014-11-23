#include <stdio.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <load_preset.h>
#include <string.h>


#define XML_NODE_BAND        "band"
#define XML_NODE_EQUALIZER   "equalizer"
#define XML_PROP_BAND_NUMBER "num"

#define MUSIC_PRESET_PATH	 "/etc/music/preset/"




/*equalizer-10bands create 17 sound effect*/
const char* 
effect_string(enum sound_effect effect)
{
	const char* str[17] = {"normal", "bass", "classical", "club", "dance",
		"flat", "hall", "headphones", "live", "party", "pop", "reggae", "rock"
		,"ska", "soft", "techno", "treble"};

	if (effect > 16 || effect < 0)
		return str[0];

	return str[effect];
}

