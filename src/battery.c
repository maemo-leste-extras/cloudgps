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

extern gdouble batteryPercentage;
extern UpClient *battery_client;
extern UpDevice *battery;

void cleanupBattery() {
    if (battery)
        g_object_unref(battery);

    if (battery_client)
        g_object_unref(battery_client);
}

void refreshBattery() {
    if (battery)
        g_object_get(battery, "percentage", &batteryPercentage, NULL);
    else
        batteryPercentage = 42;
}

void initBattery() {
    battery_client = up_client_new();
    if (!battery_client) {
        fprintf(stderr, "could not connect to UPower for battery data");
        return;
    }

    /* Use UPower's DisplayDevice for simplicity */
    battery = up_client_get_display_device(battery_client);
    if (!battery) {
        fprintf(stderr, "could not get battery data from UPower");
        return;
    }

    refreshBattery();
}

