#!/usr/bin/python
#-*- coding: utf-8 -*-



import numpy as np
import cv2
import os,\
	   sys



def PrintHelp():
	print ("Parameters: -h - print help; -f file_name - read video from file; "
		   "no parameters - read from first camera; --wf file_name - write to file")


def ReadFromFile(file_name):
	print ("Playing from the file: {}".format(file_name))
	
	cap = cv2.VideoCapture(file_name)

	while(cap.isOpened()):
		ret, frame = cap.read()

		#gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
		if frame is None:
			break
		cv2.imshow('frame', frame)
		if cv2.waitKey(50) & 0xFF == ord('q'):
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
		cv2.imshow('Video', frame)
		if cv2.waitKey(1) & 0xFF == ord('q'):
			break

	# When everything done, release the capture
	cap.release()
	cv2.destroyAllWindows()


def WriteToFile(file_name):
	print ("Writing to: {}".format (file_name))
	
	cap = cv2.VideoCapture(0)

	# Define the codec and create VideoWriter object
	#fourcc = cv2.VideoWriter_fourcc(*'XVID')
	fourcc = cv2.cv.CV_FOURCC(*'XVID')
	out = cv2.VideoWriter(file_name,fourcc, 20.0, (640,480))

	while(cap.isOpened()):
		ret, frame = cap.read()
		if ret==True:
			# write the flipped frame
			out.write(frame)

			cv2.imshow('frame',frame)
			if cv2.waitKey(1) & 0xFF == ord('q'):
				break
		else:
			break

	# Release everything if job is finished
	cap.release()
	out.release()
	cv2.destroyAllWindows()
	
	return


def ParseParameters (args):
	if type(args) != type([]):
		raise TypeError ("Excpected list")
	
	if args[0] == '-h' or len(sys.argv) < 2:
		PrintHelp()
	elif args[0] == '-f':
		ReadFromFile(args[1])
	elif args[0] == '--wf':
		WriteToFile(args[1])
	else:
		ReadFromFirstCamera()


def main ():
	if (len (sys.argv) > 1):
		ParseParameters (sys.argv[1:])
	else:
		ReadFromFirstCamera()



if __name__ == "__main__":
	main ()
else:
	pass
	# print ("Importing like a module")
