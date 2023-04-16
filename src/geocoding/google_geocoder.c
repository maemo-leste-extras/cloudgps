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

//http://maps.googleapis.com/maps/api/geocode/xml?address=krakow,wybickiego%2014&sensor=true
//reverse geocoder: http://maps.googleapis.com/maps/api/geocode/xml?latlng=52.40590109606264,16.92497491836548&sensor=true

#include "geocoder.h"
#include "google_geocoder.h"

#define GOOGLE_QUERY_URL "http://maps.googleapis.com/maps/api/geocode/json?address=%s&sensor=true"
#define KEY_RESULTS "results"
#define KEY_FORMATTED_ADDRESS "formatted_address"
#define KEY_GEOMETRY "geometry"
#define KEY_LOCATION "location"
#define KEY_VIEWPORT "viewport"
#define KEY_SOUTHWEST "southwest"
#define KEY_NORTHEAST "northeast"

// results ->
//             formatted_address
//             geometry ->
//						location ->
//								lat -> double
//								lon -> double
//						viewport ->
//								northeast ->
//										lat -> double
//										lon -> double
//								southwest ->
//										lat -> double
//										lon -> double


char* google_prepare_url(char* encodedQuery) {
	char* url;
	if (asprintf(&url, GOOGLE_QUERY_URL, encodedQuery) < 0) {
		fprintf(stderr, "asprintf failed in google_prepare_url\n");
		return 0;
	}
	return url;
}

void google_parse_response(char* response) {
	//	fprintf(stderr, "%s", response);
	size_t length;
	int i;
	json_object * jobj = json_tokener_parse(response);
	if (jobj == NULL) {
		searchResultsStatus = PARSE_ERROR;
		return;
	}
	json_object * json_results = json_object_object_get(jobj, KEY_RESULTS);
	if (json_results != NULL) {
		length = json_object_array_length(json_results);
		if (length > 0) {
			for (i = 0; i < length; i++) {
				json_object * json_result = json_object_array_get_idx(json_results, i);
				GeoCodeResult * result = calloc(1, sizeof(GeoCodeResult));
				result -> name = calloc(300, sizeof(char));
				ascifyAndStripTags((char*) json_object_get_string(json_object_object_get(json_result, KEY_FORMATTED_ADDRESS)), (char*)result -> name);
				json_object *geometry = json_object_object_get(json_result, KEY_GEOMETRY);

				readGoogleWorldCoordinate(json_object_object_get(geometry, KEY_LOCATION), &result -> centroid);
				readGoogleWorldCoordinate(json_object_object_get(json_object_object_get(geometry, KEY_VIEWPORT), KEY_NORTHEAST), &result -> bounds[0]);
				readGoogleWorldCoordinate(json_object_object_get(json_object_object_get(geometry, KEY_VIEWPORT), KEY_SOUTHWEST), &result -> bounds[1]);
				result -> description = result -> name;
				result -> wikipedia = NULL;

				//				fprintf(stderr, "Name: '%s' Description: '%s', center: %f, %f, Wikipedia: '%s'\n", result -> name, result -> description, result -> centroid.latitude, result -> centroid.longitude, result-> wikipedia == NULL ? "null": result -> wikipedia);
				geocodingResults = g_list_append(geocodingResults, result);
			}
			searchResultsStatus = RESULTS_READY;
		} else {
			searchResultsStatus = ZERO_RESULTS;
		}
	} else {
		searchResultsStatus = ZERO_RESULTS;
	}
}
