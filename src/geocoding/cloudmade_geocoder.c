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
#define KEY_CENTROID "geometry"
#define KEY_COORDINATES "coordinates"
//#define KEY_BOUNDS "bounds"
#define KEY_PROPERTIES "properties"
#define KEY_NAME "label"
#define KEY_IS_IN "is_in"
#define KEY_WIKIPEDIA "display_name"

//#define CLOUDMADE_QUERY_URL "http://geocoding.cloudmade.com/e4b1777b4b5154d69dbfc4678216183a/geocoding/v2/find.js?query=%s&return_location=true"
#define CLOUDMADE_QUERY_URL "https://nominatim.openstreetmap.org/search?q=%s&format=geojson"
char* cloudmade_prepare_url(char* encodedQuery) {
	char* url;
	if (asprintf(&url, CLOUDMADE_QUERY_URL, encodedQuery) < 0) {
		fprintf(stderr, "asprintf failed in cloudmade_prepare_url\n");
		return 0;
	}
	return url;
}

// Function to print each element of the GList
//void print_glist_item(gpointer data, gpointer user_data) {
    // Cast data to the expected type and print
//    printf("%s\n", (char *)data);
//}

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
			readCloudmadeWorldCoordinate(json_object_object_get(json_object_object_get(feature, "geometry"), "coordinates"), &result -> centroid);
//			readCloudmadeWorldCoordinate(json_object_array_get_idx(json_object_object_get(feature, "bbox"), 0), &result -> bounds[0]);
//			readCloudmadeWorldCoordinate(json_object_array_get_idx(json_object_object_get(feature, "bbox"), 1), &result -> bounds[1]);
			json_object *properties = json_object_object_get(feature, "properties");
			result -> name = calloc(300, sizeof(char));
			ascifyAndStripTags((char*) json_object_get_string(json_object_object_get(properties, "name")), (char*)result -> name);
			result -> description = calloc(300, sizeof(char));
			ascifyAndStripTags((char*) json_object_get_string(json_object_object_get(properties, "display_name")), (char*)result -> description);
//			result -> wikipedia = json_object_get_string(json_object_object_get(properties, KEY_WIKIPEDIA));

geocodingResults = g_list_append(geocodingResults, result);

//for (GList *l = geocodingResults; l != NULL; l = l->next) {
//	const char *json_test = (const char *)l->data;
//	printf("Result: %s\n", json_test);
//}


		}
		searchResultsStatus = RESULTS_READY;
	} else {
		searchResultsStatus = ZERO_RESULTS;
	}
}
