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


#ifndef CLOUDMADE_ROUTER_H_
#define CLOUDMADE_ROUTER_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <json/json.h>
#include <json/json_object_private.h>

#include "../main.h"
#include "router.h"

char* cloudmadeRouterPrepareUrl(void* queryPtr);
void cloudmadeRouterParseResponse(char* response);

#endif /* CLOUDMADE_ROUTER_H_ */
