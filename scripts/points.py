import sys

points = []
with open("processed_primeiro.txt", "r") as fd:
	data = fd.read()
	points = data.split('\n')

if len(points) > 0:
	i = 0
	size = len(points)
	with open("out_primeiro.txt", "w+") as fd:
		while True:
			el = points[i]
			rl = el.split(',')
			if len(rl) >= 2:
				lat = rl[0]
				lon = rl[1]
				str = 'dataPtr->push_back(QGeoCoordinate('
				str += lat
				str += ","
				str += lon
				str += "));\n"
				fd.write(str)
			i = i + 1
			if i > size - 1:
				break
print "DONE"