/*
 * joy.c - Joystick support for MS-DOS.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "cmdline.h"
#include "keyboard.h"
#include "joy.h"
#include "joyport.h"
#include "joystick.h"
#include "machine.h"
#include "resources.h"
#include "types.h"

/* ------------------------------------------------------------------------- */

void joystick_close(void)
{
}

/* Initialize joystick support.  */
int joy_arch_init(void)
{
    return 0;
}

void kbd_initialize_numpad_joykeys(int* joykeys)
{
}

/* ------------------------------------------------------------------------- */

int joy_arch_cmdline_options_init(void)
{
    return 0;
}
 
int joy_arch_set_device(int port_idx, int joy_dev)
{
    return 0;
} 

int joy_arch_resources_init(void)
{
    return 0;
}
 