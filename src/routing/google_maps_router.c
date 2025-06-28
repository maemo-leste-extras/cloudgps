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

#include "google_maps_router.h"

//#define GOOGLE_ROUTING_URL "http://maps.googleapis.com/maps/api/directions/json?&origin=loc:%f,%f&destination=loc:%f,%f&sensor=true"
#define GOOGLE_ROUTING_URL "http://router.project-osrm.org/route/v1/driving/%f,%f;%f,%f?steps=true&geometries=polyline"
GArray* decodePolyline(const char* encoded) {
printf("test pre-5: %s\n", "ok");
	int len = strlen(encoded);
	int index = 0;
	GArray* array = g_array_new(FALSE, FALSE, sizeof(WorldCoordinate));
	if (array == NULL) {
		fprintf(stderr, "Memory allocation error in decodeLine\n");
		exit(1);
	}
	printf("debug test 5: %s\n", "ok");
	WorldCoordinate wc;
	wc.latitude = 0.0;
	wc.longitude = 0.0;

	while (index < len) {
		char b;
		int shift = 0;
		int bits = 0;
		do {
			b = encoded[index++] - 63;
			bits |= (b & 0x1f) << shift;
			shift += 5;
		} while (b >= 0x20);
		int dlat = ((bits & 1) ? ~(bits >> 1) : (bits >> 1));
		wc.latitude += dlat;

		shift = 0;
		bits = 0;
		do {
			b = encoded[index++] - 63;
			bits |= (b & 0x1f) << shift;
			shift += 5;
		} while (b >= 0x20);
		int dlng = ((bits & 1) ? ~(bits >> 1) : (bits >> 1));
		wc.longitude += dlng;

		WorldCoordinate result;

		result.latitude = wc.latitude * 1e-5;
		result.longitude = wc.longitude * 1e-5;

		array = g_array_append_vals(array, &result, 1);
	}

	return array;
}

void testDecodeLine() {
	int i;
	WorldCoordinate* wc;
	GArray
			* points =
					decodePolyline(
							"msdpHcsd_CsD|Aa@jPmVzJuCv@xAtc@fDYf@JrDxFnChAj\\nFzTpEI`ENp@`@f@tCb@p@z@jCdPlBxHbNh_@fEhN`BpH|C`UbA|OVzKUvUeAbQwMjhAqDpc@KhMt@dKdAjGfBdH~Pzj@teAbgDp@lClX~hBtNjbA~Svr@dH|Vh@|ERhF~@zc@a@tn@BrKLdDj@~IzHhy@rGxcA^nIiHnjBiClz@cDn|@s[xhKqErxCoAtmGb@dnDv@bG`A|CdH~QfJnQ`A`C~@~EJzFSxFWfBaBhH_@GMPGh@J\\qXpe@K@uDzFyCdGaAzCsAnJ@`AmArL{Bf`@kItwCgCtgCoGx_FwA|sABxa@YnqC^fIrB|PtCjOzGf[fBdFpAdCbEvEzA~@~ErAbCHhZo@zARxCjA`BjA~B~C|AfDrPfc@vBdJx@xFl@rHtDtbAPpI@dLc@~Go@bG}Ill@eBfPi@|LKtEDjMT|Kl@jMj@zu@[vOK~QoAtYY|QChFZrb@u@zH}FrUcAdHqHjzA}WlmBw@vHq@dOY`M@xLd@tWMbG]rDgBhJyFpVeH|h@qHD}GnAeHvCcAjAge@jaAwFlJoC~BsOvHAvEU`Bw@nC_CfEcDpEsApAsAh@mFj@aB^cAz@[lAItANlBXv@hDnEh@dCf@n^dD|}ARnN?dVWfNi@xM}ArTcDxWsT|tAcEhYcHjp@aWrgCaFpb@cH|]gNzo@mw@pmCcKj`@kCdPmApKs@nIi@|KWfTDjHlCpiD`Ahy@Z|gAP`Pv@jPhAzK~BrMbChKnBxFnG`O|C`F|O~RfEfHpDlIpBnG|AhGbB`LbAbL^dNIl{@Fti@RfHhAzQTtIw@dLp@nLUfC_ClIsDpKYRSCS]Ak@pFkOzD~W|DxOlCzP`@|D?z@aArIF|ErArEvDxJ^lB?jGsFdd@~FvB~IzEhA\\|GFbCUn@`@n@dBfBdB~BrA`@Mz@sAhDaCd@RjBhD`AsE_@}DMiELiDIeBqAgK{BwUwBqHqCwMs@rAwApAq@xBg@z@sAlAu@NYZaCrEc@pA");

	for (i = 0; i < points -> len; i++) {
		wc = &g_array_index(points, WorldCoordinate, i);
		fprintf(stderr, "lon = %2.5f, lat= %2.5f\n", wc -> latitude, wc -> longitude);
	}
}

char* googleMapsRouterPrepareUrl(RoutingQuery* query) {
	char* url;
	if (asprintf(&url, GOOGLE_ROUTING_URL, query -> start.longitude, query -> start.latitude, query -> end.longitude, query -> end.latitude) < 0) {
		fprintf(stderr, "asprintf failed in googleMapsRouterPrepareUrl\n");
		return NULL;
	}
	return url;
}

#define KEY_ROUTES "routes"
#define KEY_LEGS "legs"
#define KEY_STEPS "steps"
#define KEY_DISTANCE "distance"
#define KEY_VALUE "test"
#define KEY_POLYLINE "waypoints"
#define KEY_POINTS "hint"
#define KEY_HTML_INSTRUCTIONS "maneuver"
#define KEY_STATUS "code"

void googleMapsRouterParseResponse(char* response) {
	//	fprintf(stderr, "%s", response);
	size_t length;
	int i, metersFromStart = 0;

	route.statusMessage[0] = '\0';
	json_object* jobj = json_tokener_parse(response);
	if (jobj == NULL) {
		routingStatus = PARSE_ERROR;
		return;
	}

	json_object* json_routes = json_object_object_get(jobj, KEY_ROUTES);
	if (json_routes == NULL) {
		routingStatus = PARSE_ERROR;
		return;
	}
	length = json_object_array_length(json_routes);

	if (length > 0) {
		json_object* json_route = json_object_array_get_idx(json_routes, 0);
		json_object* json_legs = json_object_object_get(json_route, KEY_LEGS);

		// Assuming that Google always returns one leg
		json_object* json_leg = json_object_array_get_idx(json_legs, 0);

		//calculate total distance:
		json_object* stepgoo_distance = json_object_object_get(json_leg, "distance");
                route.lengthMeters += json_object_get_int(stepgoo_distance); 

//		route.lengthMeters = json_object_get_int(json_object_object_get(json_object_object_get(json_leg, KEY_DISTANCE), KEY_VALUE));

		json_object* json_steps = json_object_object_get(json_leg, KEY_STEPS);
		length = json_object_array_length(json_steps);

		printf("debug test 1: %s\n", "ok");
		route.directions = g_array_sized_new(FALSE, FALSE, sizeof(RouteDirection), length);

		int position = 0;
		for (i = 0; i < length; i++) {
			json_object* json_step = json_object_array_get_idx(json_steps, i);
			RouteDirection* direction = calloc(1, sizeof(RouteDirection));
			direction -> metersFromStart = metersFromStart;
		printf("debug test 2: %s\n", "ok");
			//direction -> polyLine = decodePolyline(json_object_get_string(json_object_object_get(json_object_object_get(json_step, KEY_POLYLINE), KEY_POINTS)));
//parsing is a bit different from osrm response:
			direction -> polyLine = decodePolyline(json_object_get_string(json_object_object_get(json_step, "geometry")));
		printf("debug test 2-2: %s\n", "ok");
			direction -> text = calloc(300, sizeof(char));
//			ascifyAndStripTags((char *) json_object_get_string(json_object_object_get(json_step, KEY_HTML_INSTRUCTIONS)), direction -> text);
			//TODO new instructions parsing must be improved
			ascifyAndStripTags((char*) json_object_get_string(json_object_object_get(json_step, "maneuver")), direction -> text);

//			metersFromStart += json_object_get_int(json_object_object_get(json_object_object_get(json_step, KEY_DISTANCE), KEY_VALUE));
//			current distance parsing with osrm:
			metersFromStart += json_object_get_int(json_object_object_get(json_step, "distance"));
			direction -> position = position;
			position += direction -> polyLine -> len - 1;
			route.directions = g_array_append_vals(route.directions, direction, 1);
		}
		routingStatus = RESULTS_READY;
	} else {
		routingStatus = ZERO_RESULTS;
		json_object* status = json_object_object_get(jobj, KEY_STATUS);
		printf("debug test 3: %s\n", "ok");
		if (status != NULL) {
			strcpy(route.statusMessage, json_object_get_string(status)) ;
		printf("debug test 4: %s\n", "ok");
		}
	}
}
