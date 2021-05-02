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


#ifndef GEOCODER_H_
#define GEOCODER_H_

#include <SDL/SDL.h>

#include "../main.h"
#include "../network.h"
#include "../tileengine.h"
#include "../unicode2ascii.h"

#include "coordinate_reader.h"

extern volatile BackgroundQueryStatus searchResultsStatus;
extern GList* geocodingResults;

void initGeocodingProviders();

void processNewSearchQuery(char* query);

void processGeocoding();

#endif /* GEOCODER_H_ */
