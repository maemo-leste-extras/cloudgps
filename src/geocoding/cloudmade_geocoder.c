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

#include "cloudmade_geocoder.h"

#define KEY_FEATURES "features"
#define KEY_CENTROID "centroid"
#define KEY_COORDINATES "coordinates"
#define KEY_BOUNDS "bounds"
#define KEY_PROPERTIES "properties"
#define KEY_NAME "name"
#define KEY_IS_IN "is_in"
#define KEY_WIKIPEDIA "Wikipedia"

#define CLOUDMADE_QUERY_URL "http://geocoding.cloudmade.com/e4b1777b4b5154d69dbfc4678216183a/geocoding/v2/find.js?query=%s&return_location=true"
char* cloudmade_prepare_url(char* encodedQuery) {
	char* url;
	if (asprintf(&url, CLOUDMADE_QUERY_URL, encodedQuery) < 0) {
		fprintf(stderr, "asprintf failed in cloudmade_prepare_url\n");
		return 0;
	}
	return url;
}

// features ->
//             centroid -> coordinates -> array[2] -> double
//             bounds -> array[2] -> array[2] -> double
//             properties ->
//                            name
//                            is_in

void cloudmade_parse_response(char* response) {
	// fprintf(stderr, "%s\n", json_string);
	size_t length;
	int i;

	json_object * jobj = json_tokener_parse(response);
	if (jobj == NULL) {
		searchResultsStatus = PARSE_ERROR;
		return;
	}
	json_object * features = json_object_object_get(jobj, KEY_FEATURES);
	if (features != NULL) {
		length = json_object_array_length(features);
		for (i = 0; i < length; i++) {
			json_object * feature = json_object_array_get_idx(features, i);
			GeoCodeResult * result = calloc(1, sizeof(GeoCodeResult));
			readCloudmadeWorldCoordinate(json_object_object_get(json_object_object_get(feature, KEY_CENTROID), KEY_COORDINATES), &result -> centroid);
			readCloudmadeWorldCoordinate(json_object_array_get_idx(json_object_object_get(feature, KEY_BOUNDS), 0), &result -> bounds[0]);
			readCloudmadeWorldCoordinate(json_object_array_get_idx(json_object_object_get(feature, KEY_BOUNDS), 1), &result -> bounds[1]);
			json_object *properties = json_object_object_get(feature, KEY_PROPERTIES);
			result -> name = calloc(300, sizeof(char));
			ascifyAndStripTags((char*) json_object_get_string(json_object_object_get(properties, KEY_NAME)), (char*)result -> name);
			result -> description = calloc(300, sizeof(char));
			ascifyAndStripTags((char*) json_object_get_string(json_object_object_get(properties, KEY_IS_IN)), (char*)result -> description);
			result -> wikipedia = json_object_get_string(json_object_object_get(properties, KEY_WIKIPEDIA));

			//                    fprintf(stderr, "Name: '%s' Description: '%s', center: %f, %f, Wikipedia: '%s'\n", result -> name, result -> description, result -> centroid.latitude, result -> centroid.longitude, result-> wikipedia == NULL ? "null": result -> wikipedia);
			geocodingResults = g_list_append(geocodingResults, result);
		}
		searchResultsStatus = RESULTS_READY;
	} else {
		searchResultsStatus = ZERO_RESULTS;
	}
}
