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

#include "router.h"

#define ROUTING_TMP_FILENAME "/home/user/.cloudgps/cloudgps-routing-tmp.js"

volatile BackgroundQueryStatus routingStatus = NO_QUERY;

Route route;
RoutingQuery lastSuccessfullQuery;
GList* routingProviders = NULL;

void initRoutingProviders() {
	BackgroundQueryProvider* p;
	//p = calloc(1, sizeof(BackgroundQueryProvider));
	//p -> name = "CloudMade";
	//p -> parseResponse = &cloudmadeRouterParseResponse;
	//p -> prepareUrl = (prepare_url_t)&cloudmadeRouterPrepareUrl;
	//routingProviders = g_list_append(routingProviders, p);

	p = calloc(1, sizeof(BackgroundQueryProvider));
	p -> name = "Google";
	p -> parseResponse = &googleMapsRouterParseResponse;
	p -> prepareUrl = (prepare_url_t)&googleMapsRouterPrepareUrl;
	routingProviders = g_list_append(routingProviders, p);
}

int queryEqualsLastSuccessfull(RoutingQuery *query) {
	double startLatDiff = query -> start.latitude - lastSuccessfullQuery.start.latitude;
	double startLonDiff = query -> start.longitude - lastSuccessfullQuery.start.longitude;
	double endLatDiff = query -> end.latitude - lastSuccessfullQuery.end.latitude;
	double endLonDiff = query -> end.latitude - lastSuccessfullQuery.end.latitude;
	// TODO waypoints


	double threshold = 0.00001;

	if (fabs(startLatDiff) < threshold && fabs(startLonDiff) < threshold && fabs(endLatDiff) < threshold && fabs(endLonDiff) < threshold) {
		return TRUE;
	}
	return FALSE;
}

void loadRoute(RoutingQuery* query) {
	char *response;
	size_t length;

	if (g_file_get_contents(ROUTING_TMP_FILENAME, &response, &length, NULL)) {
		options.routingProvider -> parseResponse(response);
		g_free(response);
		if (routingStatus == RESULTS_READY) {
			if (query != NULL) {
				lastSuccessfullQuery.start = query -> start;
				lastSuccessfullQuery.end = query -> end;
				lastSuccessfullQuery.waypoints = query -> waypoints;
			} else {
				route.query = calloc(1, sizeof(RoutingQuery));
				RouteDirection firstDirection = g_array_index(route.directions, RouteDirection, 0);
				route.query -> start = g_array_index(firstDirection.polyLine, WorldCoordinate, 0);

				RouteDirection lastDirection = g_array_index(route.directions, RouteDirection, route.directions -> len - 1);
				route.query -> end = g_array_index(lastDirection.polyLine, WorldCoordinate, lastDirection.polyLine -> len - 1);
			}
		}
	} else {
		if (query != NULL) {
			routingStatus = PARSE_ERROR;
		}
		fprintf(stderr, "Unable to read routing results from %s\n", ROUTING_TMP_FILENAME);
	}
}

// Download and prepare search results.
// TODO copy pattern from geocoder.c - should be in backgroundQuery.c
int routingBgThread(void* queryPtr) {
	char *url;
	RoutingQuery* query = (RoutingQuery*) queryPtr;

	if (queryEqualsLastSuccessfull(query)) {
		routingStatus = QUERY_THE_SAME;
		return 0;
	}

	route.query = query;

	url = options.routingProvider -> prepareUrl(query);

	fprintf(stderr, "Search URL: %s\n", url);

	if (url == NULL) {
		fprintf(stderr, "Search URL is NULL\n");
		routingStatus = DOWNLOAD_ERROR;
		return 0;
	}

	if (downloadAndSave(url, ROUTING_TMP_FILENAME)) {
		loadRoute(query);
	} else {
		routingStatus = DOWNLOAD_ERROR;
	}
	return 0;
}

long lastRoutingQueryMilis = 0;
int processNewRoutingQuery(RoutingQuery* query) {
	if (routingStatus != NO_QUERY) {
		sprintf(strbuf, "Routing: thread busy...");
		addConsoleLine(strbuf, 1, 1, 0);
		return FALSE;
	} else {
		if (nowMillis - lastRoutingQueryMilis < 500) {
			return FALSE;
		}
		lastRoutingQueryMilis = nowMillis;
		routingStatus = QUERY_IN_PROGRESS;
		SDL_CreateThread(routingBgThread, (void*) query);
		return TRUE;
	}
}

void createQueryFromMarkers() {
	RoutingQuery* query = calloc(1, sizeof(RoutingQuery));
	query -> start = canvas.route.query -> start;
	query -> end = canvas.route.query -> end;
	query -> waypoints = canvas.route.query -> waypoints;
	if (processNewRoutingQuery(query)) {
		canvas.routeMarkersChanged = FALSE;
	}
}

void processRouting() {
	switch (routingStatus) {
	case QUERY_IN_PROGRESS:
		return;
	case NO_QUERY:
		break;
	case DOWNLOAD_ERROR:
		sprintf(strbuf, "Route download error");
		addConsoleLine(strbuf, 1, 0, 0);
		break;
	case PARSE_ERROR:
		sprintf(strbuf, "Route parse error");
		addConsoleLine(strbuf, 1, 0, 0);
		break;
	case ZERO_RESULTS:
		sprintf(strbuf, "Route: no results, status %s", route.statusMessage);
		addConsoleLine(strbuf, 0, 1, 0);
		break;
	case RESULTS_READY:
		sprintf(strbuf, "Route length %d m", route.lengthMeters);
		addConsoleLine(strbuf, 0, 1, 0);
		tileEngineProcessNewRoute(&route);
		break;
	case QUERY_THE_SAME:
		break;
	}
	routingStatus = NO_QUERY;
	tileEngineUpdateOptimizedRoute();

	if (canvas.routeMarkersChanged) {
		createQueryFromMarkers();
	}
}

