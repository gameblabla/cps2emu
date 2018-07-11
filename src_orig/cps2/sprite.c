/******************************************************************************

	sprite.c

	CPS2 «¹«×«é«¤«È«Þ«Í£­«¸«ã

******************************************************************************/

#include "cps2.h"
#include <SDL/SDL.h>

/******************************************************************************
	ïÒÞü/«Þ«¯«íÔõ
******************************************************************************/

extern u16 *work_frame;
extern SPRITE sprite_info[MAX_SPRITE];
extern int sprite_count;

/******************************************************************************
	«í£­«««ëÜ¨Þü/Ï°ðãô÷
******************************************************************************/

static int clip_min_y;
static int clip_max_y;
static int spr8_min_y;
static int spr16_min_y;
static int spr32_min_y;

/*------------------------------------------------------------------------
	«¹«×«é«¤«Èô¥×âªÎ«ê«»«Ã«È
------------------------------------------------------------------------*/

void blit_reset(void)
{
	clip_min_y  = FIRST_VISIBLE_LINE + 16;
	clip_max_y  = LAST_VISIBLE_LINE + 16;
	spr8_min_y  = clip_min_y - 8;
	spr16_min_y = clip_min_y - 16;
	spr32_min_y = clip_min_y - 32;
}


/*------------------------------------------------------------------------
	ûþØüªÎÝ»ÝÂËÖãæËÒã·
------------------------------------------------------------------------*/

void blit_partial_start(int start, int end)
{
	clip_min_y  = start + 16;
	clip_max_y  = end + 17;
	spr8_min_y  = clip_min_y - 8;
	spr16_min_y = clip_min_y - 16;
	spr32_min_y = clip_min_y - 32;
}


/*------------------------------------------------------------------------
	«¹«×«é«¤«ÈÙÚûþðûÖõ
------------------------------------------------------------------------*/

void blit_partial_end(void)
{
    sprite_info[sprite_count].type = SPRITE_PART;
    sprite_info[sprite_count].src = 0;
    sprite_info[sprite_count].dst = 0;
    sprite_info[sprite_count].color = 0;
    sprite_info[sprite_count].attr = clip_max_y - 17;
	++sprite_count;
}


/*------------------------------------------------------------------------
	«¹«×«é«¤«ÈÙÚûþðûÖõ
------------------------------------------------------------------------*/

void blit_finish(void)
{
    sprite_info[sprite_count].type = SPRITE_END;
    sprite_info[sprite_count].src = 0;
    sprite_info[sprite_count].dst = 0;
    sprite_info[sprite_count].color = 0;
    sprite_info[sprite_count].attr = 0;
	sprite_count = 0;
}


/*------------------------------------------------------------------------
	OBJECTªòÙÚûþ«ê«¹«ÈªËÔôÒÓ
------------------------------------------------------------------------*/

void blit_draw_object(int x, int y, int z, u32 code, u32 attr)
{
    y+=16;
	if ((x > 48 && x < 448) && (y > spr16_min_y && y < clip_max_y))
	{
        sprite_info[sprite_count].type = SPRITE_16;
		sprite_info[sprite_count].src = (*read_cache)(code << 7);
		sprite_info[sprite_count].dst = x + BUF_WIDTH * y;
		sprite_info[sprite_count].color = attr & 0x1f;
		sprite_info[sprite_count].attr = attr & 0x60;
		++sprite_count;
	}
}


/*------------------------------------------------------------------------
	SCROLL1ªòÙÚûþ«ê«¹«ÈªËÔôÒÓ
------------------------------------------------------------------------*/

void blit_draw_scroll1(int x, int y, u32 code, u32 attr)
{
    y+=16;
    if ((x > 56 && x < 448) && (y > spr8_min_y && y < clip_max_y))
    {
        sprite_info[sprite_count].type = SPRITE_8;
    	sprite_info[sprite_count].src = (*read_cache)(code << 6);
    	sprite_info[sprite_count].dst = x + BUF_WIDTH * y;
    	sprite_info[sprite_count].color = (attr & 0x1f) + 32;
    	sprite_info[sprite_count].attr = attr & 0x60;
    	++sprite_count;
    }
}


/*------------------------------------------------------------------------
	SCROLL2ªòÙÚûþ«ê«¹«ÈªËÔôÒÓ
------------------------------------------------------------------------*/

void blit_draw_scroll2(int x, int y, u32 code, u32 attr, s16 rows)
{
    y+=16;
	if (y > spr16_min_y && y < clip_max_y)
	{
        sprite_info[sprite_count].type = SPRITE_16;
		sprite_info[sprite_count].src = (*read_cache)(code << 7);
		sprite_info[sprite_count].color = (attr & 0x1f) + 64;
		sprite_info[sprite_count].attr = attr & 0x60;
		if(rows != -1) {
            sprite_info[sprite_count].type = SPRITE_16_ROWS;
            sprite_info[sprite_count].rows = rows;
			sprite_info[sprite_count].dst = BUF_WIDTH * y;
            sprite_info[sprite_count].x = x;
    		++sprite_count;
        } else if((x > 48 && x < 448)) {
		sprite_info[sprite_count].dst = x + BUF_WIDTH * y;
    		++sprite_count;
        }
	}
}


/*------------------------------------------------------------------------
	SCROLL3ªòÙÚûþ«ê«¹«ÈªËÔôÒÓ
------------------------------------------------------------------------*/

void blit_draw_scroll3(int x, int y, u32 code, u32 attr)
{
    y+=16;
    if ((x > 32 && x < 448) && (y > spr32_min_y && y < clip_max_y))
    {
        sprite_info[sprite_count].type = SPRITE_32;
    	sprite_info[sprite_count].src = (*read_cache)(code << 9);
    	sprite_info[sprite_count].dst = x + BUF_WIDTH * y;
    	sprite_info[sprite_count].color = (attr & 0x1f) + 96;
    	sprite_info[sprite_count].attr = attr & 0x60;
    	++sprite_count;
    }
}
