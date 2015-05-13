# meeting-recorder

A Qt/OpenCV-based webcam and audio recorder.  Supports up to two simultaneous cameras and writes video immediately to disk. 

## Requirements

### Qt

Qt version >= 5.4 is recommended.  It can be downloaded from
[Digia](https://www.qt.io/download-open-source/).

### OpenCV

OpenCV can be installed using package managers, e.g. in Linux:

	sudo apt-get install opencv-dev

or in OS X with MacPorts:

	sudo port install opencv

## Compiling

	PATH_TO_QT_BINDIR/qmake
	make

## Running

### Linux

One camera, of **hd** or **fullhd** resolution:

	./mrecorder 0:hd
	./mrecorder 0:fullhd

Two cameras, both in this example with **hd** resolution:

	./mrecorder 0:hd 1:hd

Usage information

	./mrecorder --help

### OS X

	mrecorder.app/Contents/MacOS/mrecorder

