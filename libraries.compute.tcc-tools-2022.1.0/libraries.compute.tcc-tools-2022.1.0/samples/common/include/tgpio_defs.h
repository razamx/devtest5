/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 * PTP 1588 clock support - user space interface
 *
 * Copyright (C) 2010 OMICRON electronics GmbH
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _TCC_TGPIO_DEFS_H
#define _TCC_TGPIO_DEFS_H

#include <linux/ptp_clock.h>

/*
 * Workaround for building on host and target
 */
#ifndef PTP_EVENT_COUNT_TSTAMP2
struct ptp_event_count_tstamp
{
    struct ptp_clock_time device_time;
    unsigned long event_count;
    unsigned int index;
    unsigned int flags;
    unsigned int rsv[4]; /* Reserved for future use. */
};

/*
 * Updated versions of the ioctls
 */
#define PTP_EVENT_COUNT_TSTAMP2 _IOWR(PTP_CLK_MAGIC, 19, struct ptp_event_count_tstamp)

/*
 * Bits of the ptp_extts_request.flags field:
 */
#define PTP_EVENT_COUNTER_MODE (1 << 4)

/*
 * Bits of the ptp_perout_request.flags field:
 */
#define PTP_PEROUT_FREQ_ADJ (1 << 1)

#endif

#endif
