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
#define USER_CONFIG_FILE "/home/user/.cloudgps/config.ini"
#define GLOBAL_CONFIG_FILE "/opt/cloudgps/cloudgps.ini"
#define PROVIDERS_CONFIG_FILE "/opt/cloudgps/providers.ini"


#include "file.h"

char *GROUP_DEFAULTS = "Defaults";
void loadTileProviders() {
    GKeyFile *keyfile;
    GError *error = NULL;
    gsize numGroups;

    keyfile = g_key_file_new();

    if (!g_key_file_load_from_file(keyfile, PROVIDERS_CONFIG_FILE, 0, &error)) {
        fprintf(stderr, "%s", error->message);
        exit(1);
    }
    gchar** names = g_key_file_get_groups(keyfile, &numGroups);

    int i = 0;

    while (names[i] != NULL) {
        TileProvider *provider = calloc(1, sizeof(TileProvider));
        provider -> name = names[i];
        provider -> filenameFormat = g_key_file_get_string(keyfile, names[i], "fileFormat", NULL);
        provider -> urlFormatType = g_key_file_get_string(keyfile, names[i], "urlFormatType", NULL);
        gchar* quadKeyFirstChar = g_key_file_get_string(keyfile, names[i], "quadKeyFirstChar", NULL);
        if (quadKeyFirstChar) {
        	provider -> quadKeyFirstChar = quadKeyFirstChar[0];
        } else {
        	provider -> quadKeyFirstChar = '0';
        }
        provider -> urlFormat = g_key_file_get_string(keyfile, names[i], "urlFormat", NULL);
        provider -> minZoom = g_key_file_get_integer(keyfile, names[i], "minZoom", NULL);
        provider -> maxZoom = g_key_file_get_integer(keyfile, names[i], "maxZoom", NULL);
//        fprintf(stderr, "tile provider = '%s', fileFormat = '%s', urlFormat = '%s', urlFormatType='%s', quadKeyFirstChar='%c', minZoom = %d, maxZoom =%d\n",
//        		provider -> name, provider -> filenameFormat, provider -> urlFormat, provider -> urlFormatType,provider -> quadKeyFirstChar, provider -> minZoom, provider -> maxZoom);
        tileProviders = g_list_append(tileProviders, provider);
        i++;
    }
    g_key_file_free(keyfile);
}

void* getProviderByName(char* name, GList* allProviders) {
	if (name != NULL) {
		GList* elem = allProviders;

		while (elem != NULL) {
			BackgroundQueryProvider* provider = elem -> data;

			if (g_str_equal(provider -> name, name)) {
    			return provider;
    		}
			elem = elem -> next;
		}
    }
	return allProviders -> data;
}

void loadConfig() {
    loadTileProviders();
	initGeocodingProviders();
	initRoutingProviders();

    GKeyFile *keyfile;
    GError *error = NULL;
    keyfile = g_key_file_new();
    gchar *tileProvider = NULL;
    gchar *geocodingProvider = NULL;
    gchar *routingProvider = NULL;
    gchar *ttfFont = NULL;

    if (g_key_file_load_from_file(keyfile, GLOBAL_CONFIG_FILE, 0, &error)) {
        options.referer = g_key_file_get_string(keyfile, "Network", "httpReferer", NULL);
        options.userAgent = g_key_file_get_string(keyfile, "Network", "httpUserAgent", NULL);
        options.curlVerbose = g_key_file_get_integer(keyfile, "Network", "curlVerbose", NULL);
        options.forceIPv4 = g_key_file_get_integer(keyfile, "Network", "forceIPv4", NULL);
        canvas.zoom = g_key_file_get_integer(keyfile, GROUP_DEFAULTS, "zoom", NULL);
        canvas.tilex = g_key_file_get_integer(keyfile, GROUP_DEFAULTS, "tilex", NULL);
        canvas.tiley = g_key_file_get_integer(keyfile, GROUP_DEFAULTS, "tiley", NULL);
        options.mipmapToggle = g_key_file_get_integer(keyfile, GROUP_DEFAULTS, "useMipmaps", NULL);
        options.showDebugTiles = g_key_file_get_integer(keyfile, GROUP_DEFAULTS, "showDebugTiles", NULL);
        tileProvider = g_key_file_get_string(keyfile, GROUP_DEFAULTS, "tileProvider", NULL);
        geocodingProvider = g_key_file_get_string(keyfile, GROUP_DEFAULTS, "geocodingProvider", NULL);
        options.downloadedFadeTime = g_key_file_get_integer(keyfile, GROUP_DEFAULTS, "downloadedFadeTime", NULL);
        options.zoomChangeFadeTime = g_key_file_get_integer(keyfile, GROUP_DEFAULTS, "zoomChangeFadeTime", NULL);
        options.locallyLoadedFadeTime = g_key_file_get_integer(keyfile, GROUP_DEFAULTS, "locallyLoadedFadeTime", NULL);
        canvas.rotation2d = g_key_file_get_integer(keyfile, GROUP_DEFAULTS, "rotation2d", NULL);
        ttfFont = g_key_file_get_string(keyfile, GROUP_DEFAULTS, "ttfFont", NULL);
        //font=TTF_OpenFont(ttfFont, 16);
//        if(!font) {
//            fprintf(stderr, "TTF_OpenFont: %s\n", TTF_GetError());
            //exit(1);
//        }
    } else {
        fprintf(stderr, "Error loading %s %s\n", GLOBAL_CONFIG_FILE, error->message);
        canvas.zoom = 2;
        canvas.x = 0;
        canvas.y = 0;
        canvas.tilex = -1;
        canvas.tiley = -1;
        options.mipmapToggle = 1;
        options.showDebugTiles = 0;
        options.downloadedFadeTime = 1500;
        options.zoomChangeFadeTime = 2000;
        options.locallyLoadedFadeTime = 200;
    }

    options.mouseHoldToPopupTime = 1000;
    options.snapToRouteToleranceMeters = 30;
    options.outsideRouteToleranceMeters = 50;
    options.recalculateRouteAfterOutSideRouteSeconds = 10;
    options.useFading = FALSE;
    options.useTileSync = FALSE;
    options.popupEntryMargin = 10;

    g_key_file_free(keyfile);
    keyfile = g_key_file_new();

    if (g_key_file_load_from_file(keyfile, USER_CONFIG_FILE, 0, &error)) {
        canvas.zoom = g_key_file_get_integer(keyfile, "CurrentView", "zoom", NULL);
        canvas.tilex = g_key_file_get_integer(keyfile, "CurrentView", "tilex", NULL);
        canvas.tiley = g_key_file_get_integer(keyfile, "CurrentView", "tiley", NULL);
        canvas.x = g_key_file_get_double(keyfile, "CurrentView", "x", NULL);
        if (isnan(canvas.x)) {
            canvas.x = 0.0;
        }
        canvas.y = g_key_file_get_double(keyfile, "CurrentView", "y", NULL);
        if (isnan(canvas.y)) {
            canvas.y = 0.0;
        }
        tileProvider = g_key_file_get_string(keyfile, "Options", "tileProvider", NULL);
        geocodingProvider = g_key_file_get_string(keyfile, "Options", "geocodingProvider", NULL);
        routingProvider = g_key_file_get_string(keyfile, "Options", "routingProvider", NULL);
        options.loadLastRoute = g_key_file_get_integer(keyfile, "Options", "loadLastRoute", NULL);
    } else {
        fprintf(stderr, "Error loading %s %s\n", USER_CONFIG_FILE, error->message);
    }

    if (tileProvider != NULL) {
        GList *provider = tileProviders;

        while (provider != NULL) {
            TileProvider *data = provider -> data;
            if (g_str_equal(data -> name, tileProvider)) {
                canvas.provider = data;

                break;
            }
            provider = provider -> next;
        }
    }
    if (canvas.provider == NULL) {
        canvas.provider = tileProviders -> data;
    }

	options.geocodingProvider = getProviderByName(geocodingProvider, geocodingProviders);
	options.routingProvider = getProviderByName(routingProvider, routingProviders);
}

void saveConfig() {
    ensureDirExists(USER_CONFIG_FILE);
    GKeyFile *keyfile;
    gsize lenght;
    GError *error = NULL;
    keyfile = g_key_file_new();

    if (!g_key_file_load_from_file(keyfile, USER_CONFIG_FILE, G_KEY_FILE_KEEP_COMMENTS, &error)) {
        fprintf(stderr, "Error loading file %s: %s\n", USER_CONFIG_FILE, error->message);
    }
    g_key_file_set_integer(keyfile, "CurrentView", "zoom", canvas.zoom);
    g_key_file_set_integer(keyfile, "CurrentView", "tilex", canvas.tilex);
    g_key_file_set_integer(keyfile, "CurrentView", "tiley", canvas.tiley);
    g_key_file_set_double(keyfile, "CurrentView", "x", canvas.x);
    g_key_file_set_double(keyfile, "CurrentView", "y", canvas.y);
    g_key_file_set_string(keyfile, "Options", "tileProvider", canvas.provider -> name);
    g_key_file_set_string(keyfile, "Options", "geocodingProvider", options.geocodingProvider -> name);
    g_key_file_set_string(keyfile, "Options", "routingProvider", options.routingProvider -> name);
    g_key_file_set_integer(keyfile, "Options", "loadLastRoute", canvas.route.directions == NULL ? 0 : 1);
    gchar* data = g_key_file_to_data(keyfile, &lenght, &error);
    if (data == NULL || lenght <= 0) {
        fprintf(stderr, "Error dumping setting to data: %s\n", error->message);
    } else {
        FILE* file = fopen(USER_CONFIG_FILE, "wt");
        if (file != NULL) {
            fprintf(file, "%s", data);
            fclose(file);
        } else {
            fprintf(stderr, "Unable to open %s for writing.\n", USER_CONFIG_FILE);
        }
    }
}
