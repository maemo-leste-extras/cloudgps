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

#include "network.h"

extern int downloaded;
extern char strbuf[];

size_t writeToFileStream(void *ptr, size_t size, size_t nmemb, void *stream) {
    fwrite(ptr, size, nmemb, (FILE *) stream);
    downloaded += nmemb * size;
    return nmemb * size;
};

int downloadAndStream(char *url, void* stream, void *writeToStream) {
	if (options.offlineMode) {
		return FALSE;
	}
    CURL *curl = curl_easy_init();
    CURLcode res;
    long http_code;
    if (curl == NULL) {
        fprintf(stderr, "downloadUrl failed: curl not initialized\n");
        return FALSE;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    if (options.forceIPv4) {
		curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    }
    if (options.referer != NULL) {
        curl_easy_setopt(curl, CURLOPT_REFERER, options.referer);
    }

    if (options.userAgent != NULL) {
        curl_easy_setopt(curl, CURLOPT_USERAGENT, options.userAgent);
    }
    if (options.curlVerbose) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToStream);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, stream);
    res = curl_easy_perform(curl);
    if (CURLE_OK == res) {
         curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

         curl_easy_cleanup(curl);
         if (http_code == 200) {
             free(url);
 //            fprintf(stderr, "download successfull, %ld\n", http_code);
             return TRUE;
         } else {
        	 char tmp[500];
             sprintf(tmp, "HTTP status %ld for url: '%s'\n", http_code, url);
             fprintf(stderr, "%s\n", tmp);
             addConsoleLine(tmp, 1, 0, 0);
             free(url);
             return FALSE;
         }
     } else {
    	 char tmp[500];
         sprintf(tmp, "D/l error %d: %s for URL: '%s'", res, curl_easy_strerror(res), url);
         fprintf(stderr, "%s\n", tmp);
         addConsoleLine(tmp, 1, 0, 0);
     }

     curl_easy_cleanup(curl);
     free(url);
     return FALSE;
}


int downloadAndSave(char *url, char *filename) {
    FILE *destFile;
//    long startTime = SDL_GetTicks();
//    fprintf(stderr, "downloading '%s' to '%s'\n", url, filename);
    destFile = fopen(filename, "wb");

    if (destFile == NULL) {
        sprintf(strbuf, "Unable to open file '%s' for writing", filename);
        addConsoleLine(strbuf, 1, 0, 0);
        return FALSE;
    }
    int result = downloadAndStream(url, destFile, &writeToFileStream);
    if (!result) {
        remove(filename);
    }
    fclose(destFile);

//    fprintf(stderr, "download '%s' took %ld ms\n", url, (SDL_GetTicks() - startTime));
    return result;
}
