/*****************************************************************************

	qsound.c

	CAPCOM QSound ���߫�죭�� (CPS1/CPS2)

******************************************************************************/

#ifndef QSOUND_H
#define QSOUND_H

void qsound_sh_start(void);
void qsound_sh_stop(void);
void qsound_sh_reset(void);
void qsound_set_samplerate(void);
#ifdef OSS_SOUND
void qsound_update(void);
#else
void qsound_update(void *data, u8 *buffer, int length);
#endif

WRITE8_HANDLER( qsound_data_h_w );
WRITE8_HANDLER( qsound_data_l_w );
WRITE8_HANDLER( qsound_cmd_w );
READ8_HANDLER( qsound_status_r );

#ifdef SAVE_STATE
STATE_SAVE( qsound );
STATE_LOAD( qsound );
#endif

#endif /* QSOUND_H */
