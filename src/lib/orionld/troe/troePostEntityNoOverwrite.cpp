/*
*
* Copyright 2020 FIWARE Foundation e.V.
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
#include <postgresql/libpq-fe.h>                               // PGconn

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjRender.h"                                    // kjRender
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/troe/pgConnectionGet.h"                      // pgConnectionGet
#include "orionld/troe/pgConnectionRelease.h"                  // pgConnectionRelease
#include "orionld/troe/pgTransactionBegin.h"                   // pgTransactionBegin
#include "orionld/troe/pgTransactionRollback.h"                // pgTransactionRollback
#include "orionld/troe/pgTransactionCommit.h"                  // pgTransactionCommit
#include "orionld/troe/pgEntityTreat.h"                        // pgEntityTreat
#include "orionld/troe/troePostEntityNoOverwrite.h"            // Own interface



// ----------------------------------------------------------------------------
//
// troePostEntityNoOverwrite -
//
bool troePostEntityNoOverwrite(ConnectionInfo* ciP)
{
  PGconn* connectionP = pgConnectionGet(orionldState.troeDbName);
  if (connectionP == NULL)
    LM_RE(false, ("no connection to postgres"));

  if (pgTransactionBegin(connectionP) != true)
  {
    pgConnectionRelease(connectionP);
    LM_RE(false, ("pgTransactionBegin failed"));
  }

  char* entityId   = orionldState.wildcard[0];
  char* entityType = (char*) "APPEND";

  if (pgEntityTreat(connectionP, orionldState.requestTree, entityId, entityType, TROE_ENTITY_UPDATE, TROE_ATTRIBUTE_APPEND) == false)
  {
    if (pgTransactionRollback(connectionP) == false)
      LM_E(("Database Error (pgTransactionRollback failed too)"));

    pgConnectionRelease(connectionP);
    LM_RE(false, ("Database Error (post entities TRoE layer failed)"));
  }

  if (pgTransactionCommit(connectionP) != true)
  {
    pgConnectionRelease(connectionP);
    LM_RE(false, ("pgTransactionCommit failed"));
  }

  pgConnectionRelease(connectionP);

  return true;
}
