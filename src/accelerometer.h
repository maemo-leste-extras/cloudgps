/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Damian Waradzyn
 */

/* Accelerometer support taken from http://wiki.maemo.org/Accelerometers */

static int ocnt = 0;
static int oax = 0;
static int oay = 0;
static int oaz = 0;

static const char *accel_filename = "/sys/class/i2c-adapter/i2c-3/3-001d/coord";

int liqaccel_read(int *ax, int *ay, int *az);
