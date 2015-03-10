#!/usr/bin/python
#-*- coding: utf-8 -*-



import numpy as np
import cv2
import os,\
	   sys,\
	   multiprocessing
	   


def ReadFromFile(file_name):
	print ("Playing from the file: {}".format(file_name))
	
	cap = cv2.VideoCapture(file_name)
	
	while(cap.isOpened()):
		ret, frame = cap.read()
		
		#gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
		if frame is None:
			break
		cv2.imshow('frame', frame)
		if cv2.waitKey(1) & 0xFF == ord('q'):
			break

	cap.release()
	cv2.destroyAllWindows()
	return


def CreateChildAndPipe (file_name):
	os.mkfifo (file_name, 0666)
	
	p = multiprocessing.Process (target = ReadFromFile, args = (file_name, ))
	p.start()
	
	return


def WriteToFile(file_name):
	print ("Writing to: {}".format (file_name))
	p = CreateChildAndPipe (file_name)
	
	cap = cv2.VideoCapture(0)

	# Define the codec and create VideoWriter object
	#fourcc = cv2.VideoWriter_fourcc(*'XVID')
	fourcc = cv2.cv.CV_FOURCC(*'XVID')
	out = cv2.VideoWriter(file_name, fourcc, 20.0, (640,480)) 

	while(cap.isOpened()):
		ret, frame = cap.read()
		if ret==True:
			# write the flipped frame
			out.write(frame)

			#cv2.namedWindow('flexible_window', 0)
			#cv2.imshow('flexible_window',frame)
			#if cv2.waitKey(1) & 0xFF == ord('q'):
			#	break
		else:
			break

	# Release everything if job is finished
	cap.release()
	cv2.destroyAllWindows()
	
	return


if __name__ == '__main__' and len(sys.argv) > 1:
	WriteToFile (sys.argv[1])
else:
	print ("Runs as script or insufficient parameters")
	
	
	
	
	
	
