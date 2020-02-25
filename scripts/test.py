# import pyproj
import math
import random
import sqlite3

# _GEOD = pyproj.Geod(ellps='WGS84')

class Vector:
	def __init__(self):
		self.x = 0.0
		self.y = 0.0

	def module(self):
		d = self.x * self.x + self.y * self.y
		return math.sqrt(d)
	
	def normalize(self):
		d = self.module()
		self.x = self.x / d
		self.y = self.y / d
	
	def __repr__(self):
		return "x: " + str(self.x) + " y: " + str(self.y)
	
	def __str__(self):
		return "x: " + str(self.x) + " y: " + str(self.y)

# returns minimal angle between 2 vectors
def get_angle(v0, v1):
	adotb = v0.x * v1.x + v0.y * v1.y
	anorm = v0.module()
	bnorm = v1.module()
	return math.degrees(math.acos(adotb / (anorm * bnorm)))

# returns counterclockwise angle between 2 vectors
def get_angle_cc(v0, v1):
	a = v0.x * v1.x + v0.y * v1.y
	b = v0.x * v1.y - v0.y * v1.x
	t = math.atan2(b, a)
	t = math.degrees(t)
	if t < 0:
		t += 360.0
	return t

def get_random_direction(v0):
	d = Vector()
	done = False
	while not done:
		d.x = random.uniform(0, 1)
		d.y = random.uniform(0, 1)
		d.normalize()
		angle = get_angle(d, v0)
		done = (angle < 90)
	return d

def get_new_point_in_radius(lon, lat, radius):
	radiusInDegrees = radius / 111000.0
	u = random.uniform(0,1)
	v = random.uniform(0,1)
	w = radiusInDegrees * math.sqrt(u)
	t = 2.0 * math.pi * v
	x = w * math.cos(t)
	y = w * math.sin(t)
	new_x = x / math.cos(math.radians(lat))
	new_lon = new_x + lon
	new_lat = y + lat
	return [new_lat, new_lon]

# Positive y represents North, negative South

# 1- Initialize stuff
# Start pointing North
currDir = Vector()
currDir.x = 0.0
currDir.y = 1.0
# Start at a fixed GPS location to serve as base for our movements
currGPS = Vector()
currGPS.x = -22.817092
currGPS.y = -47.092430

ori = 0.0
step = 0.5 # the amount of meters to dislocate at every point
num = 10000 # amount of points to generate

with open("output.txt", "w") as fd:
	conn = sqlite3.connect("database.db")
	cursor = conn.cursor()
	fd.write("Longitude, Latitude\n")
	# cursor.execute("DELETE FROM Felipe")
	# conn.commit()
	for i in range(0, num):
		data = get_new_point_in_radius(currGPS.y, currGPS.x, 20)
		line = str(currGPS.y) + "," + str(currGPS.x) + "\n"
		fd.write(line)
		cursor.execute("INSERT INTO Felipe (Latitude, Longitude, AppMask) VALUES (" + str(currGPS.x) + ", " + str(currGPS.y) + ", 1 );")
		currGPS.x = data[0]
		currGPS.y = data[1]
	conn.commit()


#ss = get_angle_cc(v0, v1)
#print("Angle ", ss)

#slat = -22.817092
#slon = -47.092430
#elat = -22.817089
#elon = -47.092432

#d, a, a2 = dist(slat, slon, elat, elon)
#print(d)
#print(a)
#print(a2)

#a = 5.0
#data = _GEOD.fwd(slon, slat, a, d)
#print(data)

