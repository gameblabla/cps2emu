/******************************************************************************

	sprite.c

	CPS2 «¹«×«é«¤«È«Þ«Í£­«¸«ã

******************************************************************************/

#ifndef CPS2_SPRITE_H
#define CPS2_SPRITE_H

void blit_reset(void);
void blit_partial_start(int start, int end);
void blit_finish(void);

void blit_draw_object(int x, int y, int z, u32 code, u32 attr);
void blit_draw_scroll1(int x, int y, u32 code, u32 attr);
void blit_set_clip_scroll2(int min_y, int max_y);
void blit_draw_scroll2(int x, int y, u32 code, u32 attr, s16 rows);
void blit_draw_scroll3(int x, int y, u32 code, u32 attr);

#endif /* CPS2_SPRITE_H */
