/******************************************************************************

	cache.c

	Memory cache interface function

******************************************************************************/

#include "emumain.h"

#if USE_CACHE

#define MIN_CACHE_SIZE		0x40		// Minimum  4MB
#define MAX_CACHE_SIZE		0x200		// Maximum 32MB
#define BLOCK_SIZE			0x10000		// 1 Block size = 64KB
#define BLOCK_MASK			0xffff
#define BLOCK_SHIFT			16
#define BLOCK_NOT_CACHED	0xffff
#define BLOCK_EMPTY			0xffffffff

#define GFX_MEMORY			memory_region_gfx1
#define GFX_SIZE			memory_length_gfx1
#define CHECK_FNAME			"block_empty"

/******************************************************************************
	Global variable
******************************************************************************/

u32 (*read_cache)(u32 offset);

u32 block_offset[MAX_CACHE_BLOCKS];
u8  *block_empty = (u8 *)block_offset;
u32 block_start;
u32 block_size;
u32 block_capacity;
u8  *block_data = NULL;

int cache_type;


/******************************************************************************
	Local struct/variable
******************************************************************************/

typedef struct cache_s
{
	int idx;
	int block;
	struct cache_s *prev;
	struct cache_s *next;
} cache_t;


static cache_t ALIGN_DATA cache_data[MAX_CACHE_SIZE];
static cache_t *head;
static cache_t *tail;

static int num_cache;
static u16 ALIGN_DATA blocks[MAX_CACHE_BLOCKS];
static char spr_cache_name[MAX_PATH];
static FILE* cache_fd;


/******************************************************************************
	Local function
******************************************************************************/


static inline void load_block(int index, int offset)
{
    int size, length;
    u8 src[BLOCK_SIZE];
    u8 dst[BLOCK_SIZE];

	size = block_data[offset] | (block_data[offset + 1] << 8);
	if(size) {
    	length = BLOCK_SIZE;
#ifdef MMUHACK
    	uncompress(&GFX_MEMORY[index << BLOCK_SHIFT], &length, &block_data[offset + 2], size);
#else
    	memcpy(src, &block_data[offset + 2], size);
    	uncompress(dst, &length, src, size);
    	memcpy(&GFX_MEMORY[index << BLOCK_SHIFT], dst, length);
#endif
    } else {
    	memcpy(&GFX_MEMORY[index << BLOCK_SHIFT], &block_data[offset + 2], BLOCK_SIZE);
    }
	//if(length != 0x10000) msg_printf("Cache is broken(Block %d)...\n", block);
}

/*------------------------------------------------------
	«­«ã«Ã«·«åªò«Ç£­«¿ªÇØØªáªë

	3ðú×¾ªÎ«Ç£­«¿ª¬ûèî¤ª·ªÆª¤ªëª¿ªá£¬ÖÅæ´ªòÏ¡ï·ªÃªÆ
	ª½ªìª¾ªìîêÓ×ªÊ«µ«¤«ºªòÔÁªß¢ÃªàªÙª­ªÇª¹ª¬£¬â¢ªò
	Úûª¤ªÆà»Ôéª«ªéÔÁªß¢ÃªóªÇª¤ªëªÀª±ªËªÊªÃªÆª¤ªÞª¹£®
------------------------------------------------------*/

static int fill_cache(void)
{
	int i, block, offset, size, length;
	int block_free = 0;
	cache_t *p;

	i = 0;
	block = 0;
	
	if(block_data == NULL) {
        block_data = (u8 *)malloc(block_size);
        block_free = 1;
    }
    
    msg_printf("Loading cache data... \n");

    //lseek(cache_fd, block_start, SEEK_SET);
    //read(cache_fd, block_data, block_size);
    fseek(cache_fd, block_start, SEEK_SET);
    fread(block_data, sizeof(char), block_size, cache_fd);
     
    msg_printf("Fill cache data... 0%%\n");

	while (i < num_cache)
	{
		if (block_offset[block] != BLOCK_EMPTY)
		{
			p = head;
			p->block = block;
			blocks[block] = p->idx;
			
			load_block(p->idx, block_offset[block]);
			/*
			offset = block_offset[block];
			size = block_data[offset] | (block_data[offset + 1] << 8);
			length = BLOCK_SIZE;
			uncompress(&GFX_MEMORY[p->idx << BLOCK_SHIFT], &length, &block_data[offset + 2], size);
			*/

			head = p->next;
			head->prev = NULL;

			p->prev = tail;
			p->next = NULL;

			tail->next = p;
			tail = p;
			i++;
			if((i % 20) == 0)
                msg_printf("Fill cache data... %d%%\n", i * 100 / num_cache);
		}

		if (++block >= MAX_CACHE_BLOCKS)
			break;
	}

	if((i % 20) != 0)
        msg_printf("Fill cache data... Complete\n");
	
	if(block_free) {
        free(block_data);
        block_data = NULL;
    }

	return 1;
}


/*------------------------------------------------------
	«¢«É«ì«¹Ü¨üµªÎªßú¡ª¦

	Íöª­ÖÅæ´ªòÞûð¶ª·ª¿ßÒ÷¾ªÇ£¬îïªÆ«á«â«êªËÌ«Ò¡ªµªìªÆ
	ª¤ªëíÞùê
------------------------------------------------------*/

static u32 read_cache_static(u32 offset)
{
	int idx = blocks[offset >> BLOCK_SHIFT];

	return ((idx << BLOCK_SHIFT) | (offset & BLOCK_MASK));
}


/*------------------------------------------------------
	Ùíäâõê«­«ã«Ã«·«åªòÞÅéÄ

	ÙíäâõêªÎ«­«ã«Ã«·«å«Õ«¡«¤«ëª«ªé«Ç£­«¿ªòÔÁªß¢Ãªà
------------------------------------------------------*/

static u32 read_cache_compress(u32 offset)
{
    int s;
	s16 new_block = offset >> BLOCK_SHIFT;
	u32 idx = blocks[new_block];
	cache_t *p;

	if (idx == BLOCK_NOT_CACHED)
	{
        int offset, size;
		p = head;
		blocks[p->block] = BLOCK_NOT_CACHED;

		p->block = new_block;
		blocks[new_block] = p->idx;

		load_block(p->idx, block_offset[new_block]);
		/*
		offset = block_offset[new_block];
		size = block_data[offset] | (block_data[offset + 1] << 8);
		s = BLOCK_SIZE;
		uncompress(&GFX_MEMORY[p->idx << BLOCK_SHIFT], &s, &block_data[offset + 2], size);
		*/
		//printf("read %x cache %d bytes...\n", new_block, s);
	}
	else p = &cache_data[idx];

	if (p->next)
	{
		if (p->prev)
		{
			p->prev->next = p->next;
			p->next->prev = p->prev;
		}
		else
		{
			head = p->next;
			head->prev = NULL;
		}

		p->prev = tail;
		p->next = NULL;

		tail->next = p;
		tail = p;
	}

	return ((tail->idx << BLOCK_SHIFT) | (offset & BLOCK_MASK));
}


/******************************************************************************
	«­«ã«Ã«·«å«¤«ó«¿«Õ«§£­«¹Î¼Þü
******************************************************************************/

/*------------------------------------------------------
	Cache initialize
------------------------------------------------------*/

void cache_init(void)
{
	int i;

	num_cache  = 0;
	cache_fd   = NULL;
	cache_type = CACHE_NOTFOUND;

	read_cache = read_cache_static;

	for (i = 0; i < MAX_CACHE_BLOCKS; i++)
		blocks[i] = BLOCK_NOT_CACHED;
}


/*------------------------------------------------------
	«­«ã«Ã«·«åô¥×âËÒã·
------------------------------------------------------*/

int cache_start(void)
{
	int i;
	u32 size = 0;
	cache_fd = cachefile_open();

	if (cache_fd < 0)
	{
		msg_printf("ERROR: Could not open cache file.\n");
		return 0;
	}

    GFX_MEMORY = upper_memory;

    if(!GFX_MEMORY) {
		msg_printf("ERROR: Could not allocate cache memory.\n");
		return 0;
    }

    if(block_capacity <= CACHE_SIZE) {
    	read_cache = read_cache_static;
		num_cache = block_capacity >> BLOCK_SHIFT;
        block_data = NULL;
    } else {
		read_cache = read_cache_compress;
		if(option_fullcache) {
			// block data to lower 32MB area
			num_cache = CACHE_SIZE >> BLOCK_SHIFT;
			block_data = malloc(block_size);
		} else {
			// block data to upper 32MB area
			num_cache = ((CACHE_SIZE - block_size) & ~0xffff) >> BLOCK_SHIFT;
			block_data = &GFX_MEMORY[num_cache << BLOCK_SHIFT];
		}
    }

	msg_printf("%dKB cache allocated.\n", (num_cache << BLOCK_SHIFT) / 1024);

	for (i = 0; i < num_cache; i++)
		cache_data[i].idx = i;

	for (i = 1; i < num_cache; i++)
		cache_data[i].prev = &cache_data[i - 1];

	for (i = 0; i < num_cache - 1; i++)
		cache_data[i].next = &cache_data[i + 1];

	cache_data[0].prev = NULL;
	cache_data[num_cache - 1].next = NULL;

	head = &cache_data[0];
	tail = &cache_data[num_cache - 1];

	if (!fill_cache())
	{
		msg_printf("Cache load error!!!\n");
		pad_wait_press(PAD_WAIT_INFINITY);
		Loop = LOOP_EXIT;
		return 0;
	}

	if (cache_fd != NULL)
	{
		fclose(cache_fd);
		cache_fd = NULL;
	}

	msg_printf("Cache setup complete.\n");

	return 1;
}


/*------------------------------------------------------
	«­«ã«Ã«·«åô¥×âðûÖõ
------------------------------------------------------*/

void cache_shutdown(void)
{
	num_cache = 0;
	if(block_data != NULL) free(block_data);
	block_data = NULL;
}


/*------------------------------------------------------
	«­«ã«Ã«·«åªòìéãÁîÜªËïÎò­/î¢ËÒª¹ªë
------------------------------------------------------*/

void cache_sleep(int flag)
{
/*
	if (num_cache)
	{
		if (flag)
		{
			close(cache_fd);
			cache_fd = -1;
		}
		else
		{
			cache_fd = open(spr_cache_name, O_RDONLY);
		}
	}
*/
}

#endif /* USE_CACHE */
