#include "driver.h"
#include "info.h"

/* CPU information table */
struct cpu_desc
{
	int cpu_type;
	const char* desc;
};

/* SOUND information table */
typedef unsigned (*SOUND_clock)(const void* interface);
typedef unsigned (*SOUND_num)(const void* interface);

struct sound_desc
{
	int sound_type;
	SOUND_num num;
	SOUND_clock clock;
	const char* desc;
};



/* ------------------------------------------------------------------------*/
/* CPU information */

struct cpu_desc CPU_DESC[] =
{
	{ CPU_Z80,       "Z80"      },
    { CPU_8085A,     "I8085"    },
	{ CPU_8080,      "I8080"    },
	{ CPU_M6502,     "M6502"    },
	{ CPU_M65C02,	 "M65C02"   },
	{ CPU_M6510,	 "M6510"    },
	{ CPU_H6280,     "H6280"    },
	{ CPU_I86,       "I86"      },
	{ CPU_I8035,     "I8035"    },
    { CPU_I8039,     "I8039"    },
	{ CPU_I8048,	 "I8048"    },
	{ CPU_N7751,	 "N7751"    },
	{ CPU_M6800,	 "M6800"    },
    { CPU_M6802,     "M6802"    },
	{ CPU_M6803,     "M6803"    },
	{ CPU_M6808,     "M6808"    },
	{ CPU_HD63701,   "HD63701"  },
	{ CPU_M6805,     "M6805"    },
	{ CPU_M68705,	 "M68705"   },
    { CPU_M6309,     "M6309"    },
	{ CPU_M6809,     "M6809"    },
	{ CPU_M68000,    "M68000"   },
	{ CPU_M68010,	 "M68010"   },
	{ CPU_M68020,	 "M68020"   },
	{ CPU_T11,       "T11"      },
	{ CPU_S2650,     "S2650"    },
	{ CPU_TMS34010,  "TMS34010" },
	{ CPU_TMS9900,   "TMS9900"  },
	{ CPU_Z8000,     "Z8000"    },
	{ CPU_TMS320C10, "TMS32010" },
	{ CPU_CCPU, 	 "C-CPU"    },
#ifdef MESS
	{ CPU_PDP1, 	 "PDP1"     },
#endif
    { 0,0 }
};

/* ------------------------------------------------------------------------*/
/* SOUND information */

unsigned DAC_num(const void* interface) { return ((struct DACinterface*)interface)->num; }
unsigned ADPCM_num(const void* interface) { return ((struct ADPCMinterface*)interface)->num; }
unsigned OKIM6295_num(const void* interface) { return ((struct OKIM6295interface*)interface)->num; }
unsigned MSM5205_num(const void* interface) { return ((struct MSM5205interface*)interface)->num; }
unsigned HC55516_num(const void* interface) { return ((struct CVSDinterface*)interface)->num; }
unsigned AY8910_clock(const void* interface) { return ((struct AY8910interface*)interface)->baseclock; }
unsigned AY8910_num(const void* interface) { return ((struct AY8910interface*)interface)->num; }
unsigned YM2203_clock(const void* interface) { return ((struct YM2203interface*)interface)->baseclock; }
unsigned YM2203_num(const void* interface) { return ((struct YM2203interface*)interface)->num; }
unsigned YM2413_clock(const void* interface) { return ((struct YM2413interface*)interface)->baseclock; }
unsigned YM2413_num(const void* interface) { return ((struct YM2413interface*)interface)->num; }
unsigned YM2610_clock(const void* interface) { return ((struct YM2610interface*)interface)->baseclock; }
unsigned YM2610_num(const void* interface) { return ((struct YM2610interface*)interface)->num; }
unsigned POKEY_clock(const void* interface) { return ((struct POKEYinterface*)interface)->baseclock; }
unsigned POKEY_num(const void* interface) { return ((struct POKEYinterface*)interface)->num; }
unsigned YM3812_clock(const void* interface) { return ((struct YM3812interface*)interface)->baseclock; }
unsigned YM3812_num(const void* interface) { return ((struct YM3812interface*)interface)->num; }
unsigned VLM5030_clock(const void* interface) { return ((struct VLM5030interface*)interface)->baseclock; }
unsigned TMS5220_clock(const void* interface) { return ((struct TMS5220interface*)interface)->baseclock; }
unsigned YM2151_clock(const void* interface) { return ((struct YM2151interface*)interface)->baseclock; }
unsigned YM2151_num(const void* interface) { return ((struct YM2151interface*)interface)->num; }
unsigned NES_clock(const void* interface) { return ((struct NESinterface*)interface)->baseclock; }
unsigned NES_num(const void* interface) { return ((struct NESinterface*)interface)->num; }
unsigned SN76496_clock(const void* interface) { return ((struct SN76496interface*)interface)->baseclock; }
unsigned SN76496_num(const void* interface) { return ((struct SN76496interface*)interface)->num; }
unsigned UPD7759_clock(const void* interface) { return ((struct UPD7759_interface*)interface)->clock_rate; }
unsigned ASTROCADE_clock(const void* interface) { return ((struct astrocade_interface*)interface)->baseclock; }
unsigned ASTROCADE_num(const void* interface) { return ((struct astrocade_interface*)interface)->num; }

struct sound_desc SOUND_DESC[] =
{
	{ SOUND_CUSTOM,     0,             0,               "Custom"    },
	{ SOUND_SAMPLES,    0,             0,               "Samples"   },
	{ SOUND_DAC,        DAC_num,       0,               "DAC"       },
	{ SOUND_AY8910,     AY8910_num,    AY8910_clock,    "AY-8910"   },
	{ SOUND_YM2203,     YM2203_num,    YM2203_clock,    "YM-2203"   },
	{ SOUND_YM2151,     YM2151_num,    YM2151_clock,    "YM-2151"   },
	{ SOUND_YM2151_ALT, YM2151_num,    YM2151_clock,    "YM-2151a"  },
	{ SOUND_YM2413,     YM2413_num,    YM2413_clock,    "YM-2413"   },
	{ SOUND_YM2610,     YM2610_num,    YM2610_clock,    "YM-2610"   },
	{ SOUND_YM3812,     YM3812_num,    YM3812_clock,    "YM-3812"   },
	{ SOUND_YM3526,     YM3812_num,    YM3812_clock,    "YM-3526"   },
	{ SOUND_SN76496,    SN76496_num,   SN76496_clock,   "SN76496"   },
	{ SOUND_POKEY,      POKEY_num,     POKEY_clock,     "Pokey"     },
	{ SOUND_NAMCO,      0,             0,               "Namco"     },
	{ SOUND_NES,        NES_num,       NES_clock,       "NES"       },
	{ SOUND_TMS5220,    0,             TMS5220_clock,   "TMS5520"   },
	{ SOUND_VLM5030,    0,             VLM5030_clock,   "VLM5030"   },
	{ SOUND_ADPCM,      ADPCM_num,     0,               "ADPCM"     },
	{ SOUND_OKIM6295,   OKIM6295_num,  0,               "OKI6295"   },
	{ SOUND_MSM5205,    MSM5205_num,   0,               "MSM5205"   },
	{ SOUND_UPD7759,    0,             UPD7759_clock,   "uPD7759"   },
	{ SOUND_HC55516,    HC55516_num,   0,               "HC55516"   },
	{ SOUND_ASTROCADE,  ASTROCADE_num, ASTROCADE_clock, "Astrocade" },
	{ SOUND_K007232,    0,             0,               "007232"    },
	{ 0,0 }
};





const char *info_cpu_name(const struct MachineCPU *cpu)
{
	int k;


	if (cpu->cpu_type == 0) return "";

	k = 0;
	while (CPU_DESC[k].cpu_type && CPU_DESC[k].cpu_type != (cpu->cpu_type & ~CPU_FLAGS_MASK))
		k++;

	if (CPU_DESC[k].cpu_type)
		return CPU_DESC[k].desc;
	else
		return "unknown";
}

const char *info_sound_name(const struct MachineSound *sound)
{
	int k;


	if (sound->sound_type == 0) return "";

	k = 0;
	while (SOUND_DESC[k].sound_type && SOUND_DESC[k].sound_type != sound->sound_type)
		k++;

	if (SOUND_DESC[k].sound_type)
		return SOUND_DESC[k].desc;
	else
		return "unknown";
}

int info_sound_num(const struct MachineSound *sound)
{
	int k;


	if (sound->sound_type == 0) return 0;

	k = 0;
	while (SOUND_DESC[k].sound_type && SOUND_DESC[k].sound_type != sound->sound_type)
		k++;

	if (SOUND_DESC[k].sound_type && SOUND_DESC[k].num)
		return (*SOUND_DESC[k].num)(sound->sound_interface);
	else
		return 0;
}

int info_sound_clock(const struct MachineSound *sound)
{
	int k;


	if (sound->sound_type == 0) return 0;

	k = 0;
	while (SOUND_DESC[k].sound_type && SOUND_DESC[k].sound_type != sound->sound_type)
		k++;

	if (SOUND_DESC[k].sound_type && SOUND_DESC[k].clock)
		return (*SOUND_DESC[k].clock)(sound->sound_interface);
	else
		return 0;
}



/* ------------------------------------------------------------------------*/
/* Output format indentation */

/* Indent one level */
#define INDENT "\t"

/* Level format
  L list
  1,2 level
  B,S,E list Begin, list Separator, list End
*/

#if 0
/* Binary */
#define L1B "("
#define L1P " "
#define L1N ""
#define L1E ")"
#define L2B "("
#define L2P " "
#define L2N ""
#define L2E ")"

#else
/* Indentation */

/* One per line */
#define L1B " (\n"
#define L1P INDENT
#define L1N "\n"
#define L1E ")\n\n"

#if 1
/* All in line */
#define L2B " ("
#define L2P " "
#define L2N ""
#define L2E " )"
#else
/* One per line */
#define L2B " (\n"
#define L2P INDENT INDENT
#define L2N "\n"
#define L2E INDENT ")"
#endif

#endif

/* ------------------------------------------------------------------------*/
/* Print the MAME info record for a game */

static void print_c_string(FILE* out, const char* s) {
	fprintf(out, "\"");
	while (*s) {
		switch (*s) {
			case '\a' : fprintf(out, "\\a"); break;
			case '\b' : fprintf(out, "\\b"); break;
			case '\f' : fprintf(out, "\\f"); break;
			case '\n' : fprintf(out, "\\n"); break;
			case '\r' : fprintf(out, "\\r"); break;
			case '\t' : fprintf(out, "\\t"); break;
			case '\v' : fprintf(out, "\\v"); break;
			case '\\' : fprintf(out, "\\\\"); break;
			case '\"' : fprintf(out, "\\\""); break;
			default:
				if ((unsigned char)*s>=(unsigned char)' ' && (unsigned char)*s<=(unsigned char)'')
					fprintf(out, "%c", *s);
				else
					fprintf(out, "\\x%2x", (int)*s);
		}
		++s;
	}
	fprintf(out, "\"");
}

static void print_game_input(FILE* out, const struct GameDriver* game) {
	const struct InputPort* input = game->input_ports;
	int nplayer = 0;
	const char* control = 0;
	int nbutton = 0;
	int ncoin = 0;

	while ((input->type & ~IPF_MASK) != IPT_END) {
		switch (input->type & IPF_PLAYERMASK) {
			case IPF_PLAYER1:
				if (nplayer<1) nplayer = 1;
				break;
			case IPF_PLAYER2:
				if (nplayer<2) nplayer = 2;
				break;
			case IPF_PLAYER3:
				if (nplayer<3) nplayer = 3;
				break;
			case IPF_PLAYER4:
				if (nplayer<4) nplayer = 4;
				break;
		}
		switch (input->type & ~IPF_MASK) {
			case IPT_JOYSTICK_UP:
			case IPT_JOYSTICK_DOWN:
			case IPT_JOYSTICK_LEFT:
			case IPT_JOYSTICK_RIGHT:
			case IPT_JOYSTICKRIGHT_UP:
				if (input->type & IPF_2WAY)
					control = "joy2way";
				else if	(input->type & IPF_4WAY)
					control = "joy4way";
				else
					control = "joy8way";
				break;
			case IPT_JOYSTICKRIGHT_DOWN:
			case IPT_JOYSTICKRIGHT_LEFT:
			case IPT_JOYSTICKRIGHT_RIGHT:
			case IPT_JOYSTICKLEFT_UP:
			case IPT_JOYSTICKLEFT_DOWN:
			case IPT_JOYSTICKLEFT_LEFT:
			case IPT_JOYSTICKLEFT_RIGHT:
				if (input->type & IPF_2WAY)
					control = "doublejoy2way";
				else if	(input->type & IPF_4WAY)
					control = "doublejoy4way";
				else
					control = "doublejoy8way";
				break;
			case IPT_BUTTON1:
				if (nbutton<1) nbutton = 1;
				break;
			case IPT_BUTTON2:
				if (nbutton<2) nbutton = 2;
				break;
			case IPT_BUTTON3:
				if (nbutton<3) nbutton = 3;
				break;
			case IPT_BUTTON4:
				if (nbutton<4) nbutton = 4;
				break;
			case IPT_BUTTON5:
				if (nbutton<5) nbutton = 5;
				break;
			case IPT_BUTTON6:
				if (nbutton<6) nbutton = 6;
				break;
			case IPT_BUTTON7:
				if (nbutton<7) nbutton = 7;
				break;
			case IPT_BUTTON8:
				if (nbutton<8) nbutton = 8;
				break;
			case IPT_PADDLE:
				control = "paddle";
				break;
			case IPT_DIAL:
				control = "dial";
				break;
			case IPT_TRACKBALL_X:
			case IPT_TRACKBALL_Y:
				control = "trackball";
				break;
			case IPT_AD_STICK_X:
			case IPT_AD_STICK_Y:
				control = "stick";
				break;
			case IPT_COIN1:
				if (ncoin < 1) ncoin = 1;
				break;
			case IPT_COIN2:
				if (ncoin < 2) ncoin = 2;
				break;
			case IPT_COIN3:
				if (ncoin < 3) ncoin = 3;
				break;
			case IPT_COIN4:
				if (ncoin < 4) ncoin = 4;
				break;
		}
		++input;
	}

	fprintf(out, L1P "input" L2B);
	fprintf(out, L2P "players %d" L2N, nplayer );
	if (control)
		fprintf(out, L2P "control %s" L2N, control );
	if (nbutton)
		fprintf(out, L2P "buttons %d" L2N, nbutton );
	if (ncoin)
		fprintf(out, L2P "coins %d" L2N, ncoin );
	fprintf(out, "%s", L2E L1N);
}

static void print_game_rom(FILE* out, const struct GameDriver* game) {
	const struct RomModule* rom = game->rom;

	if (game->clone_of && game->clone_of != game) {
		fprintf(out, L1P "romof %s" L1N, game->clone_of->name);
	}

	while (rom->name || rom->offset || rom->length) {
		rom++;

		while (rom->length) {
			char name[100];
			int length,crc;

			sprintf(name,rom->name,game->name);
			crc = rom->crc;

			length = 0;
			do {
				if (rom->name == (char *)-1)
					length = 0;	/* restart */
				length += rom->length & ~ROMFLAG_MASK;
				rom++;
			} while (rom->length && (rom->name == 0 || rom->name == (char *)-1));

			fprintf(out, L1P "rom" L2B);
			if (*name)
				fprintf(out, L2P "name %s" L2N, name );
			fprintf(out, L2P "size %d" L2N, length );
			fprintf(out, L2P "crc %08x" L2N, crc );
			fprintf(out, "%s", L2E L1N);
		}
	}
}

static void print_game_sample(FILE* out, const struct GameDriver* game) {
	const struct MachineDriver* driver = game->drv;
	const struct MachineSound* sound = driver->sound;
	int j;
	if (game->samplenames != 0 && game->samplenames[0] != 0) {
		int k = 0;
		if (game->samplenames[k][0]=='*') {
			/* output sampleof only if different from game name */
			if (strcmp(game->samplenames[k] + 1, game->name)!=0) {
				fprintf(out, L1P "sampleof %s" L1N, game->samplenames[k] + 1);
			}
			++k;
		}
		while (game->samplenames[k] != 0) {
			/* Check if is not empty */
			if (*game->samplenames[k]) {
				/* Check if sample is duplicate */
				int l = 0;
				while (l<k && strcmp(game->samplenames[k],game->samplenames[l])!=0)
					++l;
				if (l==k) {
					fprintf(out, L1P "sample %s" L1N, game->samplenames[k]);
				}
			}
			++k;
		}
	}
	/* YM3812 Samples */
	for(j=0;j<MAX_SOUND;++j) {
		if (sound[j].sound_type==SOUND_YM3812) {
			break;
		}
	}
	if (j<MAX_SOUND) {
		fprintf(out, L1P "sampleof ym3812" L1N);
		fprintf(out, L1P "sample bassdrum.sam" L1N);
		fprintf(out, L1P "sample snardrum.sam" L1N);
		fprintf(out, L1P "sample tomtom.sam" L1N);
		fprintf(out, L1P "sample topcmbal.sam" L1N);
		fprintf(out, L1P "sample hihat.sam" L1N);
	}
}

static void print_game_micro(FILE* out, const struct GameDriver* game)
{
	const struct MachineDriver* driver = game->drv;
	const struct MachineCPU* cpu = driver->cpu;
	const struct MachineSound* sound = driver->sound;
	int j;

	for(j=0;j<MAX_CPU;++j)
	{
		if (cpu[j].cpu_type!=0)
		{
			fprintf(out, L1P "chip" L2B);
			if (cpu[j].cpu_type & CPU_AUDIO_CPU)
				fprintf(out, L2P "type audio" L2N);
			else
				fprintf(out, L2P "type cpu" L2N);

			fprintf(out, L2P "name %s" L2N, info_cpu_name(&cpu[j]));

			fprintf(out, L2P "clock %d" L2N, cpu[j].cpu_clock);
			fprintf(out, "%s", L2E L1N);
		}
	}

	for(j=0;j<MAX_SOUND;++j) if (sound[j].sound_type)
	{
		if (sound[j].sound_type)
		{
			int num = info_sound_num(&sound[j]);
			int l;

			if (num == 0) num = 1;

			for(l=0;l<num;++l)
			{
				fprintf(out, L1P "chip" L2B);
				fprintf(out, L2P "type audio" L2N);
				fprintf(out, L2P "name %s" L2N, info_sound_name(&sound[j]));
				if (info_sound_num(&sound[j]))
					fprintf(out, L2P "clock %d" L2N, info_sound_num(&sound[j]));
				fprintf(out, "%s", L2E L1N);
			}
		}
	}
}

static void print_game_video(FILE* out, const struct GameDriver* game) {
	const struct MachineDriver* driver = game->drv;

	fprintf(out, L1P "video" L2B);
	if (driver->video_attributes & VIDEO_TYPE_VECTOR)
		fprintf(out, L2P "screen vector" L2N);
	else {
		int dx;
		int dy;
		fprintf(out, L2P "screen raster" L2N);
		if (game->orientation & ORIENTATION_SWAP_XY) {
			dx = driver->visible_area.max_y - driver->visible_area.min_y + 1;
			dy = driver->visible_area.max_x - driver->visible_area.min_x + 1;
		} else {
			dx = driver->visible_area.max_x - driver->visible_area.min_x + 1;
			dy = driver->visible_area.max_y - driver->visible_area.min_y + 1;
		}
		fprintf(out, L2P "x %d" L2N, dx);
		fprintf(out, L2P "y %d" L2N, dy);
	}
	fprintf(out, L2P "colors %d" L2N, driver->total_colors);
	fprintf(out, L2P "freq %d" L2N, driver->frames_per_second);
	fprintf(out, "%s", L2E L1N);
}

static void print_game_driver(FILE* out, const struct GameDriver* game) {
	const struct MachineDriver* driver = game->drv;

	fprintf(out, L1P "driver" L2B);
	if (game->flags & GAME_NOT_WORKING)
		fprintf(out, L2P "status preliminary" L2N);
	else
		fprintf(out, L2P "status good" L2N);

	if (game->flags & GAME_WRONG_COLORS)
		fprintf(out, L2P "color preliminary" L2N);
	else if (game->flags & GAME_IMPERFECT_COLORS)
		fprintf(out, L2P "color imperfect" L2N);
	else
		fprintf(out, L2P "color good" L2N);

	if (game->hiscore_load && game->hiscore_save)
		fprintf(out, L2P "hiscore good" L2N);
	else
		fprintf(out, L2P "hiscore preliminary" L2N);

	if (driver->video_attributes & VIDEO_SUPPORTS_16BIT)
		fprintf(out, L2P "colordeep 16" L2N);
	else
		fprintf(out, L2P "colordeep 8" L2N);

	fprintf(out, L2P "credits ");
	print_c_string(out, game->credits );
	fprintf(out, "%s", L2N);

	fprintf(out, "%s", L2E L1N);
}

static void print_game_info(FILE* out, const struct GameDriver* game) {

	fprintf(out, "game" L1B );

	fprintf(out, L1P "name %s" L1N, game->name );

	if (game->description) {
		fprintf(out, L1P "description ");
		print_c_string(out, game->description );
		fprintf(out, "%s", L1N);
	}

	/* print the year only if is a number */
	if (game->year && strspn(game->year,"0123456789")==strlen(game->year))
		fprintf(out, L1P "year %s" L1N, game->year );

	if (game->manufacturer) {
		fprintf(out, L1P "manufacturer ");
		print_c_string(out, game->manufacturer );
		fprintf(out, "%s", L1N);
	}

	/* print the cloneof only if is not a neogeo game */
	if (game->clone_of && game->clone_of != game && strcmp(game->clone_of->name,"neogeo")!=0) {
		fprintf(out, L1P "cloneof %s" L1N, game->clone_of->name);
	}

	print_game_rom(out,game);
	print_game_sample(out,game);
	print_game_micro(out,game);
	print_game_video(out,game);
	print_game_input(out,game);
	print_game_driver(out,game);

	fprintf(out, "%s", L1E);
}

/* ------------------------------------------------------------------------*/
/* Print all the MAME info database */
void print_mame_info(FILE* out, const struct GameDriver* games[]) {
	int j;

	for(j=0;games[j];++j)
		print_game_info( out, games[j] );

	/* addictional fixed record */
	fprintf(out, "resource" L1B);
	fprintf(out, L1P "name neogeo" L1N);
	fprintf(out, L1P "description \"Neo Geo BIOS\"" L1N);
	fprintf(out, L1P "rom" L2B);
	fprintf(out, L2P "name neo-geo.rom" L2N);
	fprintf(out, L2P "size 131072" L2N);
	fprintf(out, L2P "crc 9036d879" L2N);
	fprintf(out, "%s", L2E L1N);
	fprintf(out, L1P "rom" L2B);
	fprintf(out, L2P "name ng-sm1.rom" L2N);
	fprintf(out, L2P "size 131072" L2N);
	fprintf(out, L2P "crc 97cf998b" L2N);
	fprintf(out, "%s", L2E L1N);
	fprintf(out, L1P "rom" L2B);
	fprintf(out, L2P "name ng-sfix.rom" L2N);
	fprintf(out, L2P "size 131072" L2N);
	fprintf(out, L2P "crc 354029fc" L2N);
	fprintf(out, "%s", L2E L1N);
	fprintf(out, "%s", L1E);

	fprintf(out, "resource" L1B);
	fprintf(out, L1P "name ym3812" L1N);
	fprintf(out, L1P "description \"YM-3812 samples\"" L1N);
	fprintf(out, L1P "sample bassdrum.sam" L1N);
	fprintf(out, L1P "sample snardrum.sam" L1N);
	fprintf(out, L1P "sample tomtom.sam" L1N);
	fprintf(out, L1P "sample topcmbal.sam" L1N);
	fprintf(out, L1P "sample hihat.sam" L1N);
	fprintf(out, "%s", L1E);
}