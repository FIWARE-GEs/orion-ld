/*
*
* Copyright 2021 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <stdio.h>                                             // snprintf
#include <postgresql/libpq-fe.h>                               // PGconn

extern "C"
{
#include "kalloc/kaAlloc.h"                                    // kaAlloc
#include "kjson/KjNode.h"                                      // KjNode
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/troe/kjGeoMultiLineStringExtract.h"             // kjGeoMultiLineStringExtract
#include "orionld/troe/pgGeoSubMultiLineStringPush.h"             // Own interface


// -----------------------------------------------------------------------------
//
// pgGeoSubMultiLineStringPush -
//
bool pgGeoSubMultiLineStringPush
(
  PGconn*      connectionP,
  KjNode*      coordinatesP,
  const char*  instanceId,
  const char*  entityId,
  const char*  attrInstanceId,
  const char*  subAttributeName,
  const char*  observedAt
)
{
  char*  coordsString = kaAlloc(&orionldState.kalloc, 10240);

  if (coordsString == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  if (kjGeoMultiLineStringExtract(coordinatesP, coordsString, 10240) == false)
    LM_RE(false, ("unable to extract geo-coordinates from Kj-Tree"));

  int          sqlSize = 12008;
  char*        sql     = kaAlloc(&orionldState.kalloc, sqlSize + 1);
  PGresult*    res;

  if (sql == NULL)
    LM_RE(false, ("Internal Error (out of memory)"));

  //
  // Two combinations for NULL/non-NULL 'observedAt'
  //
  if (observedAt != NULL)
  {
    snprintf(sql, sqlSize, "INSERT INTO subAttributes("
             "instanceId, ts, id, entityId, attrInstanceId, observedAt, valueType, geoMultiLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', 'GeoMultiLineString', ST_GeomFromText('MULTILINESTRING(%s)', 4326))",
             instanceId, orionldState.requestTimeString, subAttributeName, entityId, attrInstanceId, observedAt, coordsString);
  }
  else
  {
    snprintf(sql, sqlSize, "INSERT INTO subAttributes("
             "instanceId, ts, id, entityId, attrInstanceId, valueType, geoMultiLineString) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', 'GeoMultiLineString', ST_GeomFromText('MULTILINESTRING(%s)', 4326))",
             instanceId, orionldState.requestTimeString, subAttributeName, entityId, attrInstanceId, coordsString);
  }

  // LM_TMP(("SQL[%p]: %s;", connectionP, sql));
  res = PQexec(connectionP, sql);
  if (res == NULL)
    LM_RE(false, ("Database Error (%s)", PQresStatus(PQresultStatus(res))));
  PQclear(res);

  if (PQstatus(connectionP) != CONNECTION_OK)
    LM_E(("SQL[%p]: bad connection: %d", connectionP, PQstatus(connectionP)));  // FIXME: string! (last error?)

  return true;
}
