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
#include <hal/libhal.h>

LibHalContext *libHalCtx;
char* udi;
DBusError dbusError;

void refreshBattery() {
    batteryPercentage = libhal_device_get_property_int(libHalCtx, udi, "battery.charge_level.percentage", &dbusError);
}

void initBattery() {
    DBusConnection *conn;
    dbus_error_init(&dbusError);
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dbusError);
    if (!conn) {
        fprintf(stderr, "unable to init dbus: %s\n", dbusError.message);
        dbus_error_free(&dbusError);
        return;
    }
    libHalCtx = libhal_ctx_new();
    if (!libhal_ctx_set_dbus_connection(libHalCtx, conn)) {
        fprintf(stderr, "unable to init HAL context with DBus\n");
        return;
    }

    if (!libhal_ctx_init(libHalCtx, &dbusError)) {
        fprintf(stderr, "unable to init HAL context: %s\n", dbusError.message);
        dbus_error_free(&dbusError);
    }
    int num_devices;
    char **devices = libhal_find_device_by_capability(libHalCtx, "battery", &num_devices, &dbusError);
    if (devices) {
        if (num_devices > 1) {
            fprintf(stderr, "%d batteries found", num_devices);
        }
        udi = devices[0];
    } else {
        fprintf(stderr, "unable to find devices by battery capability: %s\n", dbusError.message);
        dbus_error_free(&dbusError);
    }
    refreshBattery();
}

