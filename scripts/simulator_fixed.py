import time
import sys

def usec_to_sec(x):
    return (x / 1000000.0)

if len(sys.argv) < 2:
    print "Usage: python ", sys.argv[0], " <GPS_CSV_FILE>"
    quit()

path = sys.argv[1]
points = None
with open(path, "r") as fd:
    data = fd.read()
    points = data.split('\n')

if len(points) > 0:
    i = 0;
    size = len(points)
    while True:
        with open("../gps_test.txt", "a+") as fd:
            print("Adding ", points[i])
            fd.write(points[i] + "\n")
            fd.close()
        i = i + 1
        if i > size - 1:
            i = 0
        time.sleep(usec_to_sec(100000))
