/******************************************************************************

	loadrom.c

	ROM���ᣭ���ի��������μ��

******************************************************************************/

#include "emumain.h"
#include <unistd.h>


/******************************************************************************
	������ܨ��
******************************************************************************/

static int rom_fd;


/******************************************************************************
	�����Ы�μ��
******************************************************************************/

/*--------------------------------------------------------
	ZIP�ի����몫��ի�������������Ҫ�
--------------------------------------------------------*/

int file_open(const char *fname1, const char *fname2, const u32 crc, char *fname)
{
	int found = 0;
	struct zip_find_t file;
	char path[MAX_PATH];

	sprintf(path, "%s/%s.zip", game_dir, fname1);

	if (zip_open(path) != -1)
	{
		if (zip_findfirst(&file))
		{
			if (file.crc32 == crc)
			{
				found = 1;
			}
			else
			{
				while (zip_findnext(&file))
				{
					if (file.crc32 == crc)
					{
						found = 1;
						break;
					}
				}
			}
		}
		if (!found) zip_close();
	}

	if (!found && fname2 != NULL)
	{
		sprintf(path, "%s/%s.zip", game_dir, fname2);

		if (zip_open(path) != -1)
		{
			if (zip_findfirst(&file))
			{
				if (file.crc32 == crc)
				{
					found = 1;
				}
				else
				{
					while (zip_findnext(&file))
					{
						if (file.crc32 == crc)
						{
							found = 1;
							break;
						}
					}
				}
			}

			if (!found) zip_close();
		}
	}

	if (found)
	{
		if (fname) strcpy(fname, file.name);
		rom_fd = zopen(file.name);
		return rom_fd;
	}

	return -1;
}


/*--------------------------------------------------------
	�ի�������ͪ���
--------------------------------------------------------*/

void file_close(void)
{
	if (rom_fd != -1)
	{
		zclose(rom_fd);
		zip_close();
		rom_fd = -1;
	}
}


/*--------------------------------------------------------
	�ի����몫����ҫЫ������ߢê�
--------------------------------------------------------*/

int file_read(void *buf, size_t length)
{
	if (rom_fd != -1)
		return zread(rom_fd, buf, length);
	return -1;
}


/*--------------------------------------------------------
	�ի����몫��1������ߢê�
--------------------------------------------------------*/

int file_getc(void)
{
	if (rom_fd != -1)
		return zgetc(rom_fd);
	return -1;
}


/*--------------------------------------------------------
	����ë���ի�������Ҫ�
--------------------------------------------------------*/

#if USE_CACHE
int cachefile_open(void)
{
	char path[MAX_PATH];
	int cache_fd;

	sprintf(path, "%s/%s.cache", cache_dir, game_name);
	if ((cache_fd = open(path, O_RDONLY | O_BINARY)) >= 0)
		return cache_fd;

	sprintf(path, "%s/%s.cache", game_dir, game_name);
	if ((cache_fd = open(path, O_RDONLY | O_BINARY)) >= 0)
		return cache_fd;

	if (!cache_parent_name[0])
		return -1;

	sprintf(path, "%s/%s.cache", cache_dir, cache_parent_name);
	if ((cache_fd = open(path, O_RDONLY | O_BINARY)) >= 0)
		return cache_fd;

	sprintf(path, "%s/%s.cache", game_dir, cache_parent_name);
	if ((cache_fd = open(path, O_RDONLY | O_BINARY)) >= 0)
		return cache_fd;

	return -1;
}
#endif


/*--------------------------------------------------------
	ROM����ɪ���
--------------------------------------------------------*/

int rom_load(struct rom_t *rom, u8 *mem, int idx, int max)
{
	u32 offset, length;

_continue:
	offset = rom[idx].offset;

	if (rom[idx].skip == 0)
	{
		file_read(&mem[offset], rom[idx].length);

		if (rom[idx].type == ROM_WORDSWAP)
			swab(&mem[offset], &mem[offset], rom[idx].length);
	}
	else
	{
		int c;
		int skip = rom[idx].skip + rom[idx].group;

		length = 0;

		if (rom[idx].group == 1)
		{
			if (rom[idx].type == ROM_WORDSWAP)
				offset ^= 1;

			while (length < rom[idx].length)
			{
				if ((c = file_getc()) == EOF) break;
				mem[offset] = c;
				offset += skip;
				length++;
			}
		}
		else
		{
			while (length < rom[idx].length)
			{
				if ((c = file_getc()) == EOF) break;
				mem[offset + 0] = c;
				if ((c = file_getc()) == EOF) break;
				mem[offset + 1] = c;
				offset += skip;
				length += 2;
			}
		}
	}

	if (++idx != max)
	{
		if (rom[idx].type == ROM_CONTINUE)
		{
			goto _continue;
		}
	}

	return idx;
}
