CC=gcc
#OPT=-O3 -s -Wall
OPT=-O2 -s -Wall
#uncomment the following if you have libjpeg
DEFS=-DJPEG -DNODEBUG
#DEFS=

PictureGrabber: PictureGrabber.c
	$(CC) $(OPT) $(DEFS) PictureGrabber.c -ljpeg -o PictureGrabber

clean:  
	rm -f *.o PictureGrabber core
