/******************************************************************************

	eeprom.c

	CPS2 EEPROM«¤«ó«¿«Õ«§£­«¹Î¼Þü

******************************************************************************/

#include "cps2.h"


#define SERIAL_BUFFER_LENGTH	40


/******************************************************************************
	«í£­«««ëÜ¨Þü
******************************************************************************/

static int serial_count;
static u8  serial_buffer[SERIAL_BUFFER_LENGTH];
static u8  eeprom_data[EEPROM_SIZE];
static int eeprom_data_bits;
static int eeprom_read_address;
static int eeprom_clock_count;
static int latch;
static int reset_line;
static int clock_line;
static int sending;


/******************************************************************************
	«í£­«««ëÎ¼Þü
******************************************************************************/

/*--------------------------------------------------------
	«³«Þ«ó«É«Þ«Ã«Á«ó«°
--------------------------------------------------------*/

static int EEPROM_command_match(const char *buf, const char *cmd, int len)
{
	if (!cmd || !len) return 0;

	while (len > 0)
	{
		char b = *buf;
		char c = *cmd;

		if (!b || !c) return (b == c);
		if (b != c) return 0;

		buf++;
		cmd++;
		len--;
	}
	return (*cmd == 0);
}


/*--------------------------------------------------------
	EEPROM«Ó«Ã«Èßöª­¢Ãªß
--------------------------------------------------------*/

static void EEPROM_write(int bit)
{
	if (serial_count >= SERIAL_BUFFER_LENGTH - 1) return;

	serial_buffer[serial_count++] = (bit ? '1' : '0');
	serial_buffer[serial_count] = '\0';

	if (serial_count > 6)
	{
		int i, address;
		char *buffer = (char *)serial_buffer;

		if (EEPROM_command_match(buffer, "0110", strlen(buffer) - 6))
		{
			// read
			address = 0;
			for (i = serial_count - 6; i < serial_count; i++)
			{
				address <<= 1;
				if (serial_buffer[i] == '1') address |= 1;
			}
			eeprom_data_bits = (eeprom_data[2 * address] << 8) + eeprom_data[2 * address + 1];
			eeprom_read_address = address;
			eeprom_clock_count = 0;
			sending = 1;
			serial_count = 0;
		}
		else if (EEPROM_command_match(buffer, "0111", strlen(buffer) - 6))
		{
			// elase
			address = 0;
			for (i = serial_count - 6; i < serial_count; i++)
			{
				address <<= 1;
				if (serial_buffer[i] == '1') address |= 1;
			}
			eeprom_data[2 * address + 0] = 0x00;
			eeprom_data[2 * address + 1] = 0x00;
			serial_count = 0;
		}
		else if (serial_count > 22 && EEPROM_command_match(buffer, "0101", strlen(buffer) - 22))
		{
			int data;

			// write
			address = 0;
			for (i = serial_count - 22;i < serial_count - 16; i++)
			{
				address <<= 1;
				if (serial_buffer[i] == '1') address |= 1;
			}
			data = 0;
			for (i = serial_count - 16; i < serial_count; i++)
			{
				data <<= 1;
				if (serial_buffer[i] == '1') data |= 1;
			}
			eeprom_data[2 * address + 0] = data >> 8;
			eeprom_data[2 * address + 1] = data & 0xff;
			serial_count = 0;
		}
	}
}


/*--------------------------------------------------------
	«ê«»«Ã«È
--------------------------------------------------------*/

static void EEPROM_reset(void)
{
	serial_count = 0;
	sending = 0;
}


/******************************************************************************
	«°«í£­«Ð«ëÎ¼Þü
******************************************************************************/

/*--------------------------------------------------------
	ôøÑ¢ûù
--------------------------------------------------------*/

void EEPROM_init(void)
{
	memset(eeprom_data, 0xff, EEPROM_SIZE);
	serial_count = 0;
	latch = 0;
	reset_line = ASSERT_LINE;
	clock_line = ASSERT_LINE;
	eeprom_read_address = 0;
	sending = 0;
}


/*--------------------------------------------------------
	«Ó«Ã«Èßöª­¢Ãªß
--------------------------------------------------------*/

void EEPROM_write_bit(int bit)
{
	latch = bit;
}


/*--------------------------------------------------------
	«Ó«Ã«ÈÔÁªß¢Ãªß
--------------------------------------------------------*/

int EEPROM_read_bit(void)
{
	if (sending)
		return (eeprom_data_bits >> 16) & 1;
	return 1;
}


/*--------------------------------------------------------
	CSàâïÒ
--------------------------------------------------------*/

void EEPROM_set_cs_line(int state)
{
	reset_line = state;

	if (reset_line != CLEAR_LINE)
		EEPROM_reset();
}


/*--------------------------------------------------------
	«¯«í«Ã«¯àâïÒ
--------------------------------------------------------*/

void EEPROM_set_clock_line(int state)
{
	if (state == PULSE_LINE || (clock_line == CLEAR_LINE && state != CLEAR_LINE))
	{
		if (reset_line == CLEAR_LINE)
		{
			if (sending)
			{
				eeprom_data_bits = (eeprom_data_bits << 1) | 1;
				eeprom_clock_count++;
			}
			else
				EEPROM_write(latch);
		}
	}

	clock_line = state;
}


/*--------------------------------------------------------
	«Õ«¡«¤«ëª«ªé«Ç£­«¿ªòÔÁªß¢Ãªß
--------------------------------------------------------*/

void EEPROM_load(const char *path)
{
	FILE *fp;

	if ((fp = fopen(path, "rb")) != NULL)
	{
		fread(eeprom_data, 1, EEPROM_SIZE, fp);
		fclose(fp);
	}
}


/*--------------------------------------------------------
	«Õ«¡«¤«ëªË«Ç£­«¿ªòÜÁðí
--------------------------------------------------------*/

void EEPROM_save(const char *path)
{
	int ret = 0;
	u8 eeprom_cmp[EEPROM_SIZE];
	FILE *fp;

	if ((fp = fopen(path, "rb")) != NULL)
	{
		ret = fread(eeprom_cmp, 1, EEPROM_SIZE, fp);
		fclose(fp);
	}

	if((ret == EEPROM_SIZE) && !memcmp(eeprom_cmp, eeprom_data, EEPROM_SIZE)) return;

	if ((fp = fopen(path, "wb")) != NULL)
	{
		fwrite(eeprom_data, 1, EEPROM_SIZE, fp);
		fclose(fp);
		msg_printf("EEPROM data saved.\n");
	}
}


/*--------------------------------------------------------
	EEPROMªËòÁïÈ«Ç£­«¿ªòßöª­¢Ãªà
--------------------------------------------------------*/

u8 EEPROM_read_data(u32 address)
{
	return eeprom_data[address];
}


/*--------------------------------------------------------
	EEPROMªÎ«Ç£­«¿ªòÔÁªß¢Ãªà
--------------------------------------------------------*/

void EEPROM_write_data(u32 address, u8 data)
{
	eeprom_data[address] = data;
}


/*------------------------------------------------------
	«»£­«Ö/«í£­«É «¹«Æ£­«È
------------------------------------------------------*/

#ifdef SAVE_STATE

STATE_SAVE( eeprom )
{
	state_save_long(&serial_count, 1);
	state_save_long(&eeprom_data_bits, 1);
	state_save_long(&eeprom_read_address, 1);
	state_save_long(&eeprom_clock_count, 1);
	state_save_long(&latch, 1);
	state_save_long(&reset_line, 1);
	state_save_long(&clock_line, 1);
	state_save_long(&sending, 1);
	state_save_byte(serial_buffer, SERIAL_BUFFER_LENGTH);
	state_save_byte(eeprom_data, EEPROM_SIZE);
}

STATE_LOAD( eeprom )
{
	state_load_long(&serial_count, 1);
	state_load_long(&eeprom_data_bits, 1);
	state_load_long(&eeprom_read_address, 1);
	state_load_long(&eeprom_clock_count, 1);
	state_load_long(&latch, 1);
	state_load_long(&reset_line, 1);
	state_load_long(&clock_line, 1);
	state_load_long(&sending, 1);
	state_load_byte(serial_buffer, SERIAL_BUFFER_LENGTH);
	state_load_byte(eeprom_data, EEPROM_SIZE);
}

#endif /* SAVE_STATE */
