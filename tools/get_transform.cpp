/*
Copyright (c) 2015 University of Helsinki

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <fstream>
#include <vector>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <boost/algorithm/string.hpp>

using namespace cv;
using namespace std;

// ----------------------------------------------------------------------

void help(char** av) {
  cout << "Usage:" << endl << av[0] << " [--rotate] file" << endl;
}

// ----------------------------------------------------------------------

void on_mouse( int e, int x, int y, int d, void *ptr )
{
  if( e != EVENT_LBUTTONDOWN )
    return;


    Point*p = (Point*)ptr;
    p->x = x;
    p->y = y;
}

// ----------------------------------------------------------------------

void transform_image(Mat &src, const vector<Point2f> &pp) {
  if (pp.size()<4) {
    cout << "Transform needs four points " << endl;
    return;
  }
  Point2f src_pts[4];
  for (size_t ii=0; ii<4; ii++)
    src_pts[ii] = 3.0*(pp.at(ii)-Point2f(640/2, 360/2));
  Point2f dst_pts[4];
  dst_pts[0] = Point(0,0);
  dst_pts[1] = Point(src.cols/2,0);
  dst_pts[2] = Point(src.cols/2, src.rows/2);
  dst_pts[3] = Point(0,src.rows/2);
  Mat M = getPerspectiveTransform(src_pts, dst_pts);
  FileStorage fs("transform.yml", FileStorage::WRITE);
  fs << "M" << M;
  fs << "pp" << pp;
  fs.release();
  warpPerspective(src, src, M, Size(src.cols, src.rows));
}

// ----------------------------------------------------------------------

int main(int ac, char** av) {

  if (ac <= 1) {
    help(av);
    return 1;
  }

  VideoCapture capture;
  bool rotate = false;

  for (int i=1; i<ac; i++) {
    string arg(av[i]);
    
    if (boost::starts_with(arg, "--rotate")) {
      rotate = true;
      continue;
    }

    capture.open(arg);
    if (!capture.isOpened()) {
      cerr << "ERROR: Failed to open a video file [" << arg << "]" << endl;
      return 1;
    }
    break;
  }

  namedWindow("Select corners", CV_WINDOW_AUTOSIZE);

  Mat frame, smallframe;
  bool frameok; 
  bool read_frames = true;
  bool first = true;

  Point p(0,0), q(0,0);
  vector<Point2f> pp;

  setMouseCallback("Select corners", on_mouse, &p); 

  while (1) {
    if (read_frames) {
      frameok = capture.read(frame);
      if (!frameok) {
	cerr << "ERROR: Reading a frame from video failed" << endl;
	return 1;      
      }

      if (first) {
	cout << "Input size: " << frame.cols << "x" << frame.rows << endl;
	first = false;
      }

      if (rotate)
	flip(frame, frame, -1);
      
      resize(frame, smallframe, Size(640, 360));
    }

    if (p != q) {
      cout << p.x << " " << p.y << endl; 
      pp.push_back(p);
      q = p;
    }

    Mat image = Mat::zeros(360*2, 640*2, CV_8UC3);
    Mat roi(image, Rect(640/2, 360/2, 640, 360));
    smallframe.copyTo(roi);

    vector<Point2f>::const_iterator iter;
    for (iter = pp.begin(); iter != pp.end(); ++iter)
      circle(image, *iter, 5, Scalar(0,0,255), -1); 

    imshow("Select corners", image);
    char key = (char)waitKey(5);
    
    switch (key) {
    case 'q':
    case 'Q':
    case 27: //escape key
      return 0;
    case 's':
    case 'S':
      read_frames = !read_frames;
      break;
    case 't':
    case 'T':
      transform_image(frame, pp);
      resize(frame, smallframe, Size(640*2, 360*2));
      imshow("Select corners", smallframe);
      waitKey(0);
    }

  }
}

// ----------------------------------------------------------------------
