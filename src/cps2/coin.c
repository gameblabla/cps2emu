/******************************************************************************

	coin.c

	«³«¤«ó«««¦«ó«¿

******************************************************************************/

#include "emumain.h"


/******************************************************************************
	«í£­«««ëÜ¨Þü
******************************************************************************/

#define COIN_COUNTERS	4

static u32 coins[COIN_COUNTERS];
static u32 lastcoin[COIN_COUNTERS];
static u32 coinlockedout[COIN_COUNTERS];


/******************************************************************************
	«°«í£­«Ð«ëÎ¼Þü
******************************************************************************/

/*--------------------------------------------------------
	«³«¤«ó«««¦«ó«¿«ê«»«Ã«È
--------------------------------------------------------*/

void coin_counter_reset(void)
{
	memset(coins, 0, sizeof(coins));
	memset(lastcoin, 0, sizeof(lastcoin));
	memset(coinlockedout, 0, sizeof(coinlockedout));
}


/*--------------------------------------------------------
	«³«¤«ó«««¦«ó«¿ËÖãæ
--------------------------------------------------------*/

void coin_counter_w(int num, int on)
{
	if (num >= COIN_COUNTERS) return;

	if (on && (lastcoin[num] == 0))
	{
		coins[num]++;
	}
	lastcoin[num] = on;
}


/*--------------------------------------------------------
	«³«¤«ó«««¦«ó«¿«í«Ã«¯
--------------------------------------------------------*/

void coin_lockout_w(int num, int on)
{
	if (num >= COIN_COUNTERS) return;

	coinlockedout[num] = on;
}


/*--------------------------------------------------------
	«»£­«Ö/«í£­«É «¹«Æ£­«È
--------------------------------------------------------*/

#ifdef SAVE_STATE

STATE_SAVE( coin )
{
	state_save_long(coins, COIN_COUNTERS);
	state_save_long(lastcoin, COIN_COUNTERS);
	state_save_long(coinlockedout, COIN_COUNTERS);
}

STATE_LOAD( coin )
{
	state_load_long(coins, COIN_COUNTERS);
	state_load_long(lastcoin, COIN_COUNTERS);
	state_load_long(coinlockedout, COIN_COUNTERS);
}

#endif /* SAVE_STATE */
