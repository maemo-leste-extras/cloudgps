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

#include <dbus/dbus.h>

void invokeTaskManager() {
	DBusConnection* bus = NULL;
	DBusMessage* msg = NULL;
	DBusError error;

	dbus_error_init(&error);

	bus = dbus_bus_get(DBUS_BUS_SESSION, &error);

	if (bus == NULL) {
		fprintf(stderr, "bus == NULL\n");
		return;
	}

	// create a signal and check for errors
	msg = dbus_message_new_signal("/com/nokia/hildon_desktop", // path
			"com.nokia.hildon_desktop", // interface
			"exit_app_view"); // method str
	if (NULL == msg) {
		fprintf(stderr, "Message Null\n");
		exit(1);
	}

	// send the message and flush the connection
	if (!dbus_connection_send(bus, msg, NULL)) {
		fprintf(stderr, "Out Of Memory!\n");
		exit(1);
	}

	dbus_connection_flush(bus);
	dbus_message_unref(msg);
}
