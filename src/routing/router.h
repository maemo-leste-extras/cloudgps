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


#ifndef ROUTER_H_
#define ROUTER_H_

#include "../main.h"
#include "../network.h"
#include "../tileengine.h"
#include "../unicode2ascii.h"

#include "google_maps_router.h"

void initRoutingProviders();
void loadRoute(RoutingQuery* query);

int processNewRoutingQuery(RoutingQuery* query);

void processRouting();

extern volatile BackgroundQueryStatus routingStatus;
extern Route route;

#endif /* ROUTER_H_ */
