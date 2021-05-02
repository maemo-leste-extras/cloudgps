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

#include "cloudmade_router.h"

#define CLOUDMADE_CLOUDGPS_API_KEY "e4b1777b4b5154d69dbfc4678216183a"

#define CLOUDMADE_ROUTING_URL "http://routes.cloudmade.com/" CLOUDMADE_CLOUDGPS_API_KEY "/api/0.3/%f,%f,%f,%f/car.js?units=km&lang=en"

char* cloudmadeRouterPrepareUrl(void* queryPtr) {
	RoutingQuery* query = queryPtr;
	char* url;
	if (asprintf(&url, CLOUDMADE_ROUTING_URL, query -> start.latitude, query -> start.longitude, query -> end.latitude, query -> end.longitude) < 0) {
		fprintf(stderr, "asprintf failed in cloudmadeRouterPrepareUrl\n");
		return NULL;
	}
	return url;
}

#define KEY_STATUS "status"
#define KEY_STATUS_MESSAGE "status_message"
#define KEY_ROUTE_SUMMARY "route_summary"
#define KEY_TOTAL_DISTANCE "total_distance"
#define KEY_ROUTE_GEOMETRY "route_geometry"
#define KEY_ROUTE_INSTRUCTIONS "route_instructions"
#define IDX_INSTRUCTION 0
#define IDX_LENGTH 1
#define IDX_POSITION 2
#define IDX_LAT 0
#define IDX_LON 1

void cloudmadeRouterParseResponse(char* response) {
//	fprintf(stderr, "%s\n", response);

	size_t instructionLength;
	int i, geometryLength, metersFromStart = 0;

	route.statusMessage[0] = '\0';
	json_object* jobj = json_tokener_parse(response);
	if (is_error(jobj)) {
		routingStatus = PARSE_ERROR;
		return;
	}

	int status = json_object_get_int(json_object_object_get(jobj, KEY_STATUS));

	if (status == 0) {
		json_object* json_instructions = json_object_object_get(jobj, KEY_ROUTE_INSTRUCTIONS);
		if (json_instructions == NULL) {
			routingStatus = PARSE_ERROR;
			return;
		}
		instructionLength = json_object_array_length(json_instructions);
		route.directions = g_array_sized_new(FALSE, FALSE, sizeof(RouteDirection), instructionLength);

		route.lengthMeters = json_object_get_double(json_object_object_get(json_object_object_get(jobj, KEY_ROUTE_SUMMARY), KEY_TOTAL_DISTANCE));

		for (i = 0; i < instructionLength; i++) {
			RouteDirection* direction = calloc(1, sizeof(RouteDirection));
			json_object* json_instruction = json_object_array_get_idx(json_instructions, i);
			direction -> metersFromStart = metersFromStart;
			direction -> text = calloc(300, sizeof(char));
			ascifyAndStripTags((char*) json_object_get_string(json_object_array_get_idx(json_instruction, IDX_INSTRUCTION)), direction -> text);

			direction -> position = json_object_get_int(json_object_array_get_idx(json_instruction, IDX_POSITION));
			metersFromStart += json_object_get_double(json_object_array_get_idx(json_instruction, IDX_LENGTH));
			g_array_append_vals(route.directions, direction, 1);
		}

		json_object* json_geometry = json_object_object_get(jobj, KEY_ROUTE_GEOMETRY);
		geometryLength = json_object_array_length(json_geometry);

		for (i = 0; i < instructionLength; i++) {
			int startIdx, endIdx; // inclusive indexes
			RouteDirection direction = g_array_index(route.directions, RouteDirection, i);

			startIdx = direction.position;
			if (i >= instructionLength - 1) {
				endIdx = geometryLength - 1;
			} else {
				endIdx = g_array_index(route.directions, RouteDirection, (i + 1)).position;
			}
			int j;
			fprintf(stderr, "length %d, start %d, end %d\n", geometryLength, startIdx, endIdx);
			direction.polyLine = g_array_new(FALSE, FALSE, sizeof(WorldCoordinate));
			for (j = startIdx; j <= endIdx; j++) {
				if (j >= geometryLength) {
					fprintf(stderr, "cloudmadeRouterParseResponse: internal error: j >= geometryLength, j = %d, geometryLength = %d\n", j, geometryLength);
				}
				json_object* json_point = json_object_array_get_idx(json_geometry, j);
				WorldCoordinate wc;
				json_object* json_lat = json_object_array_get_idx(json_point, IDX_LAT);
				wc.latitude = json_object_get_double(json_lat);
				wc.longitude = json_object_get_double(json_object_array_get_idx(json_point, IDX_LON));
				direction.polyLine = g_array_append_vals(direction.polyLine, &wc, 1);
			}
			g_array_index(route.directions, RouteDirection, i).polyLine = direction.polyLine;
		}

		routingStatus = RESULTS_READY;
	} else {
		routingStatus = ZERO_RESULTS;
		json_object* statusMessage = json_object_object_get(jobj, KEY_STATUS_MESSAGE);
		if (statusMessage != NULL) {
			strcpy(route.statusMessage, json_object_get_string(statusMessage));
		}
	}
}
