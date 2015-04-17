#!/usr/bin/python
#-*- coding:utf-8 -*-

import os
import sys



def checkArgs ():
	#print sys.argv, len (sys.argv)
	if len (sys.argv) < 2:
		print ("Enter a programm name")
		return False
	return True

def main ():
	"""
		All pathes to executables must be absolute (example: '/bin/bash', not 'bash') 
	"""
	if not checkArgs (): return False
	
	try:
		pid = os.fork ()
	except Exception as Exc:
		print "Error of fork"
		print Exc
		return 1

	if pid > 0:
		#parent process
		os._exit (0)
	elif pid == 0:
		# child	process"
		os.setsid ()
		os.umask(0)
		try:
			for i in xrange (1024):
				os.close (i)
		except Exception as e:
			pass
		out_file = open("/dev/null")
		os.dup2(out_file.fileno(), 1)
		os.dup2(out_file.fileno(), 2)
		
		os.chdir ("/")
		
		#print sys.argv[1:], 77777
		os.execv (sys.argv[1], sys.argv[1:])


main ()
