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


#ifndef GOOGLE_LOCAL_GEOCODER_H_
#define GOOGLE_LOCAL_GEOCODER_H_

#include <stdlib.h>
#include <json-c/json.h>

#include "../main.h"

#define GOOGLE_QUERY_URL "http://maps.googleapis.com/maps/api/geocode/json?address=%s&sensor=true"

char* google_maps_local_prepare_url(char* encodedQuery);
char* google_maps_prepare_url(void* encodedQuery);
void google_maps_parse_response(char* response);

#endif /* GOOGLE_LOCAL_GEOCODER_H_ */
