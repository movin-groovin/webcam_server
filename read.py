#!/usr/bin/python
#-*- coding: utf-8 -*-



import numpy as np
import cv2
import os,\
	   sys



def PrintHelp():
	print ("Parameters: -h - print help; -f file_name - read video from file; no parameters - read from first camera")


def ReadFromFile(file_name):
	print ("Playing from the file: {}".format(file_name))
	
	cap = cv2.VideoCapture(file_name)

	while(cap.isOpened()):
		ret, frame = cap.read()

		#gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

		cv2.imshow('frame', frame)
		if cv2.waitKey(1) & 0xFF == ord('q'):
			break

	cap.release()
	cv2.destroyAllWindows()
	return
	
	
def ReadFromFirstCamera():
	cap = cv2.VideoCapture(0)
	while(True):
		# Capture frame-by-frame
		ret, frame = cap.read()

		# Our operations on the frame come here
		gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
		#print 1
		# Display the resulting frame
		cv2.imshow('frame', frame)
		if cv2.waitKey(1) & 0xFF == ord('q'):
			break

	# When everything done, release the capture
	cap.release()
	cv2.destroyAllWindows()


def ParseParameters (args):
	if type(args) != type([]):
		raise TypeError ("Excpected list")
	
	if args[0] == '-h':
		PrintHelp()
	elif args[1] == '-f':
		ReadFromFile(args[2])
	else:
		ReadFromFirstCamera()


def main ():
	if (len (sys.argv) > 2):
		ParseParameters (sys.argv[1:])
	else:
		ReadFromFirstCamera()



if __name__ == "__main__":
	main ()
else:
	pass
	# print ("Importing like a module")
