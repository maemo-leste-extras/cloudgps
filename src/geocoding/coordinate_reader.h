/*
 * coordinateReader.h
 *
 *  Created on: Jun 8, 2011
 *      Author: demon
 */

#ifndef COORDINATE_READER_H_
#define COORDINATE_READER_H_

#include <stdlib.h>
#include <json-c/json.h>

#include "../main.h"

void readGoogleWorldCoordinate(json_object * jobj, WorldCoordinate *wc);
void readCloudmadeWorldCoordinate(json_object * jobj, WorldCoordinate *wc);

#endif /* COORDINATE_READER_H_ */
