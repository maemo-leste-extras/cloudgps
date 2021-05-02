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

#include "screen.h"

long last_dbus_milis = 0;

void prevent_screen_dimming() {
	DBusConnection* bus = NULL;
	DBusMessage* msg = NULL;
	DBusError error;

	if (nowMillis - last_dbus_milis < 5000) {
        return;
    }

	dbus_error_init(&error);

	bus = dbus_bus_get(DBUS_BUS_SESSION, &error);

	if (bus == NULL) {
		fprintf(stderr, "bus == NULL\n");
		return;
	}


    last_dbus_milis = nowMillis;

     // create a signal and check for errors
     msg = dbus_message_new_method_call("com.nokia.mce", //dest
             "/com/nokia/mce/request", // path
             "com.nokia.mce.request", // interface
             "req_display_blanking_pause"); // method str
     if (NULL == msg) {
         fprintf(stderr, "Message Null\n");
         exit(1);
     }

     DBusPendingCall* pending;
     // send the message and flush the connection
     if (!dbus_connection_send_with_reply(bus, msg, &pending, 10)) { // -1 is default timeout
         fprintf(stderr, "Out Of Memory!\n");
         exit(1);
     }
     if (NULL == pending) {
         fprintf(stderr, "Pending Call Null\n");
         exit(1);
     }
     dbus_connection_flush(bus);
     // free the message
     dbus_message_unref(msg);

     // block until we receive a reply
     dbus_pending_call_block(pending);

     // get the reply message
     msg = dbus_pending_call_steal_reply(pending);
     if (NULL == msg) {
         fprintf(stderr, "Reply Null\n");
         exit(1);
     }
     // free the pending message handle
     dbus_pending_call_unref(pending);

     // free reply and close connection
     dbus_message_unref(msg);
}
