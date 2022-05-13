#pragma once
/*
 * Copyright 2022, Jefferson Science Associates, LLC.
 * Subject to the terms in the LICENSE file found in the top-level directory.
 *
 *     Authors: Bryan Moffit
 *              moffit@jlab.org                   Jefferson Lab, MS-12B3
 *              Phone: (757) 269-5660             12000 Jefferson Ave.
 *              Fax:   (757) 269-5800             Newport News, VA 23606
 *
 * Description: Header for Helicity Generator module Library
 *
 */

#include <stdint.h>

int32_t heliInit(uint32_t a24_addr, uint16_t init_flag);
int32_t heliStatus(int32_t print_regs);

int32_t heliSetLibraryMode(uint8_t rw_mode_set);

int32_t heliConfigure(uint8_t tsettle_set, uint8_t tstable_set, uint8_t delay_set,
		      uint8_t pattern_set, uint8_t clock_set);
int32_t heliGetSettings(uint8_t *tsettle_set, uint8_t *tstable_set, uint8_t *delay_set,
			uint8_t *pattern_set, uint8_t *clock_set);

int32_t heliSetTSettle(uint8_t tsettle_set);
int32_t heliGetTSettle(uint8_t tsettle_set);

int32_t heliSetTStable(uint8_t tstable_set);
int32_t heliGetTStable(uint8_t tstable_set);

int32_t heliSetDelay(uint8_t delay_set);
int32_t heliGetDelay(uint8_t delay_set);

int32_t heliSetPattern(uint8_t pattern_set);
int32_t heliGetPattern(uint8_t pattern_set);

int32_t heliSetClock(uint8_t clock_set);
int32_t heliGetClock(uint8_t clock_set);
