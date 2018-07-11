/******************************************************************************

	state.c
	
	State Save/Load

******************************************************************************/

#include "emumain.h"

#ifdef SAVE_STATE


/******************************************************************************
	Header string
******************************************************************************/

#ifdef GP2X
static const char *current_version_str = "CPS2XSV0";
#else
static const char *current_version_str = "CPS2EMSV";
#endif

/******************************************************************************
	State save/load function
******************************************************************************/

/*------------------------------------------------------
	State save
------------------------------------------------------*/

int state_save(int slot)
{
	FILE *fp = NULL;
	char path[MAX_PATH];

	sprintf(path, "%sstate", launchDir);
	mkdir(path, 0755);

	sprintf(path, "%sstate/%s.sv%d", launchDir, game_name, slot);
	remove(path);

	if ((fp = fopen(path, "wb")) != NULL)
	{
		// Header (8 bytes)
		state_save_byte(current_version_str, 8);

		// State save
		state_save_memory(fp);
		state_save_m68000(fp);
		state_save_z80(fp);
		state_save_input(fp);
		state_save_timer(fp);
		state_save_driver(fp);
		state_save_video(fp);
		state_save_coin(fp);
		state_save_qsound(fp);
		state_save_eeprom(fp);

		fclose(fp);
		sync();

    	msg_printf("Save: \"%s.sv%d\"\n", game_name, slot);
		return 1;
	}

	msg_printf("Save: Couldn't open \"%s.sv%d\"\n", game_name, slot);

	return 0;
}


/*------------------------------------------------------
	State load
------------------------------------------------------*/

int state_load(int slot)
{
	FILE *fp;
	char path[MAX_PATH];
	char version_str[8];

	sprintf(path, "%sstate/%s.sv%d", launchDir, game_name, slot);

	if ((fp = fopen(path, "rb")) != NULL)
	{
		// Header
		state_load_byte(version_str, 8);
		if(strncmp(current_version_str, version_str, 8) == 0) {
			// Disable loading of hiscore if state save is loaded.
			option_hiscore = 0;

    		// State load
    		state_load_memory(fp);
    		state_load_m68000(fp);
    		state_load_z80(fp);
    		state_load_input(fp);
    		state_load_timer(fp);
    		state_load_driver(fp);
    		state_load_video(fp);
    		state_load_coin(fp);
    		state_load_qsound(fp);
    		state_load_eeprom(fp);

        	msg_printf("Load: \"%s.sv%d\"\n", game_name, slot);
        } else {
        	msg_printf("Load: \"%s.sv%d\" is Broken.\n", game_name, slot);
        }

		fclose(fp);
		return 1;
	}

	msg_printf("Load: Couldn't open \"%s.sv%d\"\n", game_name, slot);

	return 0;
}

#endif /* SAVE_STATE */
