# -*- coding: utf-8 -*- 
from btle import Peripheral

from RN42 import RN42
from time import sleep

import struct 
import btle
import binascii
 
import RPi.GPIO as GPIO
import sys
import time

def s16(value):
    return -(value & 0b1000000000000000) | (value & 0b0111111111111111)


class NtfyDelegate(btle.DefaultDelegate):
    GeoMagnetic_X = 0;
    GeoMagnetic_Y = 0;
    GeoMagnetic_Z = 0;
    Acceleration_X = 0;
    Acceleration_Y = 0;
    Acceleration_Z = 0;
    Pressure = 0;
    Humidity = 0;
    Temperature = 0;
    UV = 0;
    AmbientLight = 0;
    def __init__(self, params):
        btle.DefaultDelegate.__init__(self)
        # ... initialise here
 
    def handleNotification(self, cHandle, data): 
        # ... perhaps check cHandle
        # ... process 'data'
        cal = binascii.b2a_hex(data)
        #print u'handleNotification : {0}-{1}:'.format(cHandle, cal)
         
        if int((cal[0:2]), 16) == 0xf2:
            NtfyDelegate.GeoMagnetic_X = s16(int((cal[6:8] + cal[4:6]), 16)) * 0.15
            NtfyDelegate.GeoMagnetic_Y = s16(int((cal[10:12] + cal[8:10]), 16)) * 0.15
            NtfyDelegate.GeoMagnetic_Z = s16(int((cal[14:16] + cal[12:14]), 16)) * 0.15
            print 'Geo-Magnetic X:{0:.3f} Y:{1:.3f} Z:{2:.3f}'.format(NtfyDelegate.GeoMagnetic_X, NtfyDelegate.GeoMagnetic_Y, NtfyDelegate.GeoMagnetic_Z)
            NtfyDelegate.Acceleration_X = 1.0 * s16(int((cal[18:20] + cal[16:18]), 16)) / 1024
            NtfyDelegate.Acceleration_Y = 1.0 * s16(int((cal[22:24] + cal[20:22]), 16)) / 1024
            NtfyDelegate.Acceleration_Z = 1.0 * s16(int((cal[26:28] + cal[24:26]), 16)) / 1024
            print 'Acceleration X:{0:.3f} Y:{1:.3f} Z:{2:.3f}'.format(NtfyDelegate.Acceleration_X, NtfyDelegate.Acceleration_Y, NtfyDelegate.Acceleration_Z)

        if int((cal[0:2]), 16) == 0xf3:
            NtfyDelegate.Pressure = int((cal[6:8] + cal[4:6]), 16) * 860.0/65535 + 250
            NtfyDelegate.Humidity = 1.0 * (int((cal[10:12] + cal[8:10]), 16) - 896 )/64
            NtfyDelegate.Temperature = 1.0*((int((cal[14:16] + cal[12:14]), 16) -2096)/50)
            UV = int((cal[18:20] + cal[16:18]), 16) / (100*0.388)
            NtfyDelegate.AmbientLight = int((cal[22:24] + cal[20:22]), 16) / (0.05*0.928)
            print 'Pressure:{0:.3f} Humidity:{1:.3f} Temperature:{2:.3f} UV:{0:.3f} AmbientLight:{1:.3f} '.format(NtfyDelegate.Pressure, NtfyDelegate.Humidity, NtfyDelegate.Temperature, NtfyDelegate.AmbientLight)
#            print 'UV:{0:.3f} AmbientLight:{1:.3f} '.format(UV, AmbientLight)
		
 

 
class AlpsSensor(Peripheral):
    def __init__(self,addr):
        Peripheral.__init__(self,addr)
        self.result = 1
 
def main():
    print "Turn On Sensor Node"
    alps = AlpsSensor("48:F0:7B:78:47:33")
    alps.setDelegate( NtfyDelegate(btle.DefaultDelegate) )
 
    #Hybrid MAG ACC8G　100ms　/ Other 1s
    alps.writeCharacteristic(0x0013, struct.pack('<bb', 0x01, 0x00), True)# Custom1 Notify Enable 
    alps.writeCharacteristic(0x0016, struct.pack('<bb', 0x01, 0x00), True)# Custom2 Notify Enable
    alps.writeCharacteristic(0x0018, struct.pack('<bbb', 0x2F, 0x03, 0x03), True)# (不揮発)保存内容の初期化
    alps.writeCharacteristic(0x0018, struct.pack('<bbb', 0x01, 0x03, 0x7F), True)# 地磁気、加速度,気圧,温度,湿度,UV,照度を有効
#    alps.writeCharacteristic(0x0018, struct.pack('<bbb', 0x04, 0x03, 0x04), True)# Hybrid Mode
    alps.writeCharacteristic(0x0018, struct.pack('<bbb', 0x04, 0x03, 0x00), True)# Hybrid Mode
#    alps.writeCharacteristic(0x0018, struct.pack('<bbbb', 0x06, 0x04, 0x64, 0x00), True) # Fast 100msec (地磁気,加速度)
#    alps.writeCharacteristic(0x0018, struct.pack('<bbbb', 0x06, 0x04, 0x7A, 0x01), True) # Fast 250msec (地磁気,加速度) Hybrid mode だけ(deleted)
    alps.writeCharacteristic(0x0018, struct.pack('<bbbb', 0x05, 0x04, 0x01, 0x00), True) # Slow 1sec (気圧,温度,湿度,UV,照度)     

    alps.writeCharacteristic(0x0018, struct.pack('<bbb', 0x02, 0x03, 0x02), True) # 加速度±8G
    alps.writeCharacteristic(0x0018, struct.pack('<bbb', 0x2F, 0x03, 0x01), True)# 設定内容保存
    alps.writeCharacteristic(0x0018, struct.pack('<bbb', 0x20, 0x03, 0x01), True)# センサ計測開始

    ## Setting & Connect
    ras = RN42("ras", "B8:27:EB:9A:A4:C7", 1)
    ras.connectBluetooth(ras.bdAddr,ras.port)
     
# Main loop --------
    while True:
        if alps.waitForNotifications(1.0):
            # handleNotification() was called
            print (NtfyDelegate.Pressure)
            msg = '[uniqID] Pressure:{0:.3f} Humidity:{1:.3f} Temperature:{2:.3f} UV:{0:.3f} AmbientLight:{1:.3f} '.format(NtfyDelegate.Pressure, NtfyDelegate.Humidity, NtfyDelegate.Temperature, NtfyDelegate.AmbientLight)
            ras.sock.send(msg)
            print ("data send")
            continue
 
        print "Waiting..."
        # Perhaps do something else here
if __name__ == "__main__":
    main()
 
