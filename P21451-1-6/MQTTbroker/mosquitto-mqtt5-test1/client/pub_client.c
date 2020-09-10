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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <time.h>
#else
#include <process.h>
#include <winsock2.h>
#define snprintf sprintf_s
#endif

#include <mosquitto.h>
#include "client_shared.h"
#include "pub_shared.h"

/* Global variables for use in callbacks. See sub_client.c for an example of
 * using a struct to hold variables for use in callbacks. */
static bool first_publish = true;

int my_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, void *payload, int qos, bool retain)
{
	if(cfg.protocol_version == MQTT_PROTOCOL_V5 && cfg.have_topic_alias && first_publish == false){
		return mosquitto_publish_v5(mosq, mid, NULL, payloadlen, payload, qos, retain, cfg.publish_props);
	}else{
		first_publish = false;
		return mosquitto_publish_v5(mosq, mid, topic, payloadlen, payload, qos, retain, cfg.publish_props);
	}
}


void my_connect_callback(struct mosquitto *mosq, void *obj, int result, int flags, const mosquitto_property *properties)
{
	int rc = MOSQ_ERR_SUCCESS;

	if(!result){
		switch(cfg.pub_mode){
			case MSGMODE_CMD:
			case MSGMODE_FILE:
			case MSGMODE_STDIN_FILE:
				rc = my_publish(mosq, &mid_sent, cfg.topic, cfg.msglen, cfg.message, cfg.qos, cfg.retain);
				break;
			case MSGMODE_NULL:
				rc = my_publish(mosq, &mid_sent, cfg.topic, 0, NULL, cfg.qos, cfg.retain);
				break;
			case MSGMODE_STDIN_LINE:
				status = STATUS_CONNACK_RECVD;
				break;
		}
		if(rc){
			if(!cfg.quiet){
				switch(rc){
					case MOSQ_ERR_INVAL:
						fprintf(stderr, "Error: Invalid input. Does your topic contain '+' or '#'?\n");
						break;
					case MOSQ_ERR_NOMEM:
						fprintf(stderr, "Error: Out of memory when trying to publish message.\n");
						break;
					case MOSQ_ERR_NO_CONN:
						fprintf(stderr, "Error: Client not connected when trying to publish.\n");
						break;
					case MOSQ_ERR_PROTOCOL:
						fprintf(stderr, "Error: Protocol error when communicating with broker.\n");
						break;
					case MOSQ_ERR_PAYLOAD_SIZE:
						fprintf(stderr, "Error: Message payload is too large.\n");
						break;
					case MOSQ_ERR_QOS_NOT_SUPPORTED:
						fprintf(stderr, "Error: Message QoS not supported on broker, try a lower QoS.\n");
						break;
				}
			}
			mosquitto_disconnect_v5(mosq, 0, cfg.disconnect_props);
		}
	}else{
		if(result && !cfg.quiet){
			if(cfg.protocol_version == MQTT_PROTOCOL_V5){
				fprintf(stderr, "%s\n", mosquitto_reason_string(result));
			}else{
				fprintf(stderr, "%s\n", mosquitto_connack_string(result));
			}
		}
	}
}


void print_usage(void)
{
	int major, minor, revision;

	mosquitto_lib_version(&major, &minor, &revision);
	printf("mosquitto_pub is a simple mqtt client that will publish a message on a single topic and exit.\n");
	printf("mosquitto_pub version %s running on libmosquitto %d.%d.%d.\n\n", VERSION, major, minor, revision);
	printf("Usage: mosquitto_pub {[-h host] [-p port] [-u username [-P password]] -t topic | -L URL}\n");
	printf("                     {-f file | -l | -n | -m message}\n");
	printf("                     [-c] [-k keepalive] [-q qos] [-r]\n");
#ifdef WITH_SRV
	printf("                     [-A bind_address] [-S]\n");
#else
	printf("                     [-A bind_address]\n");
#endif
	printf("                     [-i id] [-I id_prefix]\n");
	printf("                     [-d] [--quiet]\n");
	printf("                     [-M max_inflight]\n");
	printf("                     [-u username [-P password]]\n");
	printf("                     [--will-topic [--will-payload payload] [--will-qos qos] [--will-retain]]\n");
#ifdef WITH_TLS
	printf("                     [{--cafile file | --capath dir} [--cert file] [--key file]\n");
	printf("                      [--ciphers ciphers] [--insecure]]\n");
#ifdef FINAL_WITH_TLS_PSK
	printf("                     [--psk hex-key --psk-identity identity [--ciphers ciphers]]\n");
#endif
#endif
#ifdef WITH_SOCKS
	printf("                     [--proxy socks-url]\n");
#endif
	printf("                     [--property command identifier value]\n");
	printf("                     [-D command identifier value]\n");
	printf("       mosquitto_pub --help\n\n");
	printf(" -A : bind the outgoing socket to this host/ip address. Use to control which interface\n");
	printf("      the client communicates over.\n");
	printf(" -d : enable debug messages.\n");
	printf(" -D : Define MQTT v5 properties. See the documentation for more details.\n");
	printf(" -f : send the contents of a file as the message.\n");
	printf(" -h : mqtt host to connect to. Defaults to localhost.\n");
	printf(" -i : id to use for this client. Defaults to mosquitto_pub_ appended with the process id.\n");
	printf(" -I : define the client id as id_prefix appended with the process id. Useful for when the\n");
	printf("      broker is using the clientid_prefixes option.\n");
	printf(" -k : keep alive in seconds for this client. Defaults to 60.\n");
	printf(" -L : specify user, password, hostname, port and topic as a URL in the form:\n");
	printf("      mqtt(s)://[username[:password]@]host[:port]/topic\n");
	printf(" -l : read messages from stdin, sending a separate message for each line.\n");
	printf(" -m : message payload to send.\n");
	printf(" -M : the maximum inflight messages for QoS 1/2..\n");
	printf(" -n : send a null (zero length) message.\n");
	printf(" -p : network port to connect to. Defaults to 1883 for plain MQTT and 8883 for MQTT over TLS.\n");
	printf(" -P : provide a password\n");
	printf(" -q : quality of service level to use for all messages. Defaults to 0.\n");
	printf(" -r : message should be retained.\n");
	printf(" -s : read message from stdin, sending the entire input as a message.\n");
#ifdef WITH_SRV
	printf(" -S : use SRV lookups to determine which host to connect to.\n");
#endif
	printf(" -t : mqtt topic to publish to.\n");
	printf(" -u : provide a username\n");
	printf(" -V : specify the version of the MQTT protocol to use when connecting.\n");
	printf("      Can be mqttv5, mqttv311 or mqttv31. Defaults to mqttv311.\n");
	printf(" --help : display this message.\n");
	printf(" --quiet : don't print error messages.\n");
	printf(" --will-payload : payload for the client Will, which is sent by the broker in case of\n");
	printf("                  unexpected disconnection. If not given and will-topic is set, a zero\n");
	printf("                  length message will be sent.\n");
	printf(" --will-qos : QoS level for the client Will.\n");
	printf(" --will-retain : if given, make the client Will retained.\n");
	printf(" --will-topic : the topic on which to publish the client Will.\n");
#ifdef WITH_TLS
	printf(" --cafile : path to a file containing trusted CA certificates to enable encrypted\n");
	printf("            communication.\n");
	printf(" --capath : path to a directory containing trusted CA certificates to enable encrypted\n");
	printf("            communication.\n");
	printf(" --cert : client certificate for authentication, if required by server.\n");
	printf(" --key : client private key for authentication, if required by server.\n");
	printf(" --ciphers : openssl compatible list of TLS ciphers to support.\n");
	printf(" --tls-version : TLS protocol version, can be one of tlsv1.2 tlsv1.1 or tlsv1.\n");
	printf("                 Defaults to tlsv1.2 if available.\n");
	printf(" --insecure : do not check that the server certificate hostname matches the remote\n");
	printf("              hostname. Using this option means that you cannot be sure that the\n");
	printf("              remote host is the server you wish to connect to and so is insecure.\n");
	printf("              Do not use this option in a production environment.\n");
#  ifdef FINAL_WITH_TLS_PSK
	printf(" --psk : pre-shared-key in hexadecimal (no leading 0x) to enable TLS-PSK mode.\n");
	printf(" --psk-identity : client identity string for TLS-PSK mode.\n");
#  endif
#endif
#ifdef WITH_SOCKS
	printf(" --proxy : SOCKS5 proxy URL of the form:\n");
	printf("           socks5h://[username[:password]@]hostname[:port]\n");
	printf("           Only \"none\" and \"username\" authentication is supported.\n");
#endif
	printf("\nSee https://mosquitto.org/ for more information.\n\n");
}

int main(int argc, char *argv[])
{
	struct mosquitto *mosq = NULL;
	int rc;

	mosquitto_lib_init();

	if(pub_shared_init()) return 1;

	memset(&cfg, 0, sizeof(struct mosq_config));
	rc = client_config_load(&cfg, CLIENT_PUB, argc, argv);
	if(rc){
		if(rc == 2){
			/* --help */
			print_usage();
		}else{
			fprintf(stderr, "\nUse 'mosquitto_pub --help' to see usage.\n");
		}
		goto cleanup;
	}

#ifndef WITH_THREADING
	if(cfg.pub_mode == MSGMODE_STDIN_LINE){
		fprintf(stderr, "Error: '-l' mode not available, threading support has not been compiled in.\n");
		goto cleanup;
	}
#endif

	if(cfg.pub_mode == MSGMODE_STDIN_FILE){
		if(load_stdin()){
			fprintf(stderr, "Error loading input from stdin.\n");
			goto cleanup;
		}
	}else if(cfg.file_input){
		if(load_file(cfg.file_input)){
			fprintf(stderr, "Error loading input file \"%s\".\n", cfg.file_input);
			goto cleanup;
		}
	}

	if(!cfg.topic || cfg.pub_mode == MSGMODE_NONE){
		fprintf(stderr, "Error: Both topic and message must be supplied.\n");
		print_usage();
		goto cleanup;
	}


	if(client_id_generate(&cfg, "mosqpub")){
		goto cleanup;
	}

	mosq = mosquitto_new(cfg.id, true, NULL);
	if(!mosq){
		switch(errno){
			case ENOMEM:
				if(!cfg.quiet) fprintf(stderr, "Error: Out of memory.\n");
				break;
			case EINVAL:
				if(!cfg.quiet) fprintf(stderr, "Error: Invalid id.\n");
				break;
		}
		goto cleanup;
	}
	if(cfg.debug){
		mosquitto_log_callback_set(mosq, my_log_callback);
	}
	mosquitto_connect_v5_callback_set(mosq, my_connect_callback);
	mosquitto_disconnect_v5_callback_set(mosq, my_disconnect_callback);
	mosquitto_publish_v5_callback_set(mosq, my_publish_callback);

	if(client_opts_set(mosq, &cfg)){
		goto cleanup;
	}

	rc = client_connect(mosq, &cfg);
	if(rc){
		goto cleanup;
	}

	rc = pub_shared_loop(mosq);

	if(cfg.message && cfg.pub_mode == MSGMODE_FILE){
		free(cfg.message);
	}
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	client_config_cleanup(&cfg);
	pub_shared_cleanup();

	if(rc){
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
	}
	return rc;

cleanup:
	mosquitto_lib_cleanup();
	client_config_cleanup(&cfg);
	pub_shared_cleanup();
	return 1;
}
