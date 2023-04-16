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


#ifdef N900
#include <glib-2.0/glib.h>
#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>

void onError(LocationGPSDControl *control, LocationGPSDControlError error, gpointer data) {
    fprintf(stderr, "onError\n");
}
void changed(LocationGPSDevice *device, gpointer userdata) {
}
#endif

int glibMainLoopThread(void *unused) {
	GMainLoop *loop;
	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	return 0;
}

void initLocation() {
#ifdef N900
	control = location_gpsd_control_get_default();
	if (control) {
		g_object_set(G_OBJECT(control), "preferred-method", LOCATION_METHOD_USER_SELECTED, NULL);
		g_object_set(G_OBJECT(control), "preferred-interval", LOCATION_INTERVAL_1S, NULL);

		location_gpsd_control_start(control);
	} else {
		fprintf(stderr, "failed to get default gpsd control\n");
	}
	device = (LocationGPSDevice*) g_object_new(LOCATION_TYPE_GPS_DEVICE, NULL);

	if (device) {
		g_signal_connect(control, "error-verbose", G_CALLBACK(onError), 0);
		g_signal_connect(device, "changed", G_CALLBACK(changed), 0);
	} else {
		fprintf(stderr, "failed to get GPS device\n");
	}
#endif
	SDL_CreateThread(glibMainLoopThread, NULL);
}

void shutdownLocation() {
#ifdef N900
	if (device) {
		g_object_unref(device);
	}
	if (control) {
		location_gpsd_control_stop(control);
	}
#endif
}
