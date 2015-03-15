
# http://habrahabr.ru/post/155201/

CC = g++

CFLAGS = -c -Wall -pedantic -std=c++11 -I/usr/include/opencv2 -I/usr/local/include/boost
LFLAGS = -L/usr/local/lib/boost/dynamic/ -lboost_system-gcc47-mt-1_56 -lopencv_highgui -lopencv_core -lopencv_imgproc



all: net_srv net_cln

net_srv: net_srv.o
	$(CC) $(LFLAGS) net_srv.o -o net_srv

net_cln: net_cln.o
	$(CC) $(LFLAGS) net_cln.o -o net_cln

net_srv.o: net_srv.cpp network_header.hpp
	$(CC) $(CFLAGS) net_srv.cpp

net_cln.o: net_cln.cpp network_header.hpp
	$(CC) $(CFLAGS) net_cln.cpp


clean:
	rm -rf *.o
