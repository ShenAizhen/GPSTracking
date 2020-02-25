import random
import numpy
import matplotlib.pyplot as plt
import pylab

n = 100000

x = numpy.zeros(n)
y = numpy.zeros(n)
inc = 2

for i in range(1, n):
    val = random.randint(1,4)
    if val == 1:
        x[i] = x[i-1] + inc
        y[i] = y[i-1]
    elif val == 2:
        x[i] = x[i-1] - inc
        y[i] = y[i-1]
    elif val == 3:
        x[i] = x[i-1]
        y[i] = y[i-1] + inc
    else:
        x[i] = x[i-1]
        y[i] = y[i-1] - inc

pylab.title("Random 2D walk")
pylab.plot(x, y)
pylab.show()
