
# These should be set to appropriate values:

# OpenCV has to be version 2.X
#OCVPREFIX  = /users/markus/opencv2-64
#OCVPREFIX  = /users/markus/opencv64-22
#OCVPREFIX  = /share/imagedb/markus/opencv231-64
#OCVPREFIX  = /users/markus/svn/opencv-svn-64
#OCVPREFIX  = /share/imagedb/mats/sw
#OCVPREFIX  = /share/imagedb/picsom/linux64
OCVPREFIX  = /opt/local

# This is from csoap 1.1.0, available at http://csoap.sourceforge.net/
#CSOAP-CONFIG = /users/markus/libsoap-64/bin/csoap-config-rpath
CSOAP-CONFIG = /share/imagedb/markus/libsoap-64/bin/csoap-config-rpath

# Path to LIBSVM:
LIBSVMDIR = /users/markus/libsvm-3.0

# Comment these out if no LIBSVM:
#SVMINC = -I$(LIBSVMDIR)
#SVMLIB = -L$(LIBSVMDIR) -Wl,-rpath,$(LIBSVMDIR) -lsvm

# Comment these out if no SPAMS:
#SPAMSDIR = /share/imagedb/picsom/linux64/src/spams/spams-matlab
#SPAMSINC = -I$(SPAMSDIR)/dictLearn -I$(SPAMSDIR)/decomp -I$(SPAMSDIR)/linalg
#SPAMSLIB = -lblas -lcblas

#

# These are for OpenCV releases:
OPENCVINC = -I$(OCVPREFIX)/include
OPENCVLIB = -L$(OCVPREFIX)/lib -Wl,-rpath,$(OCVPREFIX)/lib \
	   -lopencv_core -lopencv_highgui \
	   -lopencv_objdetect -lopencv_imgproc -lopencv_flann -lopencv_features2d \
           -lopencv_nonfree -lopencv_flann -lopencv_calib3d -lopencv_video -lz

# These are for OpenCV SVN:
#OPENCVINC = -I$(OCVPREFIX)/include/opencv -I$(OCVPREFIX)/include
#OPENCVLIB = -L$(OCVPREFIX)/lib -lopencv_core -lopencv_features2d \
#	-lopencv_highgui -lopencv_imgproc -lopencv_calib3d

SOAPINC=`$(CSOAP-CONFIG) --cflags`
SOAPLIB=`$(CSOAP-CONFIG) --libs`

OPENMP=
#OPENMP=-fopenmp

#

CC = g++

#DEBUGFLAGS = -g -pg
DEBUGFLAGS = -g
FASTFLAGS = -O3
COMMONFLAGS = #-Wl,--no-as-needed 
DOTINC = -I.
STDFLAGS = -std=c++0x
ALLFLAGS = -Wall -c $(OPENCVINC) $(SVMINC) $(SPAMSINC) $(DOTINC) $(STDFLAGS)

#CFLAGS = $(DEBUGFLAGS) $(ALLFLAGS)
CFLAGS = $(FASTFLAGS) $(ALLFLAGS)

CXXFLAGS = $(CFLAGS)

#LFLAGS = -Wall $(DEBUGFLAGS) $(COMMONFLAGS)
LFLAGS = -Wall $(FASTFLAGS) $(COMMONFLAGS)

LDFLAGS = $(OPENCVLIB) $(SVMLIB) $(SPAMSLIB)

all: od frames summary centrist starter_video odserver odclient \
     facedetect facedetectnew webcam webcam-soap hcluster ifile

# sources for these targets not in CVS 2012-10-02:
# histeq my_find_obj adaptiveskindetector SurfTest testserver testclient lbptest

doc: Doxyfile ObjectDetection.h KeypointFilter.h SMI.h od_defs.h usage.dox
	doxygen Doxyfile

tar: ChangeLog
	tar cfz od-`date -I`.tar.gz od.C ObjectDetection.h ObjectDetection.C \
	KeypointFilter.h KeypointFilter.C Makefile pdf2png.sh webcam-soap.cpp \
	Doxyfile example1-small.png kdtree_index.h kmeans_index.h ChangeLog \
	od_defs.h doc

ChangeLog:
	rcs2log > ChangeLog

histeq: histeq.o
	$(CC) $(LFLAGS) histeq.o -o histeq $(LDFLAGS)

# histeq.o: histeq.cpp
# 	$(CC) $(CFLAGS) histeq.cpp

my_find_obj: my_find_obj.o
	$(CC) $(LFLAGS) my_find_obj.o -o my_find_obj $(LDFLAGS)

# my_find_obj.o: my_find_obj.cpp
# 	$(CC) $(CFLAGS) my_find_obj.cpp

adaptiveskindetector: adaptiveskindetector.o
	$(CC) $(LFLAGS) adaptiveskindetector.o -o adaptiveskindetector $(LDFLAGS)

# adaptiveskindetector.o: adaptiveskindetector.cpp
# 	$(CC) $(CFLAGS) adaptiveskindetector.cpp

frames: frames.o
	$(CC) $(LFLAGS) frames.o -o frames $(LDFLAGS)

MINGWDIR    = $(HOME)/mingw
MINGWCC     = g++
MINGWCFLAGS = -I$(MINGWDIR)/include \
              -I$(MINGWDIR)/opencv/2.4.6.0/build/install/include \
              -L$(MINGWDIR)/opencv/2.4.6.0/build/install/lib
MINGWLFLAGS = -lopencv_core246 -lopencv_highgui246 -lopencv_flann246 \
	      -lopencv_features2d246 -lopencv_objdetect246 -lopencv_imgproc246 \
	      -lopencv_video246 -lopencv_legacy246 -lopencv_contrib246 \
              -lopencv_nonfree246 -lopencv_calib3d246

ODCFILES = od.C ObjectDetection.C KeypointFilter.C SMI.C
od.exe : $(ODCFILES) ObjectDetection.h KeypointFilter.h SMI.h
	$(MINGWCC) $(MINGWCFLAGS) $(ODCFILES) $(MINGWLFLAGS) -o $@

MINGW32DIR    = /share/imagedb/picsom/mingw32
MINGW32CC     = i686-w64-mingw32-g++
MINGW32CFLAGS = -I$(MINGW32DIR)/include -L$(MINGW32DIR)/lib
MINGW32LFLAGS = -lopencv_core -lopencv_highgui -lopencv_flann \
	        -lopencv_features2d -lopencv_objdetect -lopencv_imgproc \
	        -lopencv_video -lopencv_legacy -lopencv_contrib \
                -lopencv_nonfree -lopencv_calib3d

od-mingw32.exe : $(ODCFILES) ObjectDetection.h KeypointFilter.h SMI.h
	$(MINGW32CC) $(MINGW32CFLAGS) $(ODCFILES) $(MINGW32LFLAGS) -o $@
	-ln -fs $(MINGW32DIR)/bin/libopencv_calib3d246.dll .
	-ln -fs $(MINGW32DIR)/bin/libopencv_core246.dll .
	-ln -fs $(MINGW32DIR)/bin/libopencv_features2d246.dll .
	-ln -fs $(MINGW32DIR)/bin/libopencv_flann246.dll .
	-ln -fs $(MINGW32DIR)/bin/libopencv_highgui246.dll .
	-ln -fs $(MINGW32DIR)/bin/libopencv_imgproc246.dll .
	-ln -fs $(MINGW32DIR)/bin/libopencv_nonfree246.dll .
	-ln -fs $(MINGW32DIR)/bin/libopencv_gpu246.dll .
	-ln -fs $(MINGW32DIR)/bin/libopencv_objdetect246.dll .
	-ln -fs $(MINGW32DIR)/lib/libgcc_s_dw2-1.dll .
	-ln -fs /usr/lib/gcc/i686-w64-mingw32/4.6/libgcc_s_sjlj-1.dll .
	-ln -fs /usr/lib/gcc/i686-w64-mingw32/4.6/libstdc++-6.dll .

od: ObjectDetection.o KeypointFilter.o SMI.o od.o XMLutil.o gzstream.o
	$(CC) $(LFLAGS) $(LDFLAGS) $(SOAPLIB) $(OPENMP) \
	ObjectDetection.o KeypointFilter.o SMI.o XMLutil.o gzstream.o od.o -o od 

od.o: od.C ObjectDetection.h
	$(CC) -o od.o $(CFLAGS) $(SOAPINC) od.C

SMI.o : SMI.C SMI.h
	$(CC) -o SMI.o $(CFLAGS) SMI.C

ObjectDetection.o: ObjectDetection.C ObjectDetection.h od_defs.h
	$(CC) $(CFLAGS) $(SOAPINC) $(OPENMP) ObjectDetection.C

KeypointFilter.o: KeypointFilter.C KeypointFilter.h od_defs.h
	$(CC) -o KeypointFilter.o $(CFLAGS) $(SOAPINC) $(OPENMP) KeypointFilter.C

XMLutil.o : ../c++/XMLutil.C  ../c++/XMLutil.h
	$(CC) -o XMLutil.o $(CFLAGS) -I../c++ -I/usr/include/libxml2 \
		../c++/XMLutil.C

gzstream.o : ../simple/gzstream.C  ../simple/gzstream.h
	$(CC) -o gzstream.o $(CFLAGS) -I../simple \
		../simple/gzstream.C

summary: summary.o global.o util.o savemat.h global.h IntImage.h util.h mdarray.h
	$(CC) $(LFLAGS) summary.o global.o util.o -o summary $(LDFLAGS)

summary.o: summary.C summary.h savemat.h IntImage.h util.h mdarray.h global.h
	$(CC) $(CFLAGS) summary.C

centrist: centrist.o global.o util.o savemat.h global.h IntImage.h util.h mdarray.h
	$(CC) $(LFLAGS) centrist.o global.o util.o -o centrist $(LDFLAGS)

centrist.o: centrist.C savemat.h IntImage.h util.h mdarray.h global.h
	$(CC) $(CFLAGS) centrist.C

util.o: util.cpp util.h mdarray.h
	$(CC) $(CFLAGS) util.cpp -o util.o

global.o: global.cpp global.h IntImage.h util.h mdarray.h
	$(CC) $(CFLAGS) global.cpp -o global.o

starter_video: starter_video.o
	$(CC) $(LFLAGS) starter_video.o -o starter_video $(LDFLAGS)

starter_video.o: starter_video.cpp
	$(CC) $(CFLAGS) starter_video.cpp

webcam_record: webcam_record.o
	$(CC) $(LFLAGS) webcam_record.o -o webcam_record $(LDFLAGS) -lboost_thread-mt -lboost_system-mt

webcam_record.o: webcam_record.cpp
	$(CC) $(CFLAGS) webcam_record.cpp

show_images: show_images.o
	$(CC) $(LFLAGS) show_images.o -o show_images $(LDFLAGS)

show_images.o: show_images.cpp
	$(CC) $(CFLAGS) show_images.cpp

SurfTest: SurfTest.o
	$(CC) $(LFLAGS) SurfTest.o -o SurfTest $(LDFLAGS)

# SurfTest.o: SurfTest.C
# 	$(CC) $(CFLAGS) SurfTest.C

surf_keypoint_matcher: surf_keypoint_matcher.o
	$(CC) $(LFLAGS) surf_keypoint_matcher.o -o surf_keypoint_matcher $(LDFLAGS)

surf_keypoint_matcher.o: surf_keypoint_matcher.cpp
	$(CC) $(CFLAGS) surf_keypoint_matcher.cpp

testserver: testserver.o
	$(CC) $(LFLAGS) $(SOAPLIB) testserver.o -o testserver $(LDFLAGS)

testserver.o: testserver.cpp
	$(CC) $(CFLAGS) $(SOAPINC) testserver.cpp

testclient: testclient.o
	$(CC) $(LFLAGS) $(SOAPLIB) testclient.o -o testclient $(LDFLAGS)

testclient.o: testclient.cpp
	$(CC) $(CFLAGS) $(SOAPINC) testclient.cpp

odserver: odserver.o
	$(CC) $(LFLAGS) $(SOAPLIB) odserver.o -o odserver $(LDFLAGS)

odserver.o: odserver.cpp
	$(CC) $(CFLAGS) $(SOAPINC) odserver.cpp

odclient: odclient.o
	$(CC) $(LFLAGS) $(SOAPLIB) odclient.o -o odclient $(LDFLAGS)

odclient.o: odclient.cpp
	$(CC) $(CFLAGS) $(SOAPINC) odclient.cpp

facedetect: facedetect.o
	$(CC) $(LFLAGS) facedetect.o -o facedetect $(LDFLAGS)

# facedetect.o: facedetect.cpp
# 	$(CC) $(CFLAGS) facedetect.cpp

facedetectnew: facedetectnew.o
	$(CC) $(LFLAGS) facedetectnew.o -o facedetectnew $(LDFLAGS)

camshiftdemo: camshiftdemo.o
	$(CC) $(LFLAGS) camshiftdemo.o -o camshiftdemo $(LDFLAGS)

# facedetectnew.o: facedetectnew.cpp
# 	$(CC) $(CFLAGS) facedetectnew.cpp

mouse_kalman: mouse_kalman.o
	$(CC) $(LFLAGS) mouse_kalman.o -o mouse_kalman $(LDFLAGS)

mouse_kalman.o: mouse_kalman.cpp
	$(CC) $(CFLAGS) mouse_kalman.cpp

webcam: webcam.o
	$(CC) $(LFLAGS) $(LDFLAGS) webcam.o -o webcam 

# webcam.o: webcam.cpp
# 	$(CC) $(CFLAGS) webcam.cpp

webcam-soap: webcam-soap.o
	$(CC) $(LFLAGS) $(SOAPLIB) webcam-soap.o -o webcam-soap $(LDFLAGS)

webcam-soap.o: webcam-soap.cpp
	$(CC) $(CFLAGS) $(SOAPINC) webcam-soap.cpp

lbptest: lbptest.o LBP.o
	$(CC) $(LFLAGS) lbptest.o LBP.o -o lbptest $(LDFLAGS)

lbptest.o: lbptest.C LBP.h
	$(CC) $(CFLAGS) lbptest.C

LBP.o: LBP.h LBP.C
	$(CC) $(CFLAGS) LBP.C

hcluster: hcluster.o
	$(CC) $(LFLAGS) $< -o $@ $(LDFLAGS) -lboost_system -lboost_filesystem -lboost_program_options

ifile: ifile.o
	$(CC) $(LFLAGS) $< -o $@ $(LDFLAGS) -lboost_system -lboost_filesystem

gen_codebook: gen_codebook.o
	$(CC) $(LFLAGS) $< -o $@ $(LDFLAGS) -lboost_system -lboost_filesystem

gen_codebook.o: gen_codebook.cpp encoders.h
	$(CC) $(CFLAGS) $<

.cpp.o: 
	$(CC) $(CFLAGS) $<

.C.o: 
	$(CC) $(CFLAGS) $<

clean:
	-/bin/rm -f *.o histeq SurfTest my_find_obj testserver testclient 
	-/bin/rm -f odserver odclient od webcam webcam-soap ChangeLog
