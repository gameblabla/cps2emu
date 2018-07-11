/******************************************************************************

	sndintrf.c

	«µ«¦«ó«É«¤«ó«¿«Õ«§£­«¹

******************************************************************************/

#include "emumain.h"
#ifdef OSS_SOUND
#include <linux/soundcard.h>
#include <pthread.h>
#include <sys/select.h>
#else
#include <SDL/SDL.h>
#endif

/******************************************************************************
	«µ«¦«ó«É«¤«ó«¿«Õ«§£­«¹Î¼Þü
******************************************************************************/

#ifdef OSS_SOUND
int sound_fd = -1;
static int sound_play = 0;
static int sound_id = -1;
static pthread_t sound_th;

void *sound_thread(void *data)
{
	fd_set fdset;
	struct timeval timeout;

    while(option_sound_enable)
    {
    	FD_ZERO(&fdset);
    	FD_SET(sound_fd, &fdset);
    	timeout.tv_sec = 10;
    	timeout.tv_usec = 0;

		if ( select(sound_fd+1, NULL, &fdset, NULL, &timeout) <= 0 ) {
            option_sound_enable = 0;
        } else if(sound_play) {
            qsound_update();
        } else {
            usleep(10000);
        }
    }

    pthread_exit(0);
}
#endif

/*------------------------------------------------------
	«µ«¦«ó«É«¨«ß«å«ì£­«·«ç«óôøÑ¢ûù
------------------------------------------------------*/
int sound_init(void)
{
	qsound_sh_start();

#ifdef OSS_SOUND
	if (sound_fd >= 0) close(sound_fd);
	sound_fd = open("/dev/dsp", O_WRONLY|O_ASYNC);
	if (sound_fd == -1) {
		printf("Couldn't open /dev/dsp device.\n");
		option_sound_enable = 0;
		return 0;
    } else {
    	int rate, bits, stereo, frag;
    	rate = 44100 >> (2 - option_samplerate);
    	bits = 16;
    	stereo = 1;
    
    	ioctl(sound_fd, SNDCTL_DSP_SPEED,  &rate);
    	ioctl(sound_fd, SNDCTL_DSP_SETFMT, &bits);
    	ioctl(sound_fd, SNDCTL_DSP_STEREO, &stereo);

    	frag = 10 + option_samplerate;
    	frag |= 2 << 16;
    	ioctl(sound_fd, SNDCTL_DSP_SETFRAGMENT, &frag);
    }

    sound_id = pthread_create(&sound_th, NULL, sound_thread, NULL);
    if (sound_id < 0) {
        msg_printf("Sound thread create failed.");
        option_sound_enable = 0;
        return 0;
    }

    sound_play = 1;
#else
	SDL_AudioSpec spec, rec;

	spec.format = AUDIO_S16LSB;
	spec.channels = 2;
	spec.freq = 44100 >> (2 - option_samplerate);
	spec.samples = SOUND_SAMPLES >> (2 - option_samplerate);

	spec.callback = qsound_update;
	spec.userdata = NULL;

	if (SDL_OpenAudio(&spec, &rec) < 0) {
		printf("Unable to open audio: %s.\n", SDL_GetError());
		return 0;
	}

	SDL_PauseAudio(0);
#endif

	printf("Sound device initialized...\n");

	return 1;
}


/*------------------------------------------------------
	«µ«¦«ó«É«¨«ß«å«ì£­«·«ç«óðûÖõ
------------------------------------------------------*/

void sound_exit(void)
{
	qsound_sh_stop();

#ifdef OSS_SOUND
    sound_play = 0;
    option_sound_enable = 0;
    pthread_join(sound_th, NULL);
    close(sound_fd);
#else
	SDL_PauseAudio(1);
#endif
}


/*------------------------------------------------------
	«µ«¦«ó«É«¨«ß«å«ì£­«·«ç«ó«ê«»«Ã«È
------------------------------------------------------*/

void sound_reset(void)
{
#if (EMU_SYSTEM == CPS1)
	//if (machine_sound_type == SOUND_QSOUND)
		qsound_sh_reset();
	//else
	//	YM2151_sh_reset();
#elif (EMU_SYSTEM == CPS2)
	qsound_sh_reset();
#elif (EMU_SYSTEM == MVS)
	YM2610_sh_reset();
#endif

	sound_mute(0);
}


/*------------------------------------------------------
	«µ«¦«ó«É«ß«å£­«È
------------------------------------------------------*/

void sound_mute(int mute)
{
#ifdef OSS_SOUND
    sound_play = mute ? 0 : option_sound_volume;
#else
	if (mute)
    	SDL_PauseAudio(1);
	else
    	SDL_PauseAudio(0);
#endif
}


void sound_volume(int volume)
{
#ifdef OSS_SOUND
    static int last_volume = -1;
    int mixer = open("/dev/mixer", O_RDWR);

    if(volume < 0) volume = 0;
    if(volume > 90) volume = 90;

    if(!option_sound_enable) {
        msg_printf("Volume: Disable\n");
    } else if(last_volume != volume) {
        last_volume = option_sound_volume = volume;

        if(mixer >= 0) {            
            int vol = option_sound_volume | (option_sound_volume << 8);
        	ioctl(mixer, SOUND_MIXER_WRITE_PCM, &vol);
        	close(mixer);
        }

        if(option_sound_volume == 90) {
            msg_printf("Volume: Max\n");
            sound_play = 1;
        } else if(option_sound_volume == 0) {
            msg_printf("Volume: Off\n");
            sound_play = 0;
        } else {
            msg_printf("Volume: %d\n", option_sound_volume);
            sound_play = 1;
        }
    }
#endif
}
