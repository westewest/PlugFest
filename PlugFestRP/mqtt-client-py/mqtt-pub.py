#! /usr/bin/python
# -*- coding: utf-8 -*-
# ------------------------------------------------------------------
import  sys
from time import sleep
import paho.mqtt.client as mqtt

# ------------------------------------------------------------------

sys.stderr.write("*** START ***\n")

host = '192.168.8.101'
port = 1883
topic = 'topic_1'

client = mqtt.Client(protocol=mqtt.MQTTv311)

client.connect(host, port=port, keepalive=60)

client.publish(topic, 'Good Afternoon')
sleep(0.5)
client.publish(topic, 'こんにちは')

client.disconnect()

sys.stderr.write("*** END ***\n")
# ------------------------------------------------------------------