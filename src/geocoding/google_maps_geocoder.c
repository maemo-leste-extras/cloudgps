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
//http://maps.googleapis.com/maps/api/geocode/xml?latlng=52.40590109606264,16.92497491836548&sensor=true

#include "geocoder.h"
#include "google_maps_geocoder.h"

#define KEY_RESPONSE_DATA "responseData"
#define KEY_RESULTS "results"
#define KEY_LON "lng"
#define KEY_LAT "lat"
#define KEY_TITLE_NO_FORMATTING "titleNoFormatting"

#define GOOGLE_MAPS_QUERY_URL "http://ajax.googleapis.com/ajax/services/search/local?start=0&rsz=large&v=1.0&q=%s"
#define GOOGLE_MAPS_LOCAL_QUERY_URL "http://ajax.googleapis.com/ajax/services/search/local?start=0&rsz=large&v=1.0&q=%s%%20loc:%f,%f"

char* google_maps_local_prepare_url(char* encodedQuery) {
	char* url;
	if (asprintf(&url, GOOGLE_MAPS_LOCAL_QUERY_URL, encodedQuery, canvas.center.latitude, canvas.center.longitude) < 0) {
		fprintf(stderr, "asprintf failed in google_local_prepare_url\n");
		return 0;
	}
	return url;
}

char* google_maps_prepare_url(void* query) {
	char* encodedQuery = query;
	char* url;
	if (asprintf(&url, GOOGLE_MAPS_QUERY_URL, encodedQuery) < 0) {
		fprintf(stderr, "asprintf failed in google_local_prepare_url\n");
		return 0;
	}
	return url;
}


// responseData ->
//             results ->
//						lat -> double
//						lng -> double
//						titleNoFormatting -> string


void google_maps_parse_response(char* response) {
	//	fprintf(stderr, "%s", response);
	size_t length;
	int i;
	json_object * jobj = json_tokener_parse(response);
	if (jobj == NULL) {
		searchResultsStatus = PARSE_ERROR;
		return;
	}

	json_object * json_response_data = json_object_object_get(jobj, KEY_RESPONSE_DATA);

	if (json_response_data != NULL) {
		json_object * json_results = json_object_object_get(json_response_data, KEY_RESULTS);

		if (json_results != NULL) {
			length = json_object_array_length(json_results);
			if (length > 0) {
				for (i = 0; i < length; i++) {
					json_object * json_result = json_object_array_get_idx(json_results, i);
					GeoCodeResult * result = calloc(1, sizeof(GeoCodeResult));
					result -> name = calloc(300, sizeof(char));
					ascifyAndStripTags((char*)json_object_get_string(json_object_object_get(json_result, KEY_TITLE_NO_FORMATTING)), (char*)result -> name);
					readGoogleWorldCoordinate(json_result, &result -> centroid);
					result -> description = result -> name;
					result -> wikipedia = NULL;

//					fprintf(stderr, "Name: '%s' Description: '%s', center: %f, %f, Wikipedia: '%s'\n", result -> name, result -> description, result -> centroid.latitude, result -> centroid.longitude, result-> wikipedia == NULL ? "null": result -> wikipedia);
					geocodingResults = g_list_append(geocodingResults, result);
				}
				searchResultsStatus = RESULTS_READY;
			} else {
				searchResultsStatus = ZERO_RESULTS;
			}
		} else {
			searchResultsStatus = ZERO_RESULTS;
		}
	} else {
		searchResultsStatus = ZERO_RESULTS;
	}
}
