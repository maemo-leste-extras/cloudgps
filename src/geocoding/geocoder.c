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

#include "geocoder.h"
#include "cloudmade_geocoder.h"
//#include "google_geocoder.h"
//#include "google_maps_geocoder.h"
#include "geocoder.h"

volatile BackgroundQueryStatus searchResultsStatus = NO_QUERY;

GList* geocodingProviders;

GList* geocodingResults;


#define QUERY_TMP_FILENAME "/tmp/cloudgps-query-tmp.js"

char* cloudmade_prepare_url(char* encodedQuery);
char* (*prepareUrl)(void* query);

void initGeocodingProviders() {
	BackgroundQueryProvider* p;
	p = calloc(1, sizeof(BackgroundQueryProvider));
	p -> name = "CloudMade";
	p -> parseResponse = &cloudmade_parse_response;
	p -> prepareUrl = (prepare_url_t)&cloudmade_prepare_url;
	geocodingProviders = g_list_append(geocodingProviders, p);
#if 0
	p = calloc(1, sizeof(BackgroundQueryProvider));
	p -> name = "Google (address only)";
	p -> parseResponse = &google_parse_response;
	p -> prepareUrl = (prepare_url_t)&google_prepare_url;
	geocodingProviders = g_list_append(geocodingProviders, p);

	p = calloc(1, sizeof(BackgroundQueryProvider));
	p -> name = "Google Maps";
	p -> parseResponse = &google_maps_parse_response;
	p -> prepareUrl = (prepare_url_t)&google_maps_prepare_url;
	geocodingProviders = g_list_append(geocodingProviders, p);

	p = calloc(1, sizeof(BackgroundQueryProvider));
	p -> name = "Google Maps Local";
	p -> parseResponse = &google_maps_parse_response;
	p -> prepareUrl = (prepare_url_t)&google_maps_local_prepare_url;
	geocodingProviders = g_list_append(geocodingProviders, p);
#endif
}

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

// Download and prepare search results.
int geocodeBgThread(void* query) {
    char *url, *response, *encoded;
    size_t length;

    encoded = url_encode((char*) query);
    free(query);

    url = options.geocodingProvider -> prepareUrl(encoded);
	fprintf(stderr, "Search URL: %s\n", url);
    free(encoded);

    if (url == NULL) {
		fprintf(stderr, "Search URL is NULL\n");
		searchResultsStatus = DOWNLOAD_ERROR;
    	return 0;
    }

    if (downloadAndSave(url, QUERY_TMP_FILENAME)) {
        if (g_file_get_contents(QUERY_TMP_FILENAME, &response, &length, NULL)) {
        	options.geocodingProvider -> parseResponse(response);

            g_free(response);
            remove(QUERY_TMP_FILENAME);
        } else {
			searchResultsStatus = DOWNLOAD_ERROR;
            fprintf(stderr, "Unable to read query results from %s\n", QUERY_TMP_FILENAME);
        }
    }
    return 0;
}

void processNewSearchQuery(char* query) {
	if (searchResultsStatus != NO_QUERY) {
		sprintf(strbuf, "Search query: thread busy - try again");
		addConsoleLine(strbuf, 1, 1, 0);
	} else {
		char* copy = malloc(SEARCHBAR_MAX_CHARS+1);
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
		sprintf(strbuf, "Search download error");
		addConsoleLine(strbuf, 1, 0, 0);
		break;
	case PARSE_ERROR:
		sprintf(strbuf, "Search parse error");
		addConsoleLine(strbuf, 1, 0, 0);
		break;
	case ZERO_RESULTS:
		sprintf(strbuf, "Search query: no results");
		addConsoleLine(strbuf, 0, 1, 0);
		break;
	case RESULTS_READY:
		sprintf(strbuf, "Search query: %d results", g_list_length(geocodingResults));
		addConsoleLine(strbuf, 0, 1, 0);
	    tileEngineProcessSearchResults(geocodingResults);
		g_list_free(geocodingResults);
		geocodingResults = NULL;
		break;
	/* XXX: Not handled previously, let's just have it here to indicate
 	 * default path explicitly */
	case QUERY_THE_SAME:
		break;
	}
	searchResultsStatus = NO_QUERY;
}
