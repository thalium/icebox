import struct
import time
import sys
import timeit

from PyFDP.FDP import *
from threading import Timer

def timeout():
    global timerOut
    timerOut = True

if __name__ == '__main__':

    timerOut = False
    singleStepCount = 0;
    timerDelay = 3

    fdp = FDP("Win10_working")

    fdp.Pause()

    print("Let's go !")
    t = Timer(timerDelay, timeout)
    t.start()

    while timerOut == False :
        fdp.SingleStep()
        singleStepCount += 1

    singleStepCountPerSecond = singleStepCount / timerDelay
    print("SingleStepCountPerSecond : " + str(singleStepCountPerSecond))

    fdp.Resume()
