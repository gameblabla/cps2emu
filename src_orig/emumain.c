/*****************************************************************************

	emumain.c

	Emulator core

******************************************************************************/

#include "emumain.h"
#include "inptport.h"
#include <stdarg.h>
#include <getopt.h>
#ifdef GP2X
#include "usbjoy.h"
#include <sys/stat.h>
#else
#include <SDL.h>
#include <windows.h>
#endif

#define EMULATOR_TITLE      "CPS2EMU for GP2X(Ver 8)"
#define FRAMESKIP_LEVELS    12
#define TICKS_PER_SEC       1000000UL
#define TICKS_PER_FRAME     (TICKS_PER_SEC / FPS)


/******************************************************************************
	Global variable
******************************************************************************/

char game_name[16];
char parent_name[16];
char cache_parent_name[16];

char game_dir[MAX_PATH] = "./roms";
char cache_dir[MAX_PATH] = "./cache";

int option_showfps;
int option_showtitle;
int option_speedlimit;
int option_autoframeskip;
int option_frameskip;
int option_rescale;
int option_screen_position;
int option_linescroll;
int option_fullcache;
int option_extinput;
int option_xorrom;
int option_tweak;
int option_cpuspeed;
int option_hiscore;

int option_sound_enable;
int option_samplerate;
int option_sound_volume;

int option_m68k_clock;
int option_z80_clock;

int machine_driver_type;
int machine_init_type;
int machine_input_type;
int machine_screen_type;
int machine_sound_type;

u32 frames_displayed;
int fatal_error;
static int msg_line = -2;
static int msg_count = 0;

volatile int Loop;
char launchDir[MAX_PATH] = {0, };

int state_slot = 0;
int service_mode = 0;
int menu_mode = 0;
u8 *upper_memory = NULL;
u16 *work_frame = NULL;


/******************************************************************************
	Local variable
******************************************************************************/

static int frameskip;
static int frameskipadjust;
static int frameskip_counter;

static TICKER last_skipcount0_time;
static TICKER this_frame_base;

static int frames_since_last_fps;
static int rendered_frames_since_last_fps;
static int game_speed_percent;
static int frames_per_second;
static int title_bg = 0x2020;
static int title_show = 1;

static int snap_no = -1;

static char fatal_error_message[256];

static const u8 skiptable[FRAMESKIP_LEVELS][FRAMESKIP_LEVELS] =
{
	{ 0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0,0,0,0,0,0,0,0,0,0,0,1 },
	{ 0,0,0,0,0,1,0,0,0,0,0,1 },
	{ 0,0,0,1,0,0,0,1,0,0,0,1 },
	{ 0,0,1,0,0,1,0,0,1,0,0,1 },
	{ 0,1,0,0,1,0,1,0,0,1,0,1 },
	{ 0,1,0,1,0,1,0,1,0,1,0,1 },
	{ 0,1,0,1,1,0,1,0,1,1,0,1 },
	{ 0,1,1,0,1,1,0,1,1,0,1,1 },
	{ 0,1,1,1,0,1,1,1,0,1,1,1 },
	{ 0,1,1,1,1,1,0,1,1,1,1,1 },
	{ 0,1,1,1,1,1,1,1,1,1,1,1 }
};

#define BTN(x) (1 << x)

#ifdef GP2X
enum {
	GPX_VK_UP = 0,
	GPX_VK_UP_LEFT,
	GPX_VK_LEFT,
	GPX_VK_DOWN_LEFT,
	GPX_VK_DOWN,
	GPX_VK_DOWN_RIGHT,
	GPX_VK_RIGHT,
	GPX_VK_UP_RIGHT,

	GPX_VK_START,
	GPX_VK_SELECT,
	GPX_VK_FL,
	GPX_VK_FR,
	GPX_VK_FA,
	GPX_VK_FB,
	GPX_VK_FX,
	GPX_VK_FY,

	GPX_VK_VOL_UP,
	GPX_VK_VOL_DOWN,
	GPX_VK_TAT,

	GPX_VK_MAX
};

enum {
	JOY_UP = 0,
	JOY_DOWN,
	JOY_LEFT,
	JOY_RIGHT,

	JOY_BUTTON,
	JOY_BUTTON1,

	JOY_MAX = 37
};

enum {
	JOY_BASIC = 0,
	JOY_GREEN,
	JOY_DRAGON
};

#define GPX_CROSS_MASK ( BTN(GPX_VK_UP) | BTN(GPX_VK_DOWN) | BTN(GPX_VK_LEFT) | BTN(GPX_VK_RIGHT) )
#define GPX_DIAG_MASK ( BTN(GPX_VK_UP_LEFT) | BTN(GPX_VK_DOWN_LEFT) | BTN(GPX_VK_UP_RIGHT) | BTN(GPX_VK_DOWN_RIGHT) )

static struct joy_info {
	const char *name;
	const int type;
} joy_list[] = {
	{"GreenAsia Inc.    USB Joystick     ", JOY_GREEN},
	{"DragonRise Inc.   Generic   USB  Joystick  ", JOY_DRAGON},
	{NULL, JOY_BASIC}
};

static int joy_fd = -1;
static int mem_fd = -1;
volatile static u16 *gp2xregs = NULL;
static struct usbjoy *joys[4];
static int joy_type[4] = {0, };
static int joy_map[4][JOY_MAX];
static int input_map[GPX_VK_MAX];

char option_frontend[MAX_PATH] = "/usr/gp2x/gp2xmenu";
#else
static LARGE_INTEGER freq, base;
static SDL_Surface *screen_surface = NULL;
#endif

static int joy_count = 0;
static u16 *screen = NULL;
int screen_mode = 0;


/******************************************************************************
	Prototype
******************************************************************************/

void open_joystick(void);
static void clean_msg(void);
static const char *draw_text(const char *text, int x, int y, int fg, int bg);
void menu_on(void);

/*--------------------------------------------------------
	Emulation start
--------------------------------------------------------*/

void Terminate(void)
{
	if(work_frame) free(work_frame);

#ifdef GP2X
	if(upper_memory) munmap(upper_memory, CACHE_SIZE);
	if(screen) munmap(screen, 0x40000);
	if(gp2xregs) munmap((void *)gp2xregs, 0x10000);
	if(mem_fd >= 0) close(mem_fd);
	if(joy_fd >= 0) close(joy_fd);
	while(joy_count)
		joy_close(joys[--joy_count]);

	printf("frontend option is '%s'\n", option_frontend);
	{
		struct stat info;
		if( (lstat(option_frontend, &info) == 0) && S_ISREG(info.st_mode) ) {
			char path[MAX_PATH];
			char *p;
			strcpy(path, option_frontend);
			p = strrchr(path, '/');
			if(p == NULL) p = strrchr(path, '\\');
			if(p != NULL) {
				*p = '\0';
				printf("frontend found in '%s'\n", path);
				chdir(path);
			}
			execl(option_frontend, option_frontend, NULL);
		} else {
			printf("frontend not found. return to gp2xmenu...\n");
			chdir("/usr/gp2x");
			execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);
		}
	}
#else
	if(upper_memory) free(upper_memory);
	SDL_Quit();
#endif
}

void parse_cmd(int argc, char *argv[])
{
	int option_index, c;
	int val;
	char *p;

	static struct option long_opts[] = {
		{"sound", 0, &option_sound_enable, 1},
		{"no-sound", 0, &option_sound_enable, 0},
		{"samplerate", required_argument, 0, 'r'},
		{"no-rescale", 0, &option_rescale, 0},
		{"sw-rescale", 0, &option_rescale, 1},
		{"hw-rescale", 0, &option_rescale, 2},
		{"hwho-rescale", 0, &option_rescale, 3},
		{"showfps", 0, &option_showfps, 1},
		{"no-showfps", 0, &option_showfps, 0},
		{"68kclock", required_argument, 0, '6'},
		{"z80clock", required_argument, 0, '8'},
		{"frontend", required_argument, 0, 'f'},
		{"showtitle", 0, &option_showtitle, 1},
		{"no-showtitle", 0, &option_showtitle, 0},
		{"screen-position", required_argument, 0, 'p'},
		{"linescroll", 0, &option_linescroll, 1},
		{"no-linescroll", 0, &option_linescroll, 0},
		{"fullcache", 0, &option_fullcache, 1},
		{"no-fullcache", 0, &option_fullcache, 0},
		{"extinput", 0, &option_extinput, 1},
		{"no-extinput", 0, &option_extinput, 0},
		{"cache-dir", required_argument, 0, 'c'},
		{"xorrom", 0, &option_xorrom, 1},
		{"no-xorrom", 0, &option_xorrom, 0},
		{"tweak", 0, &option_tweak, 1},
		{"no-tweak", 0, &option_tweak, 0},
		{"cpuspeed", required_argument, 0, 's'},
		{"hiscore", 0, &option_hiscore, 1},
		{"no-hiscore", 0, &option_hiscore, 0},
	};

	option_index=optind=0;

	while((c=getopt_long(argc, argv, "", long_opts, &option_index))!=EOF) {
		switch(c) {
			case 'r':
				if(!optarg) continue;
				if(strcmp(optarg, "11025") == 0) option_samplerate = 0;
				if(strcmp(optarg, "22050") == 0) option_samplerate = 1;
				if(strcmp(optarg, "44100") == 0) option_samplerate = 2;
				break;
			case '6':
			case '8':
				if(!optarg) continue;
				val = atoi(optarg);
				if((val > -100) && (val < 300)) {
					if(c == '6')
						option_m68k_clock = 100 + val;
					else
						option_z80_clock = 100 + val;
				}
				break;
			case 'p':
				if(!optarg) continue;
				val = atoi(optarg);
				if((val >= -32) && (val <= 32)) option_screen_position = 32 + val;
				break;
			case 'f':
				if(!optarg) continue;
#ifdef GP2X
				p = strrchr(optarg, '/');
				if(p == NULL)
					sprintf(option_frontend, "%s%s", launchDir, optarg);
				else
					strcpy(option_frontend, optarg);
#endif
				break;
			case 'c':
				if(!optarg) continue;
#ifdef GP2X
				struct stat info;
				if( (lstat(optarg, &info) == 0) && S_ISDIR(info.st_mode) )
					strcpy(cache_dir, optarg);
#endif
				break;
			case 's':
				if(!optarg) continue;
				val = atoi(optarg);
				if((val >= 30) && (val <= 400)) option_cpuspeed = val;
				break;
		}
	}

	if(optind < argc) {
		char path[MAX_PATH], name[MAX_PATH];
		char *p;
		strcpy(path, argv[optind]);
		p = strrchr(path, '/');
		if(p == NULL)
			strcpy(name, path);
		else {
			strcpy(name, p + 1);
#ifdef GP2X
			*p = '\0';
			struct stat info;
			if( (lstat(path, &info) == 0) && S_ISDIR(info.st_mode) )
				strcpy(game_dir, path);
#endif
		}
		p = strrchr(name, '.');
		if(p != NULL) *p = 0;
		if(strlen(name) < 15) strcpy(game_name, name);
	}

	if(option_rescale >= 2) option_showfps = 0;
}

int main(int argc, char *argv[])
{
	atexit (Terminate);

	memset(game_name, 0, sizeof(game_name));
	option_sound_enable = 0;
	option_samplerate = 1;
	option_sound_volume = 50;
	option_controller = INPUT_PLAYER1;
	option_speedlimit = 1;
	option_frameskip = 0;
	option_autoframeskip = 1;
	option_rescale = 0;
	option_showfps = 0;
	option_m68k_clock = 100;
	option_z80_clock = 100;
	option_showfps = 0;
	option_showtitle = 0;
	option_screen_position = 32;
	option_linescroll = 1;
	option_fullcache = 1;
	option_extinput = 0;
	option_xorrom = 1;
	option_tweak = 0;
	option_cpuspeed = 0;
	option_hiscore = 1;

#ifdef GP2X
	if(realpath("/proc/self/exe", launchDir) != NULL) {
		char *p = strrchr(launchDir, '/');
		if(p != NULL) {
			p[1] = '\0';
			chdir(launchDir);
		} else {
			strcpy(launchDir, "./");
		}
	} else {
		strcpy(launchDir, "./");
	}
#endif

	parse_cmd(argc, argv);

#ifdef GP2X
	joy_fd = open("/dev/GPIO", O_RDWR | O_NDELAY);
	if(joy_fd < 0) {
		fprintf (stderr, "Couldn't open /dev/GPIO device.\n");
		exit (1);
	}

	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(mem_fd < 0) {
		fprintf (stderr, "Couldn't open /dev/mem device.\n");
		exit (1);
	} else {
		int mmu_fd;
		upper_memory = (u8 *)mmap(0, CACHE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x02000000);
#ifdef MMUHACK
		mmu_fd = open("/dev/mmuhack", O_RDWR);
		if(mmu_fd < 0) {
			system("/sbin/insmod -f mmuhack.o");
			mmu_fd = open("/dev/mmuhack", O_RDWR);
		}
		if(mmu_fd >= 0) close(mmu_fd);
#endif
	}

	screen = (u16 *)mmap(0, 0x40000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0x03101000);
	gp2xregs = (u16 *)mmap(0, 0x10000, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0xC0000000);
	if(gp2xregs) {
		gp2xregs[0x28e2>>1] = 0;
		gp2xregs[0x28e6>>1] = 0;
		if(gp2xregs[0x2816>>1] == 319) { // LCD
			screen_mode = 0;
			gp2xregs[0x28e4>>1] = 319;
			gp2xregs[0x28e8>>1] = 239;
		} else { // TV
			gp2xregs[0x28e4>>1] = 719;
			if (gp2xregs[0x2818>>1]  == 287) { // PAL
				screen_mode = 1;
				gp2xregs[0x28e8>>1] = 287;
			} else if (gp2xregs[0x2818>>1]  == 239) { // NTSC
				screen_mode = 2;
				gp2xregs[0x28e8>>1] = 239;
			}
		}

		if(option_tweak) {
			int tMRD = 0, tRFC = 0, tRP = 1, tRCD = 1;
			int LAT = 0, tRC = 6, tRAS = 3, tWR = 0;
			int REFPERD = 459;

			FILE *fp;
			char path[MAX_PATH];

			sprintf(path, "%scpu_speed.cfg", launchDir);

			if ((fp = fopen(path, "r")) != NULL) {
				char buf[256];
				int *value = NULL;
				int isvalue = 0;
				msg_printf("Reading cpu_speed.cfg file...\n");
				#define str_cmp(s1, s2) strncasecmp(s1, s2, strlen(s2))
				while(fgets(buf, 255, fp)) {
					if(isvalue) {
						int val = atoi(buf);
						if((value == &option_cpuspeed) && !option_cpuspeed) option_cpuspeed = ((val > 400) || (val < 30)) ? 0 : val;
						else if(value == &LAT) LAT = (val == 3) ? 1 : 0;
						else if(value == &REFPERD) REFPERD = (val - 1) & 0xffff;
						else if(value != NULL) *value = (val - 1) & 0x0f;
						isvalue = 0;
					} else {
						value = NULL;
						if(!str_cmp(buf, "CPU-Clock")) value = &option_cpuspeed;
						else if(!str_cmp(buf, "CAS")) value = &LAT;
						else if(!str_cmp(buf, "tRCD")) value = &tRCD;
						else if(!str_cmp(buf, "tRC")) value = &tRC;
						else if(!str_cmp(buf, "tRAS")) value = &tRAS;
						else if(!str_cmp(buf, "tWR")) value = &tWR;
						else if(!str_cmp(buf, "tMRD")) value = &tMRD;
						else if(!str_cmp(buf, "tRFC")) value = &tRFC;
						else if(!str_cmp(buf, "tRP")) value = &tRP;
						else if(!str_cmp(buf, "Refresh-Period")) value = &REFPERD;
						isvalue = 1;
					}
				}
				fclose(fp);
			}
			gp2xregs[0x3802>>1] = (tMRD << 12) | (tRFC << 8) | (tRP << 4) | tRCD;
			gp2xregs[0x3804>>1] = (gp2xregs[0x3804>>1] & 0xE000) | (LAT << 12) | (tRC << 8) | (tRAS << 4) | tWR;
			gp2xregs[0x3808>>1] = REFPERD;
		}

		if(option_cpuspeed > 0) {
			int mdiv = option_cpuspeed / 2.4576 + 0.5;
			mdiv -= 8;
			if(mdiv < 0) mdiv = 0;
			if(mdiv > 255) mdiv = 255;
			gp2xregs[0x0910>>1] = (mdiv << 8) | (1 << 2);
		}

		{
			int mdiv = gp2xregs[0x0910>>1];
			int pdiv = ((mdiv & 0xfc) >> 2) + 2;
			mdiv = (mdiv >> 8) + 8;
			option_cpuspeed = mdiv * 7.3728 / pdiv + 0.5;
		}
	} else {
		screen_mode = 0;
		option_cpuspeed = 0;
		option_tweak = 0;
		if(option_rescale >= 2) option_rescale = 1;
	}

	joy_count = 0;
	if(option_extinput) open_joystick();
#else
	/* Initialize SDL */
	if (SDL_Init (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER) < 0) {
		fprintf (stderr, "Couldn't initialize SDL: %s\n", SDL_GetError ());
		exit (1);
	}

	screen_surface = SDL_SetVideoMode (320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen_surface == NULL) {
		fprintf (stderr, "Couldn't set video mode: %s\n", SDL_GetError ());
		exit (2);
	}
	screen = NULL;

	/* Check and open joystick device */
	if (SDL_NumJoysticks() > 0) {
		SDL_JoystickOpen(0);
	}

	upper_memory = (u8 *)malloc(CACHE_SIZE);

	/*QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&base);*/
#endif

	msg_printf("Launch dir is '%s'.\n", launchDir);

	work_frame = (u16 *)memalign(MEM_ALIGN, BUF_WIDTH * BUF_HEIGHT * 2);
	if(work_frame == NULL) {
		fprintf (stderr, "Work buffer allocation failed.\n");
		exit (1);
	}

	snap_no = -1;
	state_slot = 0;
	service_mode = 0;

	cps2_main();
}


void open_joystick(void)
{
#ifdef GP2X
	int i;

	while(joy_count)
		joy_close(joys[--joy_count]);

	for (i=1; i<16; i++) {
		struct usbjoy *joy = joy_open(i);
		if(joy != NULL) {
			int j = 0;
			const char *name = joy_name(joy);

			while(joy_list[j].name) {
				if(strcmp(joy_list[j].name, name) == 0) break;
				j++;
			}
			joy_type[joy_count] = joy_list[j].type;
			joys[joy_count++] = joy;

			if(joy_count == 4) break;
		}
	}
#else
	joy_count = 0;
#endif

	msg_printf("%d USB device found.\n", joy_count);
}

u32 reset_joystick(int num, int max_buttons, int rotate)
{
#ifdef GP2X
	if(num == 0) {
		int i;

		for(i = 0; i < GPX_VK_MAX; i++) input_map[i] = -1;

		input_map[GPX_VK_START] = P1_START;
		input_map[GPX_VK_SELECT] = P1_COIN;

		input_map[GPX_VK_FX] = P1_BUTTON1;
		input_map[GPX_VK_FB] = P1_BUTTON2;
		input_map[GPX_VK_FA] = P1_BUTTON3;
		input_map[GPX_VK_FY] = P1_BUTTON4;

		if(rotate) {
			input_map[GPX_VK_UP] = P1_LEFT;
			input_map[GPX_VK_DOWN] = P1_RIGHT;
			input_map[GPX_VK_LEFT] = P1_DOWN;
			input_map[GPX_VK_RIGHT] = P1_UP;

			input_map[GPX_VK_VOL_UP] = P1_BUTTON1;
			input_map[GPX_VK_FL] = P1_BUTTON2;
			input_map[GPX_VK_VOL_DOWN] = P1_AF_1;

			if(max_buttons == 3)
			   input_map[GPX_VK_VOL_DOWN] = P1_BUTTON3;
		} else {
			input_map[GPX_VK_UP] = P1_UP;
			input_map[GPX_VK_DOWN] = P1_DOWN;
			input_map[GPX_VK_LEFT] = P1_LEFT;
			input_map[GPX_VK_RIGHT] = P1_RIGHT;

			switch(max_buttons) {
				case 6:
					input_map[GPX_VK_FA] = P1_BUTTON1;
					input_map[GPX_VK_FY] = P1_BUTTON2;
					input_map[GPX_VK_FL] = P1_BUTTON3;
					input_map[GPX_VK_FX] = P1_BUTTON4;
					input_map[GPX_VK_FB] = P1_BUTTON5;
					input_map[GPX_VK_FR] = P1_BUTTON6;
					break;
				case 2:
					input_map[GPX_VK_FX] = P1_BUTTON1;
					input_map[GPX_VK_FB] = P1_BUTTON2;
					input_map[GPX_VK_FA] = P1_AF_1;
					input_map[GPX_VK_FY] = P1_BUTTON2;
					input_map[GPX_VK_FR] = P1_BUTTON2;
					break;
				default:
					input_map[GPX_VK_FX] = P1_BUTTON1;
					input_map[GPX_VK_FB] = P1_BUTTON2;
					input_map[GPX_VK_FA] = P1_BUTTON3;
					input_map[GPX_VK_FY] = P1_BUTTON4;
					break;
			}
		}
	}

	if(num < joy_count) {
		int i;
		for (i = 0; i < JOY_MAX; i++) joy_map[num][i] = -1;

		if(rotate) {
			joy_map[num][JOY_UP] = P1_LEFT;
			joy_map[num][JOY_DOWN] = P1_RIGHT;
			joy_map[num][JOY_LEFT] = P1_DOWN;
			joy_map[num][JOY_RIGHT] = P1_UP;
		} else {
			joy_map[num][JOY_UP] = P1_UP;
			joy_map[num][JOY_DOWN] = P1_DOWN;
			joy_map[num][JOY_LEFT] = P1_LEFT;
			joy_map[num][JOY_RIGHT] = P1_RIGHT;
		}

		switch(joy_type[num]) {
			case JOY_GREEN:
				msg_printf("Joystick %d: GreenAsia type\n", num);
				joy_map[num][JOY_BUTTON + 5] = P1_COIN;
				joy_map[num][JOY_BUTTON + 6] = P1_START;

				joy_map[num][JOY_BUTTON + 9] = P1_COIN;
				joy_map[num][JOY_BUTTON + 10] = P1_START;
				joy_map[num][JOY_BUTTON + 11] = P1_START;
				joy_map[num][JOY_BUTTON + 12] = P1_START;

				switch(max_buttons) {
					case 6:
						joy_map[num][JOY_BUTTON + 1] = P1_BUTTON1;
						joy_map[num][JOY_BUTTON + 2] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 7] = P1_BUTTON3;
						joy_map[num][JOY_BUTTON + 3] = P1_BUTTON4;
						joy_map[num][JOY_BUTTON + 4] = P1_BUTTON5;
						joy_map[num][JOY_BUTTON + 8] = P1_BUTTON6;
						break;
					case 2:
						joy_map[num][JOY_BUTTON + 3] = P1_BUTTON1;
						joy_map[num][JOY_BUTTON + 4] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 1] = P1_AF_1;
						joy_map[num][JOY_BUTTON + 2] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 7] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 8] = P1_BUTTON2;
						break;
					default:
						joy_map[num][JOY_BUTTON + 3] = P1_BUTTON1;
						joy_map[num][JOY_BUTTON + 4] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 1] = P1_BUTTON3;
						joy_map[num][JOY_BUTTON + 2] = P1_BUTTON4;
						break;
				}
				break;
				break;
			case JOY_DRAGON:
				msg_printf("Joystick %d: DragonRise type\n", num);
				joy_map[num][JOY_BUTTON + 7] = P1_COIN;
				joy_map[num][JOY_BUTTON + 9] = P1_START;

				joy_map[num][JOY_BUTTON + 11] = P1_START;
				joy_map[num][JOY_BUTTON + 12] = P1_START;

				switch(max_buttons) {
					case 6:
						joy_map[num][JOY_BUTTON + 1] = P1_BUTTON1;
						joy_map[num][JOY_BUTTON + 2] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 5] = P1_BUTTON3;
						joy_map[num][JOY_BUTTON + 8] = P1_BUTTON3;
						joy_map[num][JOY_BUTTON + 3] = P1_BUTTON4;
						joy_map[num][JOY_BUTTON + 4] = P1_BUTTON5;
						joy_map[num][JOY_BUTTON + 6] = P1_BUTTON6;
						joy_map[num][JOY_BUTTON + 10] = P1_BUTTON6;
						break;
					case 2:
						joy_map[num][JOY_BUTTON + 3] = P1_BUTTON1;
						joy_map[num][JOY_BUTTON + 4] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 1] = P1_AF_1;
						joy_map[num][JOY_BUTTON + 2] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 8] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 10] = P1_BUTTON2;
						break;
					default:
						joy_map[num][JOY_BUTTON + 3] = P1_BUTTON1;
						joy_map[num][JOY_BUTTON + 4] = P1_BUTTON2;
						joy_map[num][JOY_BUTTON + 1] = P1_BUTTON3;
						joy_map[num][JOY_BUTTON + 2] = P1_BUTTON4;
						break;
				}
				break;
			case JOY_BASIC:
			default:
				msg_printf("Joystick %d: Basic or Unknown type\n", num); // default set from mamegp2x
				joy_map[num][JOY_BUTTON + 1] = P1_BUTTON4;
				joy_map[num][JOY_BUTTON + 2] = P1_BUTTON2;
				joy_map[num][JOY_BUTTON + 3] = P1_BUTTON1;
				joy_map[num][JOY_BUTTON + 4] = P1_BUTTON3;
				joy_map[num][JOY_BUTTON + 5] = P1_BUTTON5;
				joy_map[num][JOY_BUTTON + 6] = P1_BUTTON6;
				joy_map[num][JOY_BUTTON + 7] = P1_BUTTON5;
				joy_map[num][JOY_BUTTON + 8] = P1_BUTTON6;

				joy_map[num][JOY_BUTTON + 9] = P1_COIN;
				joy_map[num][JOY_BUTTON + 10] = P1_START;
				joy_map[num][JOY_BUTTON + 11] = P1_START;
				joy_map[num][JOY_BUTTON + 12] = P1_START;
				break;
		}
	}
#endif
}

u32 read_joystick(int num)
{
	u32 buttons = 0;
	int i;

#ifdef GP2X
	if (num == 0) {
		#define PRESS(x) (gp2xbtn & BTN(x))
		static u32 prev = 0;
		u32 gp2xbtn;
		u32 change = 0;
		int fn = 0;

		if((joy_fd < 0) || (read(joy_fd, &gp2xbtn, 4) != 4)) gp2xbtn = 0;

		change = prev ^ gp2xbtn;
		prev = gp2xbtn;

		if (gp2xbtn & GPX_CROSS_MASK) gp2xbtn &= ~GPX_DIAG_MASK;
		if (PRESS(GPX_VK_UP_LEFT)) gp2xbtn |= BTN(GPX_VK_UP) | BTN(GPX_VK_LEFT);
		if (PRESS(GPX_VK_UP_RIGHT)) gp2xbtn |= BTN(GPX_VK_UP) | BTN(GPX_VK_RIGHT);
		if (PRESS(GPX_VK_DOWN_LEFT)) gp2xbtn |= BTN(GPX_VK_DOWN) | BTN(GPX_VK_LEFT);
		if (PRESS(GPX_VK_DOWN_RIGHT)) gp2xbtn |= BTN(GPX_VK_DOWN) | BTN(GPX_VK_RIGHT);

		if (PRESS(GPX_VK_SELECT) && ((PRESS(GPX_VK_FL) && PRESS(GPX_VK_FR)) || PRESS(GPX_VK_START))) {
			menu_mode = 1;
			gp2xbtn &= ~(BTN(GPX_VK_SELECT) | BTN(GPX_VK_FL) | BTN(GPX_VK_FR) | BTN(GPX_VK_START));
		}

		if ((change & BTN(GPX_VK_START)) && PRESS(GPX_VK_FL) && PRESS(GPX_VK_FR)) {
			if(PRESS(GPX_VK_START))
				buttons |= BTN(P1_START) | BTN(P2_START);
			else
				buttons &= ~(BTN(P1_START) | BTN(P2_START));
			gp2xbtn &= ~(BTN(GPX_VK_START) | BTN(GPX_VK_FL) | BTN(GPX_VK_FR));
		}

		if ((machine_screen_type != SCREEN_VERTICAL) || PRESS(GPX_VK_START) || PRESS(GPX_VK_SELECT)) {

			if (gp2xbtn & change & BTN(GPX_VK_VOL_UP)) {
				if (PRESS(GPX_VK_SELECT)) {
					if(option_autoframeskip) {
						option_autoframeskip = 0;
						option_frameskip = 0;
						autoframeskip_reset();
						msg_printf("Frame skip: %d\n", option_frameskip);
					} else if(option_frameskip < 11) {
						++option_frameskip;
						autoframeskip_reset();
						msg_printf("Frame skip: %d\n", option_frameskip);
					}
				} else {
					if(option_sound_volume < 30)
						sound_volume(option_sound_volume + 2);
					else
						sound_volume(option_sound_volume + 5);
				}
			}

			if (gp2xbtn & change & BTN(GPX_VK_VOL_DOWN)) {
				if (PRESS(GPX_VK_SELECT)) {
					option_autoframeskip = 0;
					if(option_frameskip > 0) {
						--option_frameskip;
						autoframeskip_reset();
						msg_printf("Frame skip: %d\n", option_frameskip);
					} else {
						option_autoframeskip = 1;
						autoframeskip_reset();
						msg_printf("Frame skip: Auto\n");
					}
				} else {
					if(option_sound_volume > 30)
						sound_volume(option_sound_volume - 5);
					else
						sound_volume(option_sound_volume - 2);
				}
			}
		}

		for(i = 0; i < GPX_VK_MAX; i++) {
			if((input_map[i] != -1) && PRESS(i))
				buttons |= BTN(input_map[i]);
		}

		#undef PRESS
	}

	if (num < joy_count) {
		buttons = buttons & (BTN(P1_COIN) | BTN(P1_START) | BTN(SERV_SWITCH));
		joy_update(joys[num]);
		if(joy_getaxe(UP, joys[num])) buttons |= BTN(joy_map[num][JOY_UP]);
		if(joy_getaxe(DOWN, joys[num])) buttons |= BTN(joy_map[num][JOY_DOWN]);
		if(joy_getaxe(LEFT, joys[num])) buttons |= BTN(joy_map[num][JOY_LEFT]);
		if(joy_getaxe(RIGHT, joys[num])) buttons |= BTN(joy_map[num][JOY_RIGHT]);

		for (i = 0; i < 32; i++) {
			if((joy_map[num][JOY_BUTTON1 + i] != -1) && joy_getbutton(i, joys[num]))
				buttons |= BTN(joy_map[num][JOY_BUTTON1 + i]);
		}
	}
#else
	SDL_Event event;
	static int pressed = 0;
	int button = -1;

	if(num != 0) return 0;

	SDL_Delay(1);
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				Loop = LOOP_EXIT;
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE: Loop = LOOP_EXIT; break;
					case SDLK_F5: Loop = LOOP_RESET; break;
					case SDLK_F12: menu_mode = 1; break;
				}
			case SDL_KEYUP:
				button = -1;
				switch(event.key.keysym.sym) {
					case SDLK_1: button = P1_START; break;
					case SDLK_2: button = P2_START; break;
					case SDLK_5: button = P1_COIN; break;
					case 'a': button = P1_BUTTON1; break;
					case 's': button = P1_BUTTON2; break;
					case 'd': button = P1_BUTTON3; break;
					case 'z': button = P1_BUTTON4; break;
					case 'x': button = P1_BUTTON5; break;
					case 'c': button = P1_BUTTON6; break;
					case 'q': button = P1_AF_1; break;
					case 'w': button = P1_AF_2; break;
					case 'e': button = P1_AF_3; break;
					case SDLK_COMMA: button = P1_DIAL_L; break;
					case SDLK_PERIOD: button = P1_DIAL_R; break;
					case SDLK_LEFT: button = P1_LEFT; break;
					case SDLK_RIGHT: button = P1_RIGHT; break;
					case SDLK_UP: button = P1_UP; break;
					case SDLK_DOWN: button = P1_DOWN; break;
					case SDLK_F2:
						button = SERV_SWITCH;
						msg_printf("Enter service mode\n");
						break;
				}
				if(button != -1)
					if(event.type == SDL_KEYDOWN)
						pressed |= BTN(button);
					else
						pressed &= ~BTN(button);
				break;
		}
	}

	buttons = pressed;
#endif

	if((num == 0) && (service_mode)) {
		msg_printf("Enter service mode\n");
		buttons |= BTN(SERV_SWITCH);
		service_mode = 0;
	}

	return buttons;
}


static TICKER get_ticks(void)
{
#ifdef GP2X
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (TICKER)tv.tv_sec * TICKS_PER_SEC + tv.tv_usec;
#else
	/*LARGE_INTEGER tim;
	QueryPerformanceCounter(&tim);
	return (TICKER)((tim.QuadPart - base.QuadPart) * TICKS_PER_SEC / freq.QuadPart);*/
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (TICKER)tv.tv_sec * TICKS_PER_SEC + tv.tv_usec;
	//return SDL_GetTicks();
#endif
}

/*--------------------------------------------------------
	Frameskip reset
--------------------------------------------------------*/

void autoframeskip_reset(void)
{
	frameskip = option_autoframeskip ? 0 : option_frameskip;
	frameskipadjust = 0;
	frameskip_counter = FRAMESKIP_LEVELS - 1;

	rendered_frames_since_last_fps = 0;
	frames_since_last_fps = 0;

	game_speed_percent = 100;
	frames_per_second = FPS;

	frames_displayed = 0;

	last_skipcount0_time = get_ticks() - TICKS_PER_FRAME;
}


/*--------------------------------------------------------
	Display screen
--------------------------------------------------------*/

void update_screen(void)
{
	u8 skipped_it = skiptable[frameskip][frameskip_counter];

	frames_displayed++;
	frames_since_last_fps++;

	if (!skipped_it)
	{
		register u16 *src = work_frame + BUF_WIDTH * 32 + 64;
		register u16 *dst = screen;
		int start, end;
		TICKER curr;

#ifndef GP2X
		SDL_LockSurface(screen_surface);
		screen = screen_surface->pixels;
		dst = screen;
#endif

		start = FIRST_VISIBLE_LINE;
		end = 0;

		draw_sprite_start();
		memset(src, 0, BUF_WIDTH * 224 * 2);

		switch(option_rescale) {
			case 0:
				src += option_screen_position;
				dst += 320 * 8;
				break;
			case 1:
				dst += 320 * 8;
				break;
			case 2:
				// Nothing
				break;
			case 3:
				dst += 384 * 8;
				break;
		}

		while(1) {
			int x, y;

			end = draw_sprite();
			if(end == 0) end = LAST_VISIBLE_LINE;

			switch(option_rescale) {
				case 0:
					for(y = (end - start + 1); y; --y) {
						memcpy(dst, src, 320*2);
						src += BUF_WIDTH;
						dst += 320;
					}
					break;
				case 1:
					for(y = (end - start + 1); y; --y) {
						for(x = 64; x; --x) {
							register int c1, c2;
							*dst++ = *src++;
							*dst++ = *src++;
							c1 = *src++;
							c2 = *src++;
							*dst++ = COLORMIX(c1, c2);
							*dst++ = *src++;
							*dst++ = *src++;
						}

						src += BUF_WIDTH - 384;
					}
					break;
				case 2:
				case 3:
					for(y = (end - start + 1); y; --y) {
						memcpy(dst, src, 384*2);
						src += BUF_WIDTH;
						dst += 384;
					}
					break;
			}

			start = end + 1;
			if(start > LAST_VISIBLE_LINE) break;
			memset(src, 0, BUF_WIDTH * 32 * 2);
		}

#ifndef GP2X
		SDL_UnlockSurface(screen_surface);
		screen = NULL;
		SDL_Flip(screen_surface);
#endif

		curr = get_ticks();

		if(option_speedlimit) {
			TICKER target;

			if(frameskip_counter == 0)
				this_frame_base = last_skipcount0_time + (int)((float)FRAMESKIP_LEVELS * 1000000.0 / FPS);

			target = this_frame_base + (int)((float)frameskip_counter * 1000000.0 / FPS);

			while(curr < target) {
#ifdef WIN32
				if(target - curr > 20000)
					Sleep(20);
				else if(target - curr > 5000)
					Sleep(0);
#endif
				curr = get_ticks();
			}
		}

		rendered_frames_since_last_fps++;

		if(frameskip_counter == 0) {
			float seconds_elapsed = (float)(curr - last_skipcount0_time) * (1.0 / 1000000.0);
			float frames_per_sec = (float)frames_since_last_fps / seconds_elapsed;

			game_speed_percent = (int)(100.0 * frames_per_sec / FPS + 0.5);
			frames_per_second = (int)((float)rendered_frames_since_last_fps / seconds_elapsed + 0.5);

			last_skipcount0_time = curr;
			frames_since_last_fps = 0;
			rendered_frames_since_last_fps = 0;

			if(option_showfps) {
				char buf[32];
				int x;
				sprintf(buf, "%5d(%d%%)", frames_per_second, game_speed_percent);
				x = 320 - (strlen(buf) * 8);
				draw_text(buf, x, 0, 0xBDF7, title_bg);
			}

			if (option_autoframeskip && frames_displayed > 2 * FRAMESKIP_LEVELS)
			{
				if (game_speed_percent >= 99)
				{
					frameskipadjust++;

					if (frameskipadjust >= 3)
					{
						frameskipadjust = 0;
						if (frameskip > 0) frameskip--;
					}
				}
				else
				{
					if (game_speed_percent < 80)
					{
						frameskipadjust -= (90 - game_speed_percent) / 5;
					}
					else if (frameskip < 8)
					{
						frameskipadjust--;
					}

					while (frameskipadjust <= -2)
					{
						frameskipadjust += 2;
						if (frameskip < FRAMESKIP_LEVELS - 1)
							frameskip++;
					}
				}
			}
		}
	}

	if(msg_count) {
		if(!--msg_count) clean_msg();
	}

	frameskip_counter = (frameskip_counter + 1) % FRAMESKIP_LEVELS;

	if(menu_mode) {
		menu_on();
		autoframeskip_reset();
	}
/*
	if(state_slot != -1) {
		if(state_slot < 0x10) {
			state_save(state_slot & 0x0f);
		} else {
			state_load(state_slot & 0x0f);
		}
		autoframeskip_reset();
		state_slot = -1;
	}
*/
}


/*--------------------------------------------------------
	Draw text & message
--------------------------------------------------------*/

void SetTextMode(void)
{
#ifdef GP2X
	int hsc, vsc, hw;

	switch(screen_mode) {
		case 0: // LCD
			hsc = 1024;
			vsc = 640;
			hw = 640;
			break;
		case 1: // TV PAL
			hsc = 512;
			vsc = 533;
			hw = 640;
			break;
		case 2: // TV NTSC
			hsc = 512;
			vsc = 640;
			hw = 640;
			break;
	}

	memset(screen, 0, 0x40000);

	gp2xregs[0x2906>>1] = hsc & 0xFFFF;
	gp2xregs[0x2908>>1] = vsc & 0xFFFF;
	gp2xregs[0x290A>>1] = vsc >> 16;
	gp2xregs[0x290C>>1] = hw;
#else
	if(screen_surface->w != 320) {
		SDL_FreeSurface(screen_surface);
		screen_surface = SDL_SetVideoMode (320, 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	}
#endif
}

void SetGameMode(void)
{
#ifdef GP2X
	int hsc, vsc, hw, yres;

	if((option_rescale == 3) && screen_mode) option_rescale = 2;

	switch(option_rescale) {
		case 0:
		case 1:
			hw = 640;
			yres = 240;
			break;
		case 2:
			hw = 768;
			yres = 224;
			break;
		case 3:
			hw = 768;
			yres = 240;
			break;
	}

	switch(screen_mode) {
		case 0: // LCD
			hsc = (1024 * hw) / 640;
			vsc = (yres * hw) / 240;
			break;
		case 1: // TV PAL
			hsc = (512 * hw) / 640;
			vsc = (yres * hw) / 288;
			break;
		case 2: // TV NTSC
			hsc = (512 * hw) / 640;
			vsc = (yres * hw) / 240;
			break;
	}

	memset(screen, 0, 0x40000);

	gp2xregs[0x2906>>1] = hsc & 0xFFFF;
	gp2xregs[0x2908>>1] = vsc & 0xFFFF;
	gp2xregs[0x290A>>1] = vsc >> 16;
	gp2xregs[0x290C>>1] = hw;
#else
	if((option_rescale >= 2) && (screen_surface->w != 384)) {
		SDL_FreeSurface(screen_surface);
		screen_surface = SDL_SetVideoMode (384, option_rescale == 2 ? 224 : 240, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	}
#endif
}

/*--------------------------------------------------------
	Draw text & message
--------------------------------------------------------*/

void msg_screen_init()
{
	SetTextMode();
#ifndef GP2X
	SDL_LockSurface(screen_surface);
	screen = screen_surface->pixels;
#endif
	memset(screen, 0x12, 320 * 8 * 2);
	memset(screen + 320 * 8, 0, 320 * 232 * 2);
#ifndef GP2X
	SDL_UnlockSurface(screen_surface);
	screen = NULL;
#endif
	msg_line = 0;
	msg_printf(EMULATOR_TITLE);
}

void msg_screen_clear()
{
	SetGameMode();

	if(option_rescale >= 2) {
		option_showfps = 0;
		msg_line = -2;
	} else {
#ifndef GP2X
		SDL_LockSurface(screen_surface);
		screen = screen_surface->pixels;
#endif
		if(option_showtitle) {
			title_show = 0;
		} else {
			title_bg = 0;
			title_show = 1;
		}
		memset(screen, title_bg & 0xff, 320 * 8 * 2);
		memset(screen + 320 * 8, 0, 320 * 232 * 2);
#ifndef GP2X
		SDL_UnlockSurface(screen_surface);
		screen = NULL;
#endif

		char str[64];
		if(option_showtitle) strcpy(str, EMULATOR_TITLE);
		else if(parent_name[0]) sprintf(str, "Game: %s(%s)", game_name, parent_name);
		else sprintf(str, "Game: %s", game_name);

		msg_line = 0;
		msg_printf(str);

		msg_line = -1;
		msg_printf("Game Start(%d Button Game)\n", input_max_buttons);
	}
}

static const char *draw_text(const char *text, int x, int y, int fg, int bg)
{
	u16 *dst;
	const char *p = text;
	int trans = (bg < 0) || (bg > 0xffff);

#ifndef GP2X
	SDL_LockSurface(screen_surface);
	screen = screen_surface->pixels;
#endif

	dst = screen + y * 320;

	while(*p) {
		if((*p == 0x20) && (trans)) {
			x += 8;
		} else if(*p >= 0x20) {
			int w, h;
			u8 *font = (u8 *)&font_data[(*p - 0x20) * 8];
			u16 *put = dst + x;

			for(h = 8; h; --h) {
				u8 c = *font++;
				for(w = 8; w; --w, ++put) {
					if(c & 0x80) {
						*put = fg;
					} else if(!trans) {
						*put = bg;
					}
					c <<= 1;
				}
				put += 320 - 8;
			}
			x += 8;
		}
		++p;
		if(x > 312)
			break;
	}

#ifndef GP2X
	SDL_UnlockSurface(screen_surface);
	screen = NULL;
#endif

	return p;
}

static void clean_msg()
{
#ifdef GP2X
	if(title_show) {
		int width = strlen(EMULATOR_TITLE) * 8 * 2;
		int i;
		for(i = 0; i < 8; i++)
			memset(screen + 320 * i, title_bg & 0xff, width);
		title_show = 0;
	}
	memset(screen + 320 * 232, 0x00, 320 * 8 * 2);
#else
	if(title_show) {
		SDL_Rect r = {0, 0, strlen(EMULATOR_TITLE) * 8, 8};
		SDL_FillRect(screen_surface, &r, title_bg);
		title_show = 0;
	}
	SDL_Rect r = {0, 232, 320, 8};
	SDL_FillRect(screen_surface, &r, 0x0000);
	SDL_Flip(screen_surface);
#endif
}

void msg_printf(const char *text, ...)
{
	u16 *dst;
	va_list arg;
	char buf[512];
	const char *p;
	int y;

	va_start(arg, text);
	vsprintf(buf, text, arg);
	va_end(arg);

	if(msg_line == 0) {
		printf("Message : %s\n", buf);
	} else {
		printf("%s", buf);
	}

	if(msg_line == -2) return;

	if(msg_line == -1) {
		y = 232;
		msg_count = 60*3;
	} else {
		y = msg_line * 8;
	}

	p = buf;
	do {
		if(msg_line != -1) {
			++msg_line;
			if(y == 240) {
#ifndef GP2X
				SDL_LockSurface(screen_surface);
				screen = screen_surface->pixels;
#endif
				memcpy(screen + 320 * 8, screen + 320 * 16, 320 * 224 * 2);
				msg_line = 30;
				y -= 8;
#ifndef GP2X
				SDL_UnlockSurface(screen_surface);
				screen = NULL;
#endif
			}
			if(y != 0) {
#ifdef GP2X
				memset(screen + y * 320, 0, 320 * 8 * 2);
#else
				SDL_Rect r = {0, y, 320, 8};
				SDL_FillRect(screen_surface, &r, 0);
#endif
			}
		} else {
#ifdef GP2X
			memset(screen + y * 320, 0x00, 320 * 8 * 2);
#else
			SDL_Rect r = {0, y, 320, 8};
			SDL_FillRect(screen_surface, &r, 0x0000);
#endif
		}
		p = draw_text(p, 0, y, 0xBDF7, -1);
		y += 8;
	} while(*p && (msg_line != -1));

#ifndef GP2X
	SDL_Flip(screen_surface);
#endif
}

void button_wait()
{
#ifdef GP2X
	u32 buttons;
	if(joy_fd < 0) return;

	while(1) {
		usleep(0);
		if((read(joy_fd, &buttons, 4) == 4) && !(buttons & 0x7ff00))
			break;
	}

	while(1) {
		usleep(0);
		if((read(joy_fd, &buttons, 4) == 4) && (buttons & 0x7ff00))
			break;
	}

	while(1) {
		usleep(0);
		if((read(joy_fd, &buttons, 4) == 4) && !(buttons & 0x7ff00))
			break;
	}

#else
	SDL_Event event;

	while(1) {
		SDL_Delay(1);
		if(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					Loop = LOOP_EXIT;
					return;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
					return;
			}
		}
	}
#endif
}


/*------------------------------------------------------
	System menu
------------------------------------------------------*/
static int menu_cursor = 0;
static int menu_pressed = 0;
static int menu_frameskip = 0;

void menu_init()
{
	SetTextMode();
#ifndef GP2X
	SDL_LockSurface(screen_surface);
	screen = screen_surface->pixels;
#endif
	memset(screen, 0x12, 320 * 8 * 2);
	memset(screen + 320 * 8, 0, 320 * 232 * 2);
#ifndef GP2X
	SDL_UnlockSurface(screen_surface);
	screen = NULL;
#endif
	msg_line = 0;
	msg_printf(EMULATOR_TITLE);
	msg_line = -1;
	menu_mode = 0;

	menu_pressed = 0;
}

void menu_clear()
{
	SetGameMode();

	if(option_rescale >= 2) {
		option_showfps = 0;
		msg_line = -2;
	} else {
#ifndef GP2X
		SDL_LockSurface(screen_surface);
		screen = screen_surface->pixels;
#endif
		if(option_showtitle) title_show = 0;
		memset(screen, title_bg & 0xff, 320 * 8 * 2);
		memset(screen + 320 * 8, 0, 320 * 232 * 2);
#ifndef GP2X
		SDL_UnlockSurface(screen_surface);
		screen = NULL;
#endif

		char str[64];
		if(option_showtitle) {
			strcpy(str, EMULATOR_TITLE);
			msg_line = 0;
			msg_printf(str);
		}

		msg_line = -1;
	}
}

enum {
	MENU_FRAME = 0,
	MENU_LIMIT,
	MENU_SLOT,
	MENU_SAVE,
	MENU_LOAD,
	MENU_RESCALE,
	MENU_SHOWFPS,
	MENU_LINESCROLL,
	MENU_SERVICE,
	MENU_USBREFRESH,
	MENU_RESET,
	MENU_RETURN,
	MENU_EXIT,
	MENU_COUNT
};

void menu_display(void)
{
	char str[128];
	int y = 40;
	const char *scale_item[] = {"None", "Software", "Hardware", "Hardware(Horizontal only)"};

#ifdef GP2X
	u16 *gp2x_screen = screen;
	u16 buffer[320*240];

	memset(buffer, 0, sizeof(buffer));
	screen = buffer;
#else
	SDL_Rect rect = {0, 8, 320, 224};
	SDL_FillRect(screen_surface, &rect, 0);
#endif

	if(menu_frameskip == -1)
		sprintf(str, "Frame skip: Auto");
	else
		sprintf(str, "Frame skip: %d", menu_frameskip);
	draw_text(str, 40, y, 0xBDF7, 0); y += 8;
	sprintf(str, "Speed limit: %s", option_speedlimit ? "On" : "Off");
	draw_text(str, 40, y, 0xBDF7, 0); y += 8;
	sprintf(str, "State slot: %d", state_slot);
	draw_text(str, 40, y, 0xBDF7, 0);y += 8;
	draw_text("Save state", 40, y, 0xBDF7, 0);y += 8;
	draw_text("Load state", 40, y, 0xBDF7, 0);y += 8;
	sprintf(str, "Rescale: %s", scale_item[option_rescale]);
	draw_text(str, 40, y, 0xBDF7, 0);y += 8;
	if(option_rescale < 2)
		sprintf(str, "Show FPS: %s", option_showfps ? "On" : "Off");
	else
		sprintf(str, "Show FPS: Disable");
	draw_text(str, 40, y, 0xBDF7, 0);y += 8;
	sprintf(str, "Line scroll: %s", option_linescroll ? "On" : "Off");
	draw_text(str, 40, y, 0xBDF7, 0);y += 8;
	draw_text("Enter service mode", 40, y, 0xBDF7, 0);y += 8;
	draw_text("USB device refresh", 40, y, 0xBDF7, 0);y += 8;
	draw_text("Reset game", 40, y, 0xBDF7, 0);y += 8;
	draw_text("Return to game", 40, y, 0xBDF7, 0);y += 8;
	draw_text("Exit", 40, y, 0xBDF7, 0);y += 8;

	draw_text(">", 24, 40 + (menu_cursor * 8), 0xFFFF, 0);

	switch(menu_cursor) {
		case MENU_FRAME:
			msg_printf("Left/Right : Change frame skip level\n");
			break;
		case MENU_SLOT:
			msg_printf("Left/Right : Change state slot\n");
			break;
		case MENU_RESCALE:
		case MENU_SHOWFPS:
			msg_printf("Left/Right : Change option\n");
			break;
		default:
			msg_printf("Button : Select\n");
			break;
	}

#ifdef GP2X
	screen = gp2x_screen;
	memcpy(screen + 320*8, buffer + 320*8, 320 * 232 * 2);
#else
	SDL_Flip(screen_surface);
#endif
}

enum {
	MENU_UP = 0,
	MENU_DOWN,
	MENU_LEFT,
	MENU_RIGHT,
	MENU_SELECT,
	MENU_TOGAME
};

int menu_button(void)
{
#ifdef GP2X
	u32 gp2xbtn;
	int i;

	usleep(0);
	if((joy_fd < 0) || (read(joy_fd, &gp2xbtn, 4) != 4)) gp2xbtn = 0;

	menu_pressed = menu_pressed & ~BTN(MENU_UP) | (gp2xbtn & BTN(GPX_VK_UP) ? BTN(MENU_UP) : 0);
	menu_pressed = menu_pressed & ~BTN(MENU_DOWN) | (gp2xbtn & BTN(GPX_VK_DOWN) ? BTN(MENU_DOWN) : 0);
	menu_pressed = menu_pressed & ~BTN(MENU_LEFT) | (gp2xbtn & BTN(GPX_VK_LEFT) ? BTN(MENU_LEFT) : 0);
	menu_pressed = menu_pressed & ~BTN(MENU_RIGHT) | (gp2xbtn & BTN(GPX_VK_RIGHT) ? BTN(MENU_RIGHT) : 0);
	menu_pressed = menu_pressed & ~BTN(MENU_SELECT) | (gp2xbtn & (BTN(GPX_VK_FB) | BTN(GPX_VK_FA) | BTN(GPX_VK_TAT)) ? BTN(MENU_SELECT) : 0);
	menu_pressed = menu_pressed & ~BTN(MENU_TOGAME) | (gp2xbtn & BTN(GPX_VK_FX) ? BTN(MENU_TOGAME) : 0);
#else
	SDL_Event event;
	int button = -1;

	SDL_Delay(1);
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
			case SDL_QUIT:
				Loop = LOOP_EXIT;
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym) {
					case SDLK_ESCAPE: Loop = LOOP_EXIT; break;
				}
			case SDL_KEYUP:
				button = -1;
				switch(event.key.keysym.sym) {
					case SDLK_UP: button = MENU_UP; break;
					case SDLK_DOWN: button = MENU_DOWN; break;
					case SDLK_LEFT: button = MENU_LEFT; break;
					case SDLK_RIGHT: button = MENU_RIGHT; break;
					case SDLK_RETURN: button = MENU_SELECT; break;
					case 'a': button = MENU_SELECT; break;
					case SDLK_F12: button = MENU_TOGAME; break;
				}
				if(button != -1)
					if(event.type == SDL_KEYDOWN)
						menu_pressed |= BTN(button);
					else
						menu_pressed &= ~BTN(button);
				break;
		}
	}
#endif
	return menu_pressed;
}

void menu_on(void)
{
	int prev, change;
	int menu_draw;
	int menu_do;
	int menu_volume = option_sound_volume;

	int menu_dir, item_dir;
	TICKER menu_tick, item_tick, curr_tick;

	int btn;

	if(Loop == LOOP_EXIT) return;

	sound_mute(1);
	menu_init();
	menu_pressed = 0;

	menu_draw = 1;
	menu_do = 1;
	if(option_autoframeskip) menu_frameskip = -1;
	else menu_frameskip = option_frameskip;

	menu_dir = item_dir = 0;
	menu_tick = item_tick = 0;

	prev = 0;
	while (menu_do && (Loop != LOOP_EXIT)) {
		if(menu_draw) {
			if(menu_cursor < 0) menu_cursor = MENU_COUNT - 1;
			if(menu_cursor >= MENU_COUNT) menu_cursor = 0;
			if(state_slot < 0) state_slot = 9;
			if(state_slot > 9) state_slot = 0;
			if(menu_frameskip < -1) menu_frameskip = -1;
			if(menu_frameskip > 11) menu_frameskip = 11;
			option_rescale &= 0x03;
			menu_display();
			menu_draw = 0;
		}

		btn = menu_button();
		change = prev ^ btn;
		prev = btn;
		curr_tick = get_ticks();

		if(btn & (BTN(MENU_UP) | BTN(MENU_DOWN))) {
			int val = (btn & BTN(MENU_UP)) ? -1 : 1;
			if((menu_tick < curr_tick) || (menu_dir != val)){
				menu_cursor += val;
				menu_draw = 1;
				if(menu_dir == val)
					menu_tick += 100000;
				else
					menu_tick = curr_tick + 500000;
				menu_dir = val;
			}
		} else if(menu_dir) menu_dir = 0;

		if((menu_dir == 0) && (btn & (BTN(MENU_LEFT) | BTN(MENU_RIGHT)))) {
			int val = (btn & BTN(MENU_LEFT)) ? -1 : 1;
			if((item_tick < curr_tick) || (item_dir != val)){
				switch(menu_cursor) {
					case MENU_FRAME:
						menu_frameskip += val;
						break;
					case MENU_LIMIT:
						option_speedlimit = !option_speedlimit;
						break;
					case MENU_SLOT:
						state_slot += val;
						break;
					case MENU_RESCALE:
						option_rescale += val;
						break;
					case MENU_SHOWFPS:
						option_showfps = !option_showfps;
						break;
					case MENU_LINESCROLL:
						option_linescroll = !option_linescroll;
						break;
				}
				menu_draw = 1;
				if(item_dir == val)
					item_tick += 100000;
				else
					item_tick = curr_tick + 500000;
				item_dir = val;
			}
		} else if(item_dir) item_dir = 0;

		if(change & btn & BTN(MENU_SELECT)) {
			switch(menu_cursor) {
				/*case MENU_SAVE: state_save(state_slot); break;
				case MENU_LOAD: if(state_load(state_slot)) menu_do = 0; break;*/
				case MENU_SERVICE: service_mode = 1; menu_do = 0; break;
				case MENU_USBREFRESH: open_joystick(); break;
				case MENU_RESET: Loop = LOOP_RESET; menu_do = 0; break;
				case MENU_RETURN: menu_do = 0; break;
				case MENU_EXIT: Loop = LOOP_EXIT; break;
			}
		}

		if(change & btn & BTN(MENU_TOGAME)) menu_do = 0;
	}

	if(menu_frameskip == -1) {
		option_autoframeskip = 1;
	} else {
		option_autoframeskip = 0;
		option_frameskip = menu_frameskip;
	}

	if(Loop != LOOP_EXIT) {
		sound_mute(0);
		menu_clear();
	}
}

/*------------------------------------------------------
	Save snapshot
------------------------------------------------------*/

void save_snapshot(void)
{
	// not supported.
/*
	char path[MAX_PATH];

	if (snap_no == -1)
	{
		FILE *fp;

		snap_no = 1;

		while (1)
		{
			sprintf(path, "%ssnap/%s_%02d.bmp", launchDir, game_name, snap_no);
			if ((fp = fopen(path, "rb")) == NULL) break;
			fclose(fp);
			snap_no++;
		}
	}

	sprintf(path, "%ssnap/%s_%02d.bmp", launchDir, game_name, snap_no);
	save_bmp(path);
	ui_popup("Snapshot saved as \"%s_%02d.bmp\".", game_name, snap_no++);
*/
}
