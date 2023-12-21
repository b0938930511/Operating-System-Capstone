#!/usr/bin/python3
import os
import time
import serial#pyserial

def waitFor(target,dev,display=True):#avoid loss data
    msgs=''
    while True:
        cnt=dev.inWaiting()
        if cnt>0:
            msg=dev.read(cnt).decode()
            msgs=msgs+msg
            if display:
                print(msg,end='')
            if target == '.':
                if msgs.find(target)!=-1:
                    return
            else:
                return

def dumpImg(dev,k_addr='0x80000',kernel='kernel8.img'):
    dev.write(str.encode("[Load Kernel]\n"))
    
    k_size=os.stat(kernel).st_size
    
    time.sleep(1)
    waitFor('',dev)
    dev.write(str.encode(k_addr + '\n'))

    time.sleep(1)
    waitFor('',dev)
    dev.write(str.encode(str(k_size) + '\n'))

    with open(kernel,"rb") as f:
        time.sleep(1)
        waitFor('',dev)
        for i in range(k_size):
            dev.write(f.read(1))
            waitFor('.',dev,False)

if __name__=='__main__':
    br=115200
    #dev=serial.Serial("/dev/ttyUSB0",br)
    dev=serial.Serial("/dev/pts/2",br)
    dumpImg(dev)
