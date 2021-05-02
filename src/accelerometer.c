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

#ifdef N900
static int oax = 0;
static int oay = 0;
static int oaz = 0;

static const char *accel_filename = "/sys/class/i2c-adapter/i2c-3/3-001d/coord";

int liqaccel_read(int *ax, int *ay, int *az) {
    return 0;
    FILE *fd;
    int rs;
    fd = fopen(accel_filename, "r");
    if (fd == NULL) {
        fprintf(stderr, "liqaccel, cannot open for reading");
        return -1;
    }
    rs = fscanf((FILE*) fd, "%i %i %i", ax, ay, az);
    fclose(fd);
    if (rs != 3) {
        fprintf(stderr, "liqaccel, cannot read information");
        return -2;
    }
    int bx = *ax;
    int by = *ay;
    int bz = *az;
    if (ocnt > 0) {
        *ax = oax + (bx - oax) * 0.1;
        *ay = oay + (by - oay) * 0.1;
        *az = oaz + (bz - oaz) * 0.1;
    }
    oax = *ax;
    oay = *ay;
    oaz = *az;
    ocnt++;
    return 0; 
}

#else

int liqaccel_read(int *ax, int *ay, int *az) {
    return 0;
}
#endif
