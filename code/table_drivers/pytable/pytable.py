import sys
import numpy
import serial
from struct import *

class Table():

    def __init__(self, width=16, height=8, device='/dev/ttyUSB0', baud=500000):

        self.WIDTH = width
        self.HEIGHT = height

        # initialize the table
        self.data = numpy.zeros(width*height*3).reshape(width, height, 3) 

        # init output array
        self.output = range(width*height*3)

        self.low = (self.WIDTH * self.HEIGHT) >> 8
        self.high = (self.WIDTH * self.HEIGHT) & 0xff
        self.chk = self.low ^ self.high ^ 0x55

        # initialize serial port
        self.s = serial.Serial(device, baud)

    def xy_to_flat(self,n, x):
        return x*((x/n+1)%2)+(x+(n-2*(x-n*(x/n))-1))*((x/n)%2);

    def set_led(self, x, y, r, g, b):

        loc = self.xy_to_flat(self.WIDTH, y)
        self.output[loc*3] = pack('B',r)
        self.output[loc*3+1] = pack('B',g)
        self.output[loc*3+2] = pack('B',b)

    def get(self, x, y):

        r = self.data[x][y][0]
        g = self.data[x][y][1]
        b = self.data[x][y][2]

        return [r, g, b]

    def set(self, x, y, color):

        self.data[x][y] = color

    def set_all(self, color):

        for x in range(self.WIDTH):
            for y in range(self.HEIGHT):
                self.data[x][y] = color

    def send(self):

        # Start of sequence
        self.s.write("Ada")
        self.s.write(pack('BBB',self.low,self.high,self.chk))

        i = 0
        for y in range(self.HEIGHT):
            for x in range(self.WIDTH):
                self.set_led(x, i, self.data[x][y][0], self.data[x][y][1], self.data[x][y][2])
                i = i + 1

        for byte in self.output:
            self.s.write(byte)


table = Table()
table.set_all([0,0,254])
table.send()
