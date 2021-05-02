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


#ifndef GOOGLE_GEOCODER_H_
#define GOOGLE_GEOCODER_H_

#include <stdlib.h>
#include <json/json.h>
#include <json/json_object_private.h>

#include "../main.h"

char* google_prepare_url(char* encodedQuery);
void google_parse_response(char* response);

#endif /* GOOGLE_GEOCODER_H_ */
