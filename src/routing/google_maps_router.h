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


#ifndef GOOGLE_MAPS_ROUTER_H_
#define GOOGLE_MAPS_ROUTER_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <json-c/json.h>
#include <json-c/json_object_private.h>

#include "../main.h"
#include "router.h"

char* googleMapsRouterPrepareUrl(RoutingQuery* encodedQuery);
void googleMapsRouterParseResponse(char* response);

#endif /* GOOGLE_MAPS_ROUTER_H_ */
