import matplotlib.pyplot as plt
import random
import numpy as np

start = 2
prob = [0.05, 0.95]
positions = [start]

rr = np.random.random(1000)
downp = rr < prob[0]
upp = rr > prob[1]

for idownp, iupp in zip(downp, upp):
    down = idownp and positions[-1] > 1
    up = iupp and positions[-1] < 4
    positions.append(positions[-1] - down + up)

plt.plot(positions)
plt.show()
