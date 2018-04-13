/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2018 (c) Ari Breitkreuz, fortiss GmbH
 */

#ifndef UA_SUBSCRIPTION_EVENTS_H_
#define UA_SUBSCRIPTION_EVENTS_H_

#ifdef UA_ENABLE_EVENTS

#define UA_NS0ID_SIMPLEOVERFLOWEVENTTYPE 4035

#ifdef UA_DEBUG_EVENTS
void UA_Event_generateExampleEvent(UA_Server *server);
#endif

#endif

#endif /* UA_SUBSCRIPTION_EVENTS_H_ */
