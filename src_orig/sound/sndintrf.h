/******************************************************************************

	sndintrf.c

	«µ«¦«ó«É«¤«ó«¿«Õ«§£­«¹

******************************************************************************/

#ifndef SOUND_INTERFACE_H
#define SOUND_INTERFACE_H

#define MAXOUT		(+32767)
#define MINOUT		(-32768)

#define Limit(val, max, min)			\
{										\
	if (val > max) val = max;			\
	else if (val < min) val = min;		\
}

#ifdef OSS_SOUND
extern int sound_fd;
#endif

enum
{
	SOUND_QSOUND = 0
};

int sound_init(void);
void sound_exit(void);
void sound_reset(void);
void sound_mute(int mute);
void sound_volume(int volume);

#endif /* SOUND_INTERFACE_H */
