/*	hiscore.c
**	generalized high score save/restore support
*/

//#include "driver.h"
#include "hiscore.h"
#include "emumain.h"

#define MAX_CONFIG_LINE_SIZE 80

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	logerror x
#else
#define LOG(x)  printf x
#endif

char *db_filename = "hiscore.dat"; /* high score definition file */

static struct
{
	int hiscores_have_been_loaded;
	int hiscore_file_size;

	struct mem_range
	{
		unsigned int cpu, addr, num_bytes, start_value, end_value;
		struct mem_range *next;
	} *mem_range;
} state;

//extern char *game_name;
//extern char *launchDir;

/*****************************************************************************/

#if 0
#define MEMORY_READ(index,offset) \
    ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK]. \
    memory_read)(offset))
#define MEMORY_WRITE(index,offset,data) \
    ((*cpuintf[Machine->drv->cpu[index].cpu_type & ~CPU_FLAGS_MASK]. \
    memory_write)(offset,data))

void computer_writemem_byte(int cpu, int addr, int value)
{
    int oldcpu = cpu_getactivecpu();
    memory_set_context(cpu);
    MEMORY_WRITE(cpu, addr, value);
    if (oldcpu != cpu)
		memory_set_context(oldcpu);
}

int computer_readmem_byte(int cpu, int addr)
{
    int oldcpu = cpu_getactivecpu(), result;
    memory_set_context(cpu);
    result = MEMORY_READ(cpu, addr);
    if (oldcpu != cpu)
    	memory_set_context(oldcpu);
    return result;
}
#endif

/*****************************************************************************/

static void copy_to_memory (int cpu, int addr, const unsigned char *source, int num_bytes)
{
	int i;
	for (i=0; i<num_bytes; i++)
	{
		//computer_writemem_byte (cpu, addr+i, source[i]);
		m68000_write_memory_8(addr+i, source[i]);
	}
}

static void copy_from_memory (int cpu, int addr, unsigned char *dest, int num_bytes)
{
	int i;
	for (i=0; i<num_bytes; i++)
	{
		//dest[i] = computer_readmem_byte (cpu, addr+i);
		dest[i] =  m68000_read_memory_8(addr+i);
	}
}

/*****************************************************************************/

/*	hexstr2num extracts and returns the value of a hexadecimal field from the
	character buffer pointed to by pString.

	When hexstr2num returns, *pString points to the character following
	the first non-hexadecimal digit, or NULL if an end-of-string marker
	(0x00) is encountered.

*/
static unsigned int hexstr2num (const char **pString)
{
	const char *string = *pString;
	unsigned int result = 0;
	if (string)
	{
		for(;;)
		{
			char c = *string++;
			int digit;

			if (c>='0' && c<='9')
			{
				digit = c-'0';
			}
			else if (c>='a' && c<='f')
			{
				digit = 10+c-'a';
			}
			else if (c>='A' && c<='F')
			{
				digit = 10+c-'A';
			}
			else
			{
				/* not a hexadecimal digit */
				/* safety check for premature EOL */
				if (!c) string = NULL;
				break;
			}
			result = result*16 + digit;
		}
		*pString = string;
	}
	return result;
}

/*	given a line in the hiscore.dat file, determine if it encodes a
	memory range (or a game name).
	For now we assume that CPU number is always a decimal digit, and
	that no game name starts with a decimal digit.
*/
static int is_mem_range (const char *pBuf)
{
	char c;
	for(;;)
	{
		c = *pBuf++;
		if (c == 0) return 0; /* premature EOL */
		if (c == ':') break;
	}
	c = *pBuf; /* character following first ':' */

	return	(c>='0' && c<='9') ||
			(c>='a' && c<='f') ||
			(c>='A' && c<='F');
}

/*	matching_game_name is used to skip over lines until we find <game_name>: */
static int matching_game_name (const char *pBuf)
{
	const char *name = game_name;
	while (*name)
		if (*name++ != *pBuf++) return 0;
	return (*pBuf == ':');
}

/*****************************************************************************/

/* safe_to_load checks the start and end values of each memory range */
static int safe_to_load (void)
{
	struct mem_range *mem_range = state.mem_range;
	while (mem_range)
	{
		//if (computer_readmem_byte (mem_range->cpu, mem_range->addr) !=
		if (m68000_read_memory_8(mem_range->addr) !=
			mem_range->start_value)
		{
			//printf ("Start Want: %d Got:%d\n", mem_range->start_value, m68000_read_memory_8(mem_range->addr));
			return 0;
		}
		//if (computer_readmem_byte (mem_range->cpu, mem_range->addr + mem_range->num_bytes - 1) !=
		if (m68000_read_memory_8 (mem_range->addr + mem_range->num_bytes - 1) !=
			mem_range->end_value)
		{
			//printf ("End Want: %d Got:%d\n", mem_range->start_value, m68000_read_memory_8(mem_range->addr + mem_range->num_bytes  - 1));
			return 0;
		}
		mem_range = mem_range->next;
	}
	return 1;
}

/* hs_free disposes of the mem_range linked list */
static void hs_free (void)
{
	struct mem_range *mem_range = state.mem_range;
	while (mem_range)
	{
		struct mem_range *next = mem_range->next;
		free (mem_range);
		mem_range = next;
	}
	state.mem_range = NULL;
}

static void hs_load (void)
{
	FILE *f = NULL;
	char path[MAX_PATH];

        sprintf(path, "%shi/%s.hi", launchDir, game_name);

	if ((f = fopen(path, "rb")) == NULL) {
		msg_printf("High score file not found.\n");
	} else {
		struct mem_range *mem_range = state.mem_range;
		unsigned char *data = malloc (state.hiscore_file_size);
		if (data) {
			while (mem_range)
			{
				/*	this buffer will almost certainly be small
					enough to be dynamically allocated, but let's
					avoid memory trashing just in case
				*/
				fread (data, 1, mem_range->num_bytes, f);
				copy_to_memory (mem_range->cpu, mem_range->addr, data, mem_range->num_bytes);
				mem_range = mem_range->next;
			}
			free (data);
			msg_printf("High score loaded.\n");
		} else {
			msg_printf("High score loading failed.\n");
		}
		fclose (f);
	}
}

static void hs_save (void)
{
	unsigned char *hs_cmp = malloc(state.hiscore_file_size);
	unsigned char *data = malloc (state.hiscore_file_size);

	if(!hs_cmp || !data) {
		msg_printf("High score save failed.\n");
	} else {
		struct mem_range *mem_range = state.mem_range;
		unsigned char *cmp = hs_cmp;
		char path[MAX_PATH];
		FILE *f = NULL;
		int ret = 0;

                sprintf(path, "%shi/%s.hi", launchDir, game_name);

		if ((f = fopen(path, "rb")) != NULL) {
			ret = fread(hs_cmp, 1, state.hiscore_file_size, f);
			fclose(f);
		}

		while (mem_range)
		{
			/*	this buffer will almost certainly be small
				enough to be dynamically allocated, but let's
				avoid memory trashing just in case
			*/
			copy_from_memory (mem_range->cpu, mem_range->addr, data, mem_range->num_bytes);
			if((ret != state.hiscore_file_size) || memcmp(data, cmp, mem_range->num_bytes)) {
				memcpy(cmp, data, mem_range->num_bytes);
				if(ret) ret = 0;
			}

			cmp += mem_range->num_bytes;
			mem_range = mem_range->next;
		}

		if(!ret) {
			if ((f = fopen(path, "wb")) == NULL) {
				msg_printf("High score save failed.\n");
			} else {
				fwrite(hs_cmp, 1, state.hiscore_file_size, f);
				fclose(f);
				msg_printf("High score saved.\n");
			}
		}
	}

	if(hs_cmp) free(hs_cmp);
	if(data) free(data);
}

/*****************************************************************************/
/* public API */

void hs_clear (void)
{
	state.hiscores_have_been_loaded = 0;
	state.hiscore_file_size = 0;
	state.mem_range = NULL;
}

/* call hs_open once after loading a game */
void hs_open (void)
{
	FILE *f = NULL;
	char path[MAX_PATH];
#ifdef N900
        sprintf(path, "%s%s", "/opt/cps2emu/config/", db_filename);
#else
        sprintf(path, "%s%s", launchDir, db_filename);
#endif
	if ((f = fopen(path, "rb")) == NULL) {
		msg_printf("High score DB file is broken.\n");
	} else {
		char buffer[MAX_CONFIG_LINE_SIZE];
		enum { FIND_NAME, FIND_DATA, FETCH_DATA } mode;
		mode = FIND_NAME;

		hs_free();
		state.hiscore_file_size = 0;
		//LOG(("hs_open: '%s'\n", game_name));
		while (fgets (buffer, MAX_CONFIG_LINE_SIZE, f))
		{
			if (mode==FIND_NAME)
			{
				if (matching_game_name (buffer))
				{
					mode = FIND_DATA;
					//LOG(("hs config found!\n"));
				}
			}
			else if (is_mem_range (buffer))
			{
				const char *pBuf = buffer;
				struct mem_range *mem_range = malloc(sizeof(struct mem_range));
				if (mem_range)
				{
					mem_range->cpu = hexstr2num (&pBuf);
					mem_range->addr = hexstr2num (&pBuf);
					mem_range->num_bytes = hexstr2num (&pBuf);
					mem_range->start_value = hexstr2num (&pBuf);
					mem_range->end_value = hexstr2num (&pBuf);
					state.hiscore_file_size += mem_range->num_bytes;

					mem_range->next = NULL;
					{
						struct mem_range *last = state.mem_range;
						while (last && last->next) last = last->next;
						if (last == NULL)
							state.mem_range = mem_range;
						else
							last->next = mem_range;
					}

					mode = FETCH_DATA;
				}
				else
				{
					hs_free();
					break;
				}
			}
			else
			{
				/* line is a game name */
				if (mode == FETCH_DATA) break;
			}
		}
		fclose (f);
	}

	state.hiscores_have_been_loaded = 0;
	if (state.mem_range) {
		struct mem_range *mem_range = state.mem_range;

		while (mem_range)
		{
			//computer_writemem_byte(
			//	mem_range->cpu,
			//	mem_range->addr,
			//	~mem_range->start_value
			//);
			m68000_write_memory_8(mem_range->addr, ~mem_range->start_value);
	
			//computer_writemem_byte(
			//	mem_range->cpu,
			//	mem_range->addr + mem_range->num_bytes-1,
			//	~mem_range->end_value
			//);
			m68000_write_memory_8(mem_range->addr + mem_range->num_bytes-1,~mem_range->end_value);
			mem_range = mem_range->next;
		}

		//printf ("cpu:%d, addr:%d, bytes:%d, start_value:%d, end_value:%d\n", state.mem_range->cpu, state.mem_range->addr, state.mem_range->num_bytes, state.mem_range->start_value, state.mem_range->end_value);
	} else {
		option_hiscore = 0;
	}
}

/* call hs_update periodically (i.e. once per frame) */
void hs_update (void)
{
	if (state.mem_range && !state.hiscores_have_been_loaded && safe_to_load()) {
		hs_load();
		state.hiscores_have_been_loaded = 1;
		option_hiscore = 0;
	}
}

/* call hs_close when done playing game */
void hs_close (void)
{
	if (state.hiscores_have_been_loaded) hs_save();
	hs_free();
}
