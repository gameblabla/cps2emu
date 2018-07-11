/******************************************************************************

	vidhrdw.c

	CPS2 �ӫǫ����߫�죭�����

******************************************************************************/

#ifndef CPS2_VIDEO_HARDWARE_H
#define CPS2_VIDEO_HARDWARE_H

#define FIRST_VISIBLE_LINE	16
#define LAST_VISIBLE_LINE	239

enum
{
	LAYER_SKIP = -1,
	LAYER_OBJECT = 0,
	LAYER_SCROLL1,
	LAYER_SCROLL2,
	LAYER_SCROLL3,
	LAYER_MAX
};

enum
{
	TILE08 = 0,
	TILE16,
	TILE32
};

extern int cps_rotate_screen;
extern int cps_flip_screen;
extern int cps_raster_enable;
extern int cps2_has_mask;
extern u32 video_palette[2048];

extern int scanline1;
extern int scanline2;

extern int cps_scroll2_rows[32][16];

extern void (*cps2_build_palette)(void);

void cps2_scan_object_callback(void);
void cps2_scan_scroll1_callback(void);
void cps2_scan_scroll2_callback(void);
void cps2_scan_scroll3_callback(void);

int cps2_video_init(void);
void cps2_video_exit(void);
void cps2_video_reset(void);
void cps2_screenrefresh(int start, int end);
void cps2_objram_latch(void);

READ16_HANDLER( cps1_output_r );
WRITE16_HANDLER( cps1_output_w );

#ifdef SAVE_STATE
STATE_SAVE( video );
STATE_LOAD( video );
#endif

#endif /* CPS2_VIDEO_HARDWARE_H */
