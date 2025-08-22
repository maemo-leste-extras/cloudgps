/*
 * coordinate_reader.c
 *
 *  Created on: Jun 8, 2011
 *      Author: demon
 */

#include "coordinate_reader.h"

#define KEY_LON "lng"
#define KEY_LAT "lat"

void readCloudmadeWorldCoordinate(json_object * jobj, WorldCoordinate *wc) {
	wc -> latitude = json_object_get_double(json_object_array_get_idx(jobj, 1));
	wc -> longitude = json_object_get_double(json_object_array_get_idx(jobj, 0));
}
