#include "cps2.h"

extern u16 *work_frame;

int sprite_count;
SPRITE sprite_info[MAX_SPRITE];
static SPRITE *sprite;

/*------------------------------------------------------------------------
	OBJECTªòÙÚûþ«ê«¹«ÈªËÔôÒÓ
------------------------------------------------------------------------*/

void draw_sprite_start(void)
{
    sprite = sprite_info;
}

int draw_sprite(void)
{
	register u16 *dst;
	register u32 *gfx, *pal, tile;
	register int i;

	while (sprite->type != SPRITE_END) {
        int attr = sprite->attr;
		gfx = (u32 *)&memory_region_gfx1[sprite->src];
		dst = work_frame + sprite->dst;
		pal = &video_palette[sprite->color << 4];

		switch(sprite->type) {
#define DRAW_8 tile = *gfx++;DRAW_1;DRAW_1;DRAW_1;DRAW_1;DRAW_1;DRAW_1;DRAW_1;DRAW_1
#define DRAW_TILE for(i=(TILE_SIZE - 1); i; --i) { DRAW_LINE;DRAW_NEXT; } DRAW_LINE;
            case SPRITE_8:
                #define DRAW_LINE ++gfx;DRAW_8;
                #define TILE_SIZE 8
                if (attr & 0x20) {
                    #define DRAW_1 if(tile & 0x0f) *dst = pal[tile & 0x0f]; --dst; tile >>= 4;
            		if (attr & 0x40) {
                		dst += BUF_WIDTH * (TILE_SIZE - 1) + TILE_SIZE - 1;
                        #define DRAW_NEXT dst -= BUF_WIDTH - TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    } else {
                		dst += TILE_SIZE - 1;
                        #define DRAW_NEXT dst += BUF_WIDTH + TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    }
                    #undef DRAW_1
                } else {
                    #define DRAW_1 if(tile & 0x0f) *dst = pal[tile & 0x0f]; ++dst; tile >>= 4;
            		if (attr & 0x40) {
                		dst += BUF_WIDTH * (TILE_SIZE - 1);
                        #define DRAW_NEXT dst -= BUF_WIDTH + TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    } else {
                        #define DRAW_NEXT dst += BUF_WIDTH - TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    }
                    #undef DRAW_1
                }
                #undef TILE_SIZE
                #undef DRAW_LINE
                break;
            case SPRITE_16:
                #define DRAW_LINE DRAW_8;DRAW_8
                #define TILE_SIZE 16
                if (attr & 0x20) {
                    #define DRAW_1 if(tile & 0x0f) *dst = pal[tile & 0x0f]; --dst; tile >>= 4;
            		if (attr & 0x40) {
                		dst += BUF_WIDTH * (TILE_SIZE - 1) + TILE_SIZE - 1;
                        #define DRAW_NEXT dst -= BUF_WIDTH - TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    } else {
                		dst += TILE_SIZE - 1;
                        #define DRAW_NEXT dst += BUF_WIDTH + TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    }
                    #undef DRAW_1
                } else {
                    #define DRAW_1 if(tile & 0x0f) *dst = pal[tile & 0x0f]; ++dst; tile >>= 4;
            		if (attr & 0x40) {
                		dst += BUF_WIDTH * (TILE_SIZE - 1);
                        #define DRAW_NEXT dst -= BUF_WIDTH + TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    } else {
                        #define DRAW_NEXT dst += BUF_WIDTH - TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    }
                    #undef DRAW_1
                }
                #undef TILE_SIZE
                #undef DRAW_LINE
                break;
            case SPRITE_32:
                #define DRAW_LINE DRAW_8;DRAW_8;DRAW_8;DRAW_8
                #define TILE_SIZE 32
                if (attr & 0x20) {
                    #define DRAW_1 if(tile & 0x0f) *dst = pal[tile & 0x0f]; --dst; tile >>= 4;
            		if (attr & 0x40) {
                		dst += BUF_WIDTH * (TILE_SIZE - 1) + TILE_SIZE - 1;
                        #define DRAW_NEXT dst -= BUF_WIDTH - TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    } else {
                		dst += TILE_SIZE - 1;
                        #define DRAW_NEXT dst += BUF_WIDTH + TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    }
                    #undef DRAW_1
                } else {
                    #define DRAW_1 if(tile & 0x0f) *dst = pal[tile & 0x0f]; ++dst; tile >>= 4;
            		if (attr & 0x40) {
                		dst += BUF_WIDTH * (TILE_SIZE - 1);
                        #define DRAW_NEXT dst -= BUF_WIDTH + TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    } else {
                        #define DRAW_NEXT dst += BUF_WIDTH - TILE_SIZE;
               			DRAW_TILE;
               			#undef DRAW_NEXT
                    }
                    #undef DRAW_1
                }
                #undef TILE_SIZE
                #undef DRAW_LINE
                break;
            case SPRITE_16_ROWS:
                #define DRAW_LINE DRAW_8;DRAW_8
                #define TILE_SIZE 16
                if (attr & 0x20) {
                    #define DRAW_1 if(tile & 0x0f) *dst = pal[tile & 0x0f]; --dst; tile >>= 4;
            		if (attr & 0x40) {
						int x = sprite->x;
						int rows = sprite->rows;
						dst += BUF_WIDTH * (TILE_SIZE - 1) + TILE_SIZE - 1;
               			for(i=TILE_SIZE; i >= 0; --i) {
							int dx = x - cps_scroll2_rows[rows][i];
							if((dx > 48) && (dx < 448)) {
								dst += dx;
								DRAW_LINE;
								dst -= BUF_WIDTH - TILE_SIZE + dx;
							} else {
								gfx += 2;
								dst -= BUF_WIDTH;
							}
						}
						break;
                    } else {
						int x = sprite->x;
						int rows = sprite->rows;
                		dst += TILE_SIZE - 1;
               			for(i=0; i < TILE_SIZE; ++i) {
							int dx = x - cps_scroll2_rows[rows][i];
							if((dx > 48) && (dx < 448)) {
								dst += dx;
								DRAW_LINE;
								dst += BUF_WIDTH + TILE_SIZE - dx;
							} else {
								gfx += 2;
								dst += BUF_WIDTH;
							}
						}
                    }
                    #undef DRAW_1
                } else {
                    #define DRAW_1 if(tile & 0x0f) *dst = pal[tile & 0x0f]; ++dst; tile >>= 4;
            		if (attr & 0x40) {
						int x = sprite->x;
						int rows = sprite->rows;
						dst += BUF_WIDTH * (TILE_SIZE - 1);
               			for(i=TILE_SIZE; i >= 0; --i) {
							int dx = x - cps_scroll2_rows[rows][i];
							if((dx > 48) && (dx < 448)) {
								dst += dx;
								DRAW_LINE;
								dst -= BUF_WIDTH + TILE_SIZE + dx;
							} else {
								gfx += 2;
								dst -= BUF_WIDTH;
							}
						}
						break;
                    } else {
						int x = sprite->x;
						int rows = sprite->rows;
               			for(i=0; i < TILE_SIZE; ++i) {
							int dx = x - cps_scroll2_rows[rows][i];
							if((dx > 48) && (dx < 448)) {
								dst += dx;
								DRAW_LINE;
								dst += BUF_WIDTH - TILE_SIZE - dx;
							} else {
								gfx += 2;
								dst += BUF_WIDTH;
							}
						}
                    }
                    #undef DRAW_1
                }
                #undef TILE_SIZE
                #undef DRAW_LINE
                break;
#undef DRAW_8
#undef DRAW_TILE
            case SPRITE_PART:
                i = sprite->attr;
                ++sprite;
                return i;
        }

        ++sprite;
	}

	return 0;
}
