#!/usr/bin/python2

import signal
import sys
import serial
import popen2
from blessings import Terminal

t = Terminal()

try:
    ser = serial.Serial("/dev/ttyACM0", 9600)
except:
    # don't do this at home kids, use proper error handling!
    ser = serial.Serial("/dev/ttyACM1", 9600)


def listen():
    print 'Listening on ' + ser.port
    while True:
      line = ser.readline()
      if 'PLAYME' in line:
            print line
            print t.on_blue('Searching Youtube for ' + line)
            print t.on_blue('Starting OMXPLAYER...')
            cmd = 'omxplayer -v -o local $(youtube-dl -g "ytsearch1: ' + line.strip() + '")'
            print cmd
            r, w, e = popen2.popen3(cmd)
            print r.readlines()
            print w.readlines()
            print e.readlines()
      else:
            print line

# Start listening on serial port until we crash
listen()

