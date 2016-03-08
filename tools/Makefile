
OCVPREFIX  = /home/jmakoske/src/opencv249$

OPENCVINC = -I$(OCVPREFIX)/include
OPENCVLIB = -L$(OCVPREFIX)/lib -Wl,-rpath,$(OCVPREFIX)/lib \
	   -lopencv_core -lopencv_highgui \
	   -lopencv_objdetect -lopencv_imgproc -lopencv_flann -lopencv_features2d \
           -lopencv_nonfree -lopencv_flann -lopencv_calib3d -lopencv_video -lz

LIBMEDIAINFOINC=`pkg-config libmediainfo --cflags`
LIBMEDIAINFOLIB=`pkg-config libmediainfo --libs`

#

CC = g++

#DEBUGFLAGS = -g -pg
DEBUGFLAGS = -g
FASTFLAGS = -O3
COMMONFLAGS = -Wl,--no-as-needed 
DOTINC = -I.
STDFLAGS = -std=c++0x
ALLFLAGS = -Wall -c $(OPENCVINC) $(STDFLAGS)

CXXFLAGS = $(CFLAGS)

CFLAGS = $(DEBUGFLAGS) $(ALLFLAGS)
#CFLAGS = $(FASTFLAGS) $(ALLFLAGS)

LFLAGS = -Wall $(DEBUGFLAGS) $(COMMONFLAGS)
#LFLAGS = -Wall $(FASTFLAGS) $(COMMONFLAGS)

LDFLAGS = $(OPENCVLIB) $(SVMLIB) $(SPAMSLIB)

all: combine_video get_transform unfish

combine_video: combine_video.o
	$(CC) $(LFLAGS) combine_video.o -o combine_video $(LDFLAGS) $(LIBMEDIAINFOLIB) -lboost_date_time

combine_video.o: combine_video.cpp
	$(CC) $(CFLAGS) $(LIBMEDIAINFOINC) combine_video.cpp

get_transform: get_transform.o
	$(CC) $(LFLAGS) get_transform.o -o get_transform $(LDFLAGS)

get_transform.o: get_transform.cpp
	$(CC) $(CFLAGS) get_transform.cpp

unfish: unfish.o
	$(CC) $(LFLAGS) unfish.o -o unfish $(LDFLAGS) 

unfish.o: unfish.cpp
	$(CC) $(CFLAGS) unfish.cpp

