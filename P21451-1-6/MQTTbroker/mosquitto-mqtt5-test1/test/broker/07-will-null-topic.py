#!/usr/bin/env python

import struct

from mosq_test_helper import *

rc = 1
keepalive = 60
connect_packet = mosq_test.gen_connect("will-null-topic", keepalive=keepalive, will_topic="", will_payload=struct.pack("!4sB7s", "will", 0, "message"))
connack_packet = mosq_test.gen_connack(rc=2)

port = mosq_test.get_port()
broker = mosq_test.start_broker(filename=os.path.basename(__file__), port=port)

try:
    sock = mosq_test.do_client_connect(connect_packet, "", timeout=30, port=port)
    rc = 0
    sock.close()
finally:
    broker.terminate()
    broker.wait()
    (stdo, stde) = broker.communicate()
    if rc:
        print(stde)

exit(rc)

