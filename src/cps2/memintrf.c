/******************************************************************************

	memintrf.c

	CPS2 Memory interface function

******************************************************************************/

#include "cps2.h"


#define M68K_AMASK 0x00ffffff
#define Z80_AMASK 0x0000ffff

#define READ_BYTE(mem, offset)			mem[offset ^ 1]
#define READ_WORD(mem, offset)			*(u16 *)&mem[offset]
#define WRITE_BYTE(mem, offset, data)	mem[offset ^ 1] = data
#define WRITE_WORD(mem, offset, data)	*(u16 *)&mem[offset] = data

#define str_cmp(s1, s2)		strncasecmp(s1, s2, strlen(s2))

enum
{
	REGION_CPU1 = 0,
	REGION_CPU2,
	REGION_GFX1,
	REGION_SOUND1,
	REGION_USER1,
	REGION_SKIP
};

#define MAX_CPU1ROM		8
#define MAX_CPU2ROM		8
#define MAX_SND1ROM		8
#define MAX_USR1ROM		8


/******************************************************************************
	Global variable
******************************************************************************/

u8 *memory_region_cpu1;
u8 *memory_region_cpu2;
u8 *memory_region_gfx1;
u8 *memory_region_sound1;
u8 *memory_region_user1;

u32 memory_length_cpu1;
u32 memory_length_cpu2;
u32 memory_length_gfx1;
u32 memory_length_sound1;
u32 memory_length_user1;

u32 gfx_total_elements[3];
u8 *gfx_pen_usage[3];

u8  ALIGN_DATA cps1_ram[0x10000];
u8  ALIGN_DATA cps2_ram[0x4000 + 2];
u16 ALIGN_DATA cps1_gfxram[0x30000 >> 1];
u16 ALIGN_DATA cps1_output[0x100 >> 1];

u16 ALIGN_DATA cps2_objram[2][0x2000 >> 1];
u16 ALIGN_DATA cps2_output[0x10 >> 1];

u8 *qsound_sharedram1;
u8 *qsound_sharedram2;

static u32 crypt_key[2];
void cps2_decrypt(const u32 *master_key);

/******************************************************************************
	Local struct/variable
******************************************************************************/

static struct rom_t cpu1rom[MAX_CPU1ROM];
static struct rom_t cpu2rom[MAX_CPU2ROM];
static struct rom_t snd1rom[MAX_SND1ROM];
static struct rom_t usr1rom[MAX_USR1ROM];

static int num_cpu1rom;
static int num_cpu2rom;
static int num_snd1rom;
static int num_usr1rom;

static u8 *static_ram1;
static u8 *static_ram2;
static u8 *static_ram3;
static u8 *static_ram4;
static u8 *static_ram5;
static u8 *static_ram6;


/******************************************************************************
	Display error message
******************************************************************************/

/*------------------------------------------------------
	Memory allocation error message
------------------------------------------------------*/

static void error_memory(const char *mem_name)
{
	zip_close();
	msg_printf("ERROR: Could not allocate %s memory.\n", mem_name);
	msg_printf("Press any button.\n");
	pad_wait_press(PAD_WAIT_INFINITY);
	Loop = LOOP_BROWSER;
}


/*------------------------------------------------------
	File open error message
------------------------------------------------------*/

static void error_file(const char *file_name)
{
	zip_close();
	msg_printf("ERROR: Could not open file. \"%s\"\n", file_name);
	msg_printf("Press any button.\n");
	pad_wait_press(PAD_WAIT_INFINITY);
	Loop = LOOP_BROWSER;
}


/*------------------------------------------------------
	ROM file error message
------------------------------------------------------*/

static void error_rom(const char *rom_name)
{
	zip_close();
	msg_printf("ERROR: File not found or CRC32 not correct. \"%s\"\n", rom_name);
	msg_printf("Press any button.\n");
	pad_wait_press(PAD_WAIT_INFINITY);
	Loop = LOOP_BROWSER;
}



/******************************************************************************
	ROM Load
******************************************************************************/

/*--------------------------------------------------------
	CPU1 (M68000 program ROM / encrypted)
--------------------------------------------------------*/

static int load_rom_cpu1(void)
{
	int i;
	char fname[32], *parent;
/*
	if ((memory_region_cpu1 = memalign(MEM_ALIGN, memory_length_cpu1)) == NULL)
	{
		error_memory("REGION_CPU1");
		return 0;
	}
	memset(memory_region_cpu1, 0, memory_length_cpu1);
*/
	if ((memory_region_cpu1 = memalign(MEM_ALIGN, 0x400000)) == NULL)
	{
		error_memory("REGION_CPU1");
		return 0;
	}
	memset(memory_region_cpu1, 0, 0x400000);

	parent = strlen(parent_name) ? parent_name : NULL;

	for (i = 0; i < num_cpu1rom; )
	{
		if (file_open(game_name, parent, cpu1rom[i].crc, fname) == -1)
		{
			error_rom("CPU1");
			return 0;
		}

		msg_printf("Loading \"%s\"\n", fname);

		i = rom_load(cpu1rom, memory_region_cpu1, i, num_cpu1rom);

		file_close();
	}

	return 1;
}


/*--------------------------------------------------------
	CPU2 (Z80 program ROM)
--------------------------------------------------------*/

static int load_rom_cpu2(void)
{
	int i;
	char fname[32], *parent;

	if ((memory_region_cpu2 = memalign(MEM_ALIGN, memory_length_cpu2)) == NULL)
	{
		error_memory("REGION_CPU2");
		return 0;
	}
	memset(memory_region_cpu2, 0, memory_length_cpu2);

	parent = strlen(parent_name) ? parent_name : NULL;

	for (i = 0; i < num_cpu2rom; )
	{
		if (file_open(game_name, parent, cpu2rom[i].crc, fname) == -1)
		{
			error_rom("CPU2");
			return 0;
		}

		msg_printf("Loading \"%s\"\n", fname);

		i = rom_load(cpu2rom, memory_region_cpu2, i, num_cpu2rom);

		file_close();
	}

	return 1;
}


/*--------------------------------------------------------
	GFX1 (graphic ROM)
--------------------------------------------------------*/

static int load_rom_gfx1(void)
{
	int fd, found = 0;

	msg_printf("Loading cache information data...\n");

	gfx_total_elements[TILE08] = (memory_length_gfx1 - 0x800000) >> 6;
	gfx_total_elements[TILE16] = memory_length_gfx1 >> 7;
	gfx_total_elements[TILE32] = (memory_length_gfx1 - 0x800000) >> 9;
	
	block_start = 0;
    block_size = 0;
    block_capacity = 0;

	if (gfx_total_elements[TILE08] > 0x10000) gfx_total_elements[TILE08] = 0x10000;
	if (gfx_total_elements[TILE32] > 0x10000) gfx_total_elements[TILE32] = 0x10000;

	if ((gfx_pen_usage[TILE08] = memalign(MEM_ALIGN, gfx_total_elements[TILE08])) == NULL)
	{
		error_memory("GFX_PEN_USAGE (tile8)");
		return 0;
	}
	if ((gfx_pen_usage[TILE16] = memalign(MEM_ALIGN, gfx_total_elements[TILE16])) == NULL)
	{
		error_memory("GFX_PEN_USAGE (tile16)");
		return 0;
	}
	if ((gfx_pen_usage[TILE32] = memalign(MEM_ALIGN, gfx_total_elements[TILE32])) == NULL)
	{
		error_memory("GFX_PEN_USAGE (tile32)");
		return 0;
	}

	if ((fd = cachefile_open()) >= 0)
	{
		char version_str[8];

		read(fd, version_str, 8);

		if (strcmp(version_str, "CPS2XC0") == 0)
		{
			read(fd, &block_start, 4);
			read(fd, &block_size, 4);
			read(fd, &block_capacity, 4);
			read(fd, block_offset, 0x200 * sizeof(u32));
			read(fd, gfx_pen_usage[TILE08], gfx_total_elements[TILE08]);
			read(fd, gfx_pen_usage[TILE16], gfx_total_elements[TILE16]);
			read(fd, gfx_pen_usage[TILE32], gfx_total_elements[TILE32]);
			found = 1;
		}
		close(fd);
	}

	if (!found)
	{
		msg_printf("ERROR: unsupported cache file.\n");
		msg_printf("Please rebuild cache file.\n");
		msg_printf("Press any button.\n");
		pad_wait_press(PAD_WAIT_INFINITY);
		Loop = LOOP_EXIT;
		return 0;
	}

	memory_length_gfx1 = driver->cache_size;

	if (cache_start() == 0)
	{
		msg_printf("Press any button.\n");
		pad_wait_press(PAD_WAIT_INFINITY);
		Loop = LOOP_EXIT;
		return 0;
	}

	return 1;
}


/*--------------------------------------------------------
	SOUND1 (Q-SOUND PCM ROM)
--------------------------------------------------------*/

static int load_rom_sound1(void)
{
	int i;
	char fname[32], *parent;

	if ((memory_region_sound1 = memalign(MEM_ALIGN, memory_length_sound1)) == NULL)
	{
		error_memory("REGION_SOUND1");
		return 0;
	}
	memset(memory_region_sound1, 0, memory_length_sound1);

	parent = strlen(parent_name) ? parent_name : NULL;

	for (i = 0; i < num_snd1rom; )
	{
		if (file_open(game_name, parent_name, snd1rom[i].crc, fname) == -1)
		{
			error_rom("SOUND1");
			return 0;
		}

		msg_printf("Loading Sound Rom \"%s\"\n", fname);

		i = rom_load(snd1rom, memory_region_sound1, i, num_snd1rom);

		file_close();
	}

	return 1;
}


/*--------------------------------------------------------
	USER1 (MC68000 ROM xor table)
--------------------------------------------------------*/

static int load_rom_user1(void)
{
	if (memory_length_user1)
	{
		int i = -1;

		if ((memory_region_user1 = memalign(MEM_ALIGN, memory_length_user1)) == NULL)
		{
			error_memory("REGION_USER1");
			return 0;
		}
		memset(memory_region_user1, 0, memory_length_user1);
		
		if (option_xorrom || (!crypt_key[0] && !crypt_key[1])) {
			char fname[32], *parent;

			parent = strlen(parent_name) ? parent_name : NULL;

			for (i = 0; i < num_usr1rom; )
			{
				if ((usr1rom[i].crc == 0) || (file_open(game_name, parent, usr1rom[i].crc, fname) == -1))
				{
					if(crypt_key[0] || crypt_key[1]) {
						//msg_printf("Crypt key: 0x%08x, 0x%08x\n", crypt_key[0], crypt_key[1]);
						i = -1;
						break;
					}
					error_rom("USER1");
					return 0;
				}

				msg_printf("Loading \"%s\"\n", fname);

				i = rom_load(usr1rom, memory_region_user1, i, num_usr1rom);

				file_close();
			}
		}

		if(i == -1) {
			cps2_decrypt(crypt_key);
		} else {
			u16 *rom = (u16 *)memory_region_cpu1;
			u16 *xor = (u16 *)memory_region_user1;

			for (i = (memory_length_user1 >> 1) - 1; i >= 0; --i) 
				xor[i] ^= rom[i];
		}
	}

	return 1;
}


/*--------------------------------------------------------
	ROM���ê�ǣ����٣��������
--------------------------------------------------------*/

static int load_rom_info(const char *game_name)
{
	FILE *fp;
	char path[MAX_PATH];
	char buf[256];
	int rom_start = 0;
	int region = 0;

	num_cpu1rom = 0;
	num_cpu2rom = 0;
	num_snd1rom = 0;
	num_usr1rom = 0;

	machine_driver_type  = 0;
	machine_input_type   = 0;
	machine_init_type    = 0;
	machine_screen_type  = 0;
	
	sprintf(path, "%sconfig/rominfo.cps2", launchDir);

	if ((fp = fopen(path, "r")) != NULL)
	{
		while (fgets(buf, 255, fp))
		{
			if (buf[0] == '/' && buf[1] == '/')
				continue;

			if (buf[0] != '\t')
			{
				if (buf[0] == '\r' || buf[0] == '\n')
				{
					// ����
					continue;
				}
				else if (str_cmp(buf, "FILENAME(") == 0)
				{
					char *name, *parent;
					char *machine, *input, *init, *rotate;

					strtok(buf, " ");
					name    = strtok(NULL, " ,");
					parent  = strtok(NULL, " ,");
					machine = strtok(NULL, " ,");
					input   = strtok(NULL, " ,");
					init    = strtok(NULL, " ,");
					rotate  = strtok(NULL, " ");

					if (strcasecmp(name, game_name) == 0)
					{
#if RELEASE
						if (stricmp(name, "hsf2d") == 0)
						{
							fclose(fp);
							return 1;
						}
#endif
						if (str_cmp(parent, "cps2") == 0)
							parent_name[0] = '\0';
						else
							strcpy(parent_name, parent);

						sscanf(machine, "%d", &machine_driver_type);
						sscanf(input, "%d", &machine_input_type);
						sscanf(init, "%d", &machine_init_type);
						sscanf(rotate, "%d", &machine_screen_type);
						rom_start = 1;
					}
				}
				else if (rom_start && str_cmp(buf, "END") == 0)
				{
					fclose(fp);
					return 0;
				}
			}
			else if (rom_start)
			{
				if (str_cmp(&buf[1], "REGION(") == 0)
				{
					char *size, *type, *flag;

					strtok(&buf[1], " ");
					size = strtok(NULL, " ,");
					type = strtok(NULL, " ,");
					flag = strtok(NULL, " ");

					if (strcmp(type, "CPU1") == 0)
					{
						sscanf(size, "%x", &memory_length_cpu1);
						region = REGION_CPU1;
					}
					else if (strcmp(type, "CPU2") == 0)
					{
						sscanf(size, "%x", &memory_length_cpu2);
						region = REGION_CPU2;
					}
					else if (strcmp(type, "GFX1") == 0)
					{
						sscanf(size, "%x", &memory_length_gfx1);
						region = REGION_SKIP;
					}
					else if (strcmp(type, "SOUND1") == 0)
					{
						sscanf(size, "%x", &memory_length_sound1);
						region = REGION_SOUND1;
					}
					else if (strcmp(type, "USER1") == 0)
					{
						sscanf(size, "%x", &memory_length_user1);
						crypt_key[0] = crypt_key[1] = 0;
						region = REGION_USER1;
					}
					else
					{
						region = REGION_SKIP;
					}
				}
				else if (str_cmp(&buf[1], "ROM(") == 0)
				{
					char *type, *offset, *length, *crc;

					strtok(&buf[1], " ");
					type   = strtok(NULL, " ,");
					offset = strtok(NULL, " ,");
					length = strtok(NULL, " ,");
					crc    = strtok(NULL, " ");

					switch (region)
					{
					case REGION_CPU1:
						sscanf(type, "%x", &cpu1rom[num_cpu1rom].type);
						sscanf(offset, "%x", &cpu1rom[num_cpu1rom].offset);
						sscanf(length, "%x", &cpu1rom[num_cpu1rom].length);
						sscanf(crc, "%x", &cpu1rom[num_cpu1rom].crc);
						cpu1rom[num_cpu1rom].group = 0;
						cpu1rom[num_cpu1rom].skip = 0;
						num_cpu1rom++;
						break;

					case REGION_CPU2:
						sscanf(type, "%x", &cpu2rom[num_cpu2rom].type);
						sscanf(offset, "%x", &cpu2rom[num_cpu2rom].offset);
						sscanf(length, "%x", &cpu2rom[num_cpu2rom].length);
						sscanf(crc, "%x", &cpu2rom[num_cpu2rom].crc);
						cpu2rom[num_cpu2rom].group = 0;
						cpu2rom[num_cpu2rom].skip = 0;
						num_cpu2rom++;
						break;

					case REGION_SOUND1:
						sscanf(type, "%x", &snd1rom[num_snd1rom].type);
						sscanf(offset, "%x", &snd1rom[num_snd1rom].offset);
						sscanf(length, "%x", &snd1rom[num_snd1rom].length);
						sscanf(crc, "%x", &snd1rom[num_snd1rom].crc);
						snd1rom[num_snd1rom].group = 0;
						snd1rom[num_snd1rom].skip = 0;
						num_snd1rom++;
						break;

					case REGION_USER1:
						sscanf(type, "%x", &usr1rom[num_usr1rom].type);
						sscanf(offset, "%x", &usr1rom[num_usr1rom].offset);
						sscanf(length, "%x", &usr1rom[num_usr1rom].length);
						sscanf(crc, "%x", &usr1rom[num_usr1rom].crc);
						usr1rom[num_usr1rom].group = 0;
						usr1rom[num_usr1rom].skip = 0;
						num_usr1rom++;
						break;
					}
				}
				else if (str_cmp(&buf[1], "ROMX(") == 0)
				{
					char *type, *offset, *length, *crc;
					char *group, *skip;

					strtok(&buf[1], " ");
					type   = strtok(NULL, " ,");
					offset = strtok(NULL, " ,");
					length = strtok(NULL, " ,");
					crc    = strtok(NULL, " ,");
					group  = strtok(NULL, " ,");
					skip   = strtok(NULL, " ");

					switch (region)
					{
					case REGION_CPU1:
						sscanf(type, "%x", &cpu1rom[num_cpu1rom].type);
						sscanf(offset, "%x", &cpu1rom[num_cpu1rom].offset);
						sscanf(length, "%x", &cpu1rom[num_cpu1rom].length);
						sscanf(crc, "%x", &cpu1rom[num_cpu1rom].crc);
						sscanf(group, "%x", &cpu1rom[num_cpu1rom].group);
						sscanf(skip, "%x", &cpu1rom[num_cpu1rom].skip);
						num_cpu1rom++;
						break;

					case REGION_CPU2:
						sscanf(type, "%x", &cpu2rom[num_cpu2rom].type);
						sscanf(offset, "%x", &cpu2rom[num_cpu2rom].offset);
						sscanf(length, "%x", &cpu2rom[num_cpu2rom].length);
						sscanf(crc, "%x", &cpu2rom[num_cpu2rom].crc);
						sscanf(group, "%x", &cpu2rom[num_cpu2rom].group);
						sscanf(skip, "%x", &cpu2rom[num_cpu2rom].skip);
						num_cpu2rom++;
						break;

					case REGION_SOUND1:
						sscanf(type, "%x", &snd1rom[num_snd1rom].type);
						sscanf(offset, "%x", &snd1rom[num_snd1rom].offset);
						sscanf(length, "%x", &snd1rom[num_snd1rom].length);
						sscanf(crc, "%x", &snd1rom[num_snd1rom].crc);
						sscanf(group, "%x", &snd1rom[num_snd1rom].group);
						sscanf(skip, "%x", &snd1rom[num_snd1rom].skip);
						num_snd1rom++;
						break;

					case REGION_USER1:
						sscanf(type, "%x", &usr1rom[num_usr1rom].type);
						sscanf(offset, "%x", &usr1rom[num_usr1rom].offset);
						sscanf(length, "%x", &usr1rom[num_usr1rom].length);
						sscanf(crc, "%x", &usr1rom[num_usr1rom].crc);
						sscanf(group, "%x", &usr1rom[num_usr1rom].group);
						sscanf(skip, "%x", &usr1rom[num_usr1rom].skip);
						num_usr1rom++;
						break;
					}
				}
				else if (str_cmp(&buf[1], "KEY(") == 0)
				{
					char *key1, *key2;

					strtok(&buf[1], " ");
					key1 = strtok(NULL, " ,");
					key2 = strtok(NULL, " ,");

					switch (region)
					{
					case REGION_USER1:
						sscanf(key1, "%x", &crypt_key[0]);
						sscanf(key2, "%x", &crypt_key[1]);
						break;
					}
				}
			}
		}
		fclose(fp);
		return 2;
	}
	return 3;
}


/******************************************************************************
	Memory interface function
******************************************************************************/

/*------------------------------------------------------
	Memory interface initialize
-----------------------------------------------------*/

extern int screen_mode;

int memory_init(void)
{
	int i, res;

	memory_region_cpu1   = NULL;
	memory_region_cpu2   = NULL;
	memory_region_gfx1   = NULL;
	memory_region_sound1 = NULL;
	memory_region_user1  = NULL;

	memory_length_cpu1   = 0;
	memory_length_cpu2   = 0;
	memory_length_gfx1   = 0;
	memory_length_sound1 = 0;
	memory_length_user1  = 0;

	gfx_pen_usage[TILE08] = NULL;
	gfx_pen_usage[TILE16] = NULL;
	gfx_pen_usage[TILE32] = NULL;

	cache_init();
	pad_wait_clear();
	video_clear_screen();
	msg_screen_init();
	
	if(option_sound_enable) {
    	msg_printf("Sound: Enable\n");
    	msg_printf("Samplerate: %d Hz\n", 11025 << option_samplerate);
    } else
        msg_printf("Sound: Disable\n");

	if(screen_mode != 0) {
		msg_printf("TV-Out: %s mode\n", screen_mode == 1 ? "PAL" : "NTSC");
		if(option_rescale == 3) option_rescale = 2;
	}
    msg_printf("Rescale: %s\n", option_rescale == 0 ? "None" : option_rescale == 1 ? "Software rescale" :
		option_rescale == 2 ? "Hardware rescale" : "Hardware(horizontal only)");
	if(option_rescale == 0) {
		if(option_screen_position == 32) {
			msg_printf("Screen position: Center\n");
		} else {
			msg_printf("Screen position: %s(%d)\n", option_screen_position < 32 ? "Left" : "Right", abs(option_screen_position - 32));
		}
	}

	msg_printf("Line Scroll: %s\n", option_linescroll ? "On" : "Off");

    if(option_autoframeskip)
        msg_printf("Frame skip: Auto\n");
    else
        msg_printf("Frame skip: %d\n", option_frameskip);

    if(option_m68k_clock != 100)
	    msg_printf("M68000 main core clock: %d %%\n", option_m68k_clock);
    if(option_z80_clock != 100)
	    msg_printf("Z80 sound core clock: %d %%\n", option_z80_clock);

	msg_printf("Cache dir: %s\n", cache_dir);
	msg_printf("Game dir: %s\n", game_dir);

	if(option_showfps)
		msg_printf("Show FPS: On\n");

	if(option_showtitle)
		msg_printf("Show title: On\n");
		
	if(option_fullcache)
		msg_printf("Upper memory full use only cache data.\n");
	else
		msg_printf("Upper memory using cache and compressed block data.\n");

	if(option_tweak)
		msg_printf("Apply RAM timing tweak.\n");
	if(option_cpuspeed)
		msg_printf("GP2X CPU clock: %d MHz\n", option_cpuspeed);

	msg_printf("Checking ROM info...\n");

	if ((res = load_rom_info(game_name)) != 0)
	{
		switch (res)
		{
		case 1: msg_printf("ERROR: This game not supported.\n"); break;
		case 2: msg_printf("ERROR: ROM not found. (zip file name incorrect)\n"); break;
		case 3: msg_printf("ERROR: rominfo.cps2 not found.\n"); break;
		}
		msg_printf("Press any button.\n");
		pad_wait_press(PAD_WAIT_INFINITY);
		Loop = LOOP_BROWSER;
		return 0;
	}

	if (!strcmp(game_name, "ssf2ta")
	||	!strcmp(game_name, "ssf2tu")
	||	!strcmp(game_name, "ssf2tur1")
	||	!strcmp(game_name, "ssf2xj"))
	{
		strcpy(cache_parent_name, "ssf2t");
	}
	else if (!strcmp(game_name, "ssf2t"))
	{
		cache_parent_name[0] = '\0';
	}
	else
	{
		strcpy(cache_parent_name, parent_name);
	}

	i = 0;
	driver = NULL;
	while (CPS2_driver[i].name)
	{
		if (!strcmp(game_name, CPS2_driver[i].name) || !strcmp(cache_parent_name, CPS2_driver[i].name))
		{
			driver = &CPS2_driver[i];
			break;
		}
		i++;
	}
	if (!driver)
	{
		msg_printf("ERROR: Driver for \"%s\" not found.\n", game_name);
		Loop = LOOP_BROWSER;
		return 0;
	}

	if (parent_name[0])
		msg_printf("ROM set \"%s\" (parent: %s).\n", game_name, parent_name);
	else
		msg_printf("ROM set \"%s\".\n", game_name);

	if (cache_parent_name[0])
		msg_printf("Cache file \"%s.cache\".\n", cache_parent_name);
	else
		msg_printf("Cache file \"%s.cache\".\n", game_name);

	load_gamecfg(game_name);

	if (load_rom_gfx1() == 0) return 0;
	if (load_rom_cpu1() == 0) return 0;
	if (load_rom_user1() == 0) return 0;
	if (load_rom_cpu2() == 0) return 0;
	if (option_sound_enable)
	{
		if (load_rom_sound1() == 0) return 0;
	}

	static_ram1 = (u8 *)cps1_ram    - 0xff0000;
	static_ram2 = (u8 *)cps1_gfxram - 0x900000;
	static_ram3 = (u8 *)cps2_ram    - 0x660000;
	static_ram4 = (u8 *)cps2_output - 0x400000;
	static_ram5 = (u8 *)cps2_objram[0];
	static_ram6 = (u8 *)cps2_objram[1];

	qsound_sharedram1 = &memory_region_cpu2[0xc000];
	qsound_sharedram2 = &memory_region_cpu2[0xf000];

	memory_region_cpu2[0xd007] = 0x80;
	
	msg_printf("Done.\n");

	video_clear_screen();

	return 1;
}


/*------------------------------------------------------
	Memory interface shutdown
------------------------------------------------------*/

void memory_shutdown(void)
{
	cache_shutdown();

	if (gfx_pen_usage[TILE08]) free(gfx_pen_usage[TILE08]);
	if (gfx_pen_usage[TILE16]) free(gfx_pen_usage[TILE16]);
	if (gfx_pen_usage[TILE32]) free(gfx_pen_usage[TILE32]);

	if (memory_region_cpu1)   free(memory_region_cpu1);
	if (memory_region_cpu2)   free(memory_region_cpu2);
	// memory_region_gfx1 memory free is in emumain.c(Terminate function)
	if (memory_region_sound1) free(memory_region_sound1);
	if (memory_region_user1)  free(memory_region_user1);
}


/******************************************************************************
	M68000 memory read/write function
******************************************************************************/

/*------------------------------------------------------
	M68000 memory read (byte)
------------------------------------------------------*/

u8 m68000_read_memory_8(u32 offset)
{
	int shift;
	u16 mem_mask;

	offset &= M68K_AMASK;

	if (offset < 0x400000)
	{
		return READ_BYTE(memory_region_cpu1, offset);
	}

	shift = (~offset & 1) << 3;
	mem_mask = ~(0xff << shift);

	switch (offset & 0xff0000)
	{
	case 0x400000:
		return READ_BYTE(static_ram4, offset);

	case 0x610000:
		return qsound_sharedram1_r(offset >> 1, mem_mask) >> shift;

	case 0x660000:
		return READ_BYTE(static_ram3, offset);

	case 0x700000:
		if (offset & 0x8000)
			return READ_BYTE(static_ram6, (offset & 0x1fff));
		else
			return READ_BYTE(static_ram5, (offset & 0x1fff));
		break;

	case 0x800000:
		switch (offset & 0xff00)
		{
		case 0x0100:
		case 0x4100:
			return cps1_output_r(offset >> 1, mem_mask) >> shift;

		case 0x4000:
			switch (offset & 0xfe)
			{
			case 0x00: return cps2_inputport0_r(offset >> 1, mem_mask) >> shift;
			case 0x10: return cps2_inputport1_r(offset >> 1, mem_mask) >> shift;
			case 0x20: return cps2_eeprom_port_r(offset >> 1, mem_mask) >> shift;
			case 0x30: return cps2_qsound_volume_r(offset >> 1, mem_mask) >> shift;
			}
			break;
		}
		break;

	case 0x900000:
	case 0x910000:
	case 0x920000:
		return READ_BYTE(static_ram2, offset);

	case 0xff0000:
		return READ_BYTE(static_ram1, offset);
	}

	return 0xff;
}


/*------------------------------------------------------
	M68000 memory read (word)
------------------------------------------------------*/

u16 m68000_read_memory_16(u32 offset)
{
	offset &= M68K_AMASK;

	if (offset < 0x400000)
	{
		return READ_WORD(memory_region_cpu1, offset);
	}

	switch (offset & 0xff0000)
	{
	case 0x400000:
		return READ_WORD(static_ram4, offset);

	case 0x610000:
		return qsound_sharedram1_r(offset >> 1, 0);

	case 0x660000:
		return READ_WORD(static_ram3, offset);

	case 0x700000:
		if (offset & 0x8000)
			return READ_WORD(static_ram6, (offset & 0x1fff));
		else
			return READ_WORD(static_ram5, (offset & 0x1fff));
		break;

	case 0x800000:
		switch (offset & 0xff00)
		{
		case 0x0100:
		case 0x4100:
			return cps1_output_r(offset >> 1, 0);

		case 0x4000:
			switch (offset & 0xfe)
			{
			case 0x00: return cps2_inputport0_r(offset >> 1, 0);
			case 0x10: return cps2_inputport1_r(offset >> 1, 0);
			case 0x20: return cps2_eeprom_port_r(offset >> 1, 0);
			case 0x30: return cps2_qsound_volume_r(offset >> 1, 0);
			}
			break;
		}
		break;

	case 0x900000:
	case 0x910000:
	case 0x920000:
		return READ_WORD(static_ram2, offset);

	case 0xff0000:
		return READ_WORD(static_ram1, offset);
	}

	return 0xffff;
}


/*------------------------------------------------------
	M68000 memory write (byte)
------------------------------------------------------*/

void m68000_write_memory_8(u32 offset, u8 data)
{
	int shift = (~offset & 1) << 3;
	u16 mem_mask = ~(0xff << shift);

	offset &= M68K_AMASK;

	switch (offset & 0xff0000)
	{
	case 0x400000:
#if !RELEASE
		if (driver->kludge != CPS2_KLUDGE_HSF2D)
#endif
			WRITE_BYTE(static_ram4, offset, data);
		return;

	case 0x610000:
		qsound_sharedram1_w(offset >> 1, data << shift, mem_mask);
		return;

	case 0x660000:
		WRITE_BYTE(static_ram3, offset, data);
		return;

	case 0x700000:
		if (offset & 0x8000)
			WRITE_BYTE(static_ram6, (offset & 0x1fff), data);
		else
			WRITE_BYTE(static_ram5, (offset & 0x1fff), data);
		return;

	case 0x800000:
		switch (offset & 0xff00)
		{
		case 0x0100:
		case 0x4100:
			cps1_output_w(offset >> 1, data << shift, mem_mask);
			return;

		case 0x4000:
			switch (offset & 0xfe)
			{
			case 0x40:
				cps2_eeprom_port_w(offset >> 1, data << shift, mem_mask);
				return;

			case 0xe0:
				if (offset & 1)
				{
					cps2_objram_bank = data & 1;
					static_ram6 = (u8 *)cps2_objram[cps2_objram_bank ^ 1];
				}
				return;
			}
			break;
		}
		break;

	case 0x900000:
	case 0x910000:
	case 0x920000:
		WRITE_BYTE(static_ram2, offset, data);
		return;

	case 0xff0000:
#if !RELEASE
		if (driver->kludge == CPS2_KLUDGE_HSF2D)
		{
			if (offset >= 0xfffff0)
			{
				offset -= 0xbffff0;
				WRITE_BYTE(static_ram4, offset, data);
				return;
			}
		}
#endif
		WRITE_BYTE(static_ram1, offset, data);
		return;
	}
}


/*------------------------------------------------------
	M68000 memory write (word)
------------------------------------------------------*/

void m68000_write_memory_16(u32 offset, u16 data)
{
	offset &= M68K_AMASK;

	switch (offset & 0xff0000)
	{
	case 0x400000:
#if !RELEASE
		if (driver->kludge != CPS2_KLUDGE_HSF2D)
#endif
			WRITE_WORD(static_ram4, offset, data);
		return;

	case 0x610000:
		qsound_sharedram1_w(offset >> 1, data, 0);
		return;

	case 0x660000:
		WRITE_WORD(static_ram3, offset, data);
		return;

	case 0x700000:
		if (offset & 0x8000)
			WRITE_WORD(static_ram6, (offset & 0x1fff), data);
		else
			WRITE_WORD(static_ram5, (offset & 0x1fff), data);
		break;

	case 0x800000:
		switch (offset & 0xff00)
		{
		case 0x0100:
		case 0x4100:
			cps1_output_w(offset >> 1, data, 0);
			return;

		case 0x4000:
			switch (offset & 0xfe)
			{
			case 0x40:
				cps2_eeprom_port_w(offset >> 1, data, 0);
				return;

			case 0xe0:
				cps2_objram_bank = data & 1;
				static_ram6 = (u8 *)cps2_objram[cps2_objram_bank ^ 1];
				return;
			}
			break;
		}
		break;

	case 0x900000:
	case 0x910000:
	case 0x920000:
		WRITE_WORD(static_ram2, offset, data);
		return;

	case 0xff0000:
#if !RELEASE
		if (driver->kludge == CPS2_KLUDGE_HSF2D)
		{
			if (offset >= 0xfffff0)
			{
				offset -= 0xbffff0;
				WRITE_WORD(static_ram4, offset, data);
				return;
			}
		}
#endif
		WRITE_WORD(static_ram1, offset, data);
		return;
	}
}


/******************************************************************************
	Z80 memory read/write function
******************************************************************************/

/*------------------------------------------------------
	Z80 memory read (byte)
------------------------------------------------------*/

#if defined(GP2X) || defined(N900)
u8 z80_read_memory_8(u16 offset)
#else
u8 z80_read_memory_8(u32 offset)
#endif
{
	return memory_region_cpu2[offset & Z80_AMASK];
}


/*------------------------------------------------------
	Z80 memory write (byte)
------------------------------------------------------*/

#if defined(GP2X) || defined(N900)
void z80_write_memory_8(u8 data, u16 offset)
#else
void z80_write_memory_8(u32 offset, u8 data)
#endif
{
	offset &= Z80_AMASK;

	switch (offset & 0xf000)
	{
	case 0xc000: case 0xf000:
		// c000-cfff: QSOUND shared RAM
		// f000-ffff: RAM
		memory_region_cpu2[offset] = data;
		break;

	case 0xd000:
        //printf("qsound write: %d / %02x\n", offset & 0xf, data);
		switch (offset)
		{
		case 0xd000: qsound_data_h_w(0, data); break;
		case 0xd001: qsound_data_l_w(0, data); break;
		case 0xd002: qsound_cmd_w(0, data); break;
		case 0xd003: qsound_banksw_w(0, data); break;
		}
		break;
	}
}


/******************************************************************************
	Save/load state
******************************************************************************/

#ifdef SAVE_STATE

STATE_SAVE( memory )
{
	state_save_byte(cps1_ram, 0x10000);
	state_save_byte(cps1_gfxram, 0x30000);
	state_save_byte(cps1_output, 0x100);
	state_save_byte(cps2_ram, 0x4002);
	state_save_byte(cps2_objram[0], 0x2000);
	state_save_byte(cps2_objram[1], 0x2000);
	state_save_byte(cps2_output, 0x10);
	state_save_byte(qsound_sharedram1, 0x1000);
	state_save_byte(qsound_sharedram2, 0x1000);
}

STATE_LOAD( memory )
{
	state_load_byte(cps1_ram, 0x10000);
	state_load_byte(cps1_gfxram, 0x30000);
	state_load_byte(cps1_output, 0x100);
	state_load_byte(cps2_ram, 0x4002);
	state_load_byte(cps2_objram[0], 0x2000);
	state_load_byte(cps2_objram[1], 0x2000);
	state_load_byte(cps2_output, 0x10);
	state_load_byte(qsound_sharedram1, 0x1000);
	state_load_byte(qsound_sharedram2, 0x1000);
}

#endif /* SAVE_STATE */
