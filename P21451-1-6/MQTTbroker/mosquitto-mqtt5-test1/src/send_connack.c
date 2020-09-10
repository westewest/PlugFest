/*
Copyright (c) 2009-2018 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

Contributors:
   Roger Light - initial implementation and documentation.
*/

#include "config.h"

#include "mosquitto_broker_internal.h"
#include "mqtt_protocol.h"
#include "memory_mosq.h"
#include "packet_mosq.h"
#include "property_mosq.h"
#include "util_mosq.h"

int send__connack(struct mosquitto_db *db, struct mosquitto *context, int ack, int reason_code, const mosquitto_property *properties)
{
	struct mosquitto__packet *packet = NULL;
	int rc;
	mosquitto_property *connack_props = NULL;
	int proplen, varbytes;

	rc = mosquitto_property_copy_all(&connack_props, properties);
	if(rc){
		return rc;
	}

	if(context){
		if(context->id){
			log__printf(NULL, MOSQ_LOG_DEBUG, "Sending CONNACK to %s (%d, %d)", context->id, ack, reason_code);
		}else{
			log__printf(NULL, MOSQ_LOG_DEBUG, "Sending CONNACK to %s (%d, %d)", context->address, ack, reason_code);
		}
	}

	packet = mosquitto__calloc(1, sizeof(struct mosquitto__packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->command = CMD_CONNACK;
	packet->remaining_length = 2;
	if(context->protocol == mosq_p_mqtt5){
		if(reason_code < 128 && db->config->retain_available == false){
			rc = mosquitto_property_add_byte(&connack_props, MQTT_PROP_RETAIN_AVAILABLE, 0);
			if(rc){
				mosquitto__free(packet);
				return rc;
			}
		}
		/* FIXME - disable support until available */
		rc = mosquitto_property_add_byte(&connack_props, MQTT_PROP_SHARED_SUB_AVAILABLE, 0);
		if(rc){
			mosquitto__free(packet);
			return rc;
		}

		proplen = property__get_length_all(connack_props);
		varbytes = packet__varint_bytes(proplen);
		packet->remaining_length += proplen + varbytes;
	}
	rc = packet__alloc(packet);
	if(rc){
		mosquitto__free(packet);
		return rc;
	}
	packet__write_byte(packet, ack);
	packet__write_byte(packet, reason_code);
	if(context->protocol == mosq_p_mqtt5){
		property__write_all(packet, connack_props, true);
	}
	mosquitto_property_free_all(&connack_props);

	return packet__queue(context, packet);
}

