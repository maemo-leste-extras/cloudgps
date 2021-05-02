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

#include <stdlib.h>
#include <stdio.h>
#include <json-c/json.h>
#include <json-c/json_object_private.h>

#include "tileengine.h"
#include "console.h"


//http://maps.googleapis.com/maps/api/geocode/xml?address=krakow,wybickiego%2014&sensor=true
//http://maps.googleapis.com/maps/api/geocode/xml?latlng=52.40590109606264,16.92497491836548&sensor=true


#define CLOUDMADE_QUERY_URL "http://geocoding.cloudmade.com/e4b1777b4b5154d69dbfc4678216183a/geocoding/v2/find.js?query=%s&return_location=true"
#define QUERY_TMP_FILENAME "/tmp/cloudgps-query-tmp.js"

#define KEY_FEATURES "features"
#define KEY_CENTROID "centroid"
#define KEY_COORDINATES "coordinates"
#define KEY_BOUNDS "bounds"
#define KEY_PROPERTIES "properties"
#define KEY_NAME "name"
#define KEY_IS_IN "is_in"
#define KEY_WIKIPEDIA "Wikipedia"

/* to_hex and url_encode taken from: http://www.geekhideout.com/urlcode.shtml */

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
  char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
      *pbuf++ = *pstr;
    else if (*pstr == ' ')
      *pbuf++ = '+';
    else
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}




void readWorldCoordinate(json_object * jobj, WorldCoordinate *wc) {
    wc -> latitude = json_object_get_double(json_object_array_get_idx(jobj, 0));
    wc -> longitude = json_object_get_double(json_object_array_get_idx(jobj, 1));
}




// features ->
//             centroid -> coordinates -> array[2] -> double
//             bounds -> array[2] -> array[2] -> double
//             properties ->
//                            name
//                            is_in

typedef enum {
	NO_QUERY, QUERY_IN_PROGRESS, DOWNLOAD_ERROR, PARSE_ERROR, ZERO_RESULTS, RESULTS_READY
} SearchResultStatus;

volatile SearchResultStatus searchResultsStatus = NO_QUERY;
GList* results;
// Download and prepare search results.
int geocodeBgThread(void* query) {
    char *url, *json_string, *tmp;
    int i;
    size_t length;

    tmp = url_encode((char*) query);
    free(query);

    if (asprintf(&url, CLOUDMADE_QUERY_URL, tmp) < 0) {
    	fprintf(stderr, "asprintf failed in geocode\n");
    	return 0;
    }
	fprintf(stderr, "Search URL: %s\n", url);
    free(tmp);

    if (downloadAndSave(url, QUERY_TMP_FILENAME)) {
        if (g_file_get_contents(QUERY_TMP_FILENAME, &json_string, &length, NULL)) {
//            fprintf(stderr, "%s\n", json_string);
            json_object * jobj = json_tokener_parse(json_string);
            json_object * features = json_object_object_get(jobj, KEY_FEATURES);
            if (features != NULL) {
                length = json_object_array_length(features);
                for (i = 0; i < length; i++) {
                    json_object * feature = json_object_array_get_idx(features, i);
                    GeoCodeResult * result = calloc(1, sizeof(GeoCodeResult));
                    readWorldCoordinate(json_object_object_get(json_object_object_get(feature, KEY_CENTROID), KEY_COORDINATES), &result -> centroid);
                    readWorldCoordinate(json_object_array_get_idx(json_object_object_get(feature, KEY_BOUNDS), 0), &result -> bounds[0]);
                    readWorldCoordinate(json_object_array_get_idx(json_object_object_get(feature, KEY_BOUNDS), 1), &result -> bounds[1]);
                    json_object *properties = json_object_object_get(feature, KEY_PROPERTIES);
                    result -> name = json_object_get_string(json_object_object_get(properties, KEY_NAME));
                    result -> description = json_object_get_string(json_object_object_get(properties, KEY_IS_IN));
                    result -> wikipedia = json_object_get_string(json_object_object_get(properties, KEY_WIKIPEDIA));

//                    fprintf(stderr, "Name: '%s' Description: '%s', center: %f, %f, Wikipedia: '%s'\n", result -> name, result -> description, result -> centroid.latitude, result -> centroid.longitude, result-> wikipedia == NULL ? "null": result -> wikipedia);
                    results = g_list_append(results, result);
                }
                searchResultsStatus = RESULTS_READY;
            	return 0;
            } else {
            	searchResultsStatus = ZERO_RESULTS;
            	return 0;
            }

            g_free(json_string);

			searchResultsStatus = PARSE_ERROR;
        } else {
			searchResultsStatus = DOWNLOAD_ERROR;
            fprintf(stderr, "Unable to read query results from %s\n", QUERY_TMP_FILENAME);
        }
    }
    searchResultsStatus ;
    return 0;
}

void spawnQueryThread(char* query) {
	if (searchResultsStatus != NO_QUERY) {
		sprintf(buf, "Search query: thread busy - try again");
		addConsoleLine(buf, 1, 1, 0);
	} else {
		char* copy = malloc(SEARCHBAR_MAX_CHARS);
		strncpy(copy, query, SEARCHBAR_MAX_CHARS);
		SDL_CreateThread(geocodeBgThread, (void*) copy);
	}
}

void processGeocoding() {
	switch (searchResultsStatus) {
	case NO_QUERY:
	case QUERY_IN_PROGRESS:
		break;
	case DOWNLOAD_ERROR:
		sprintf(buf, "Search download error");
		addConsoleLine(buf, 1, 0, 0);
		break;
	case PARSE_ERROR:
		sprintf(buf, "Search parse error");
		addConsoleLine(buf, 1, 0, 0);
		break;
	case ZERO_RESULTS:
		sprintf(buf, "Search query: no results");
		addConsoleLine(buf, 0, 1, 0);
		break;
	case RESULTS_READY:
		sprintf(buf, "Search query: %d results", g_list_length(results));
		addConsoleLine(buf, 0, 1, 0);
	    tileEngineProcessSearchResults(results);
		g_list_free(results);
		results = NULL;
		break;
	}
	searchResultsStatus = NO_QUERY;
}
