/*
Copyright (c) 2016 University of Helsinki

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

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <boost/algorithm/string.hpp>

using namespace cv;
using namespace std;

// ----------------------------------------------------------------------

void help(char** av) {
  cout << "Usage:" << endl << av[0] 
       << " [options] videofile"
       << endl << endl
       << "Arguments:" << endl
       << "  videofile : full path to video file" << endl
       << endl << endl
       << "Options:" << endl
       << "  [--todisk|--todisk=X]  : " 
       << "save output video to disk as \"output.avi\" or as \"X\"" 
       << endl
       << "  [--fps=X]              : "
       << "set framerate to X, default is 5"
       << endl;

}

// ----------------------------------------------------------------------

void buildMap(Mat &map_x, Mat &map_y, Size src_size, Size dst_size, 
              double hfovd=170.0, double vfovd=170.0) {

  // Build the fisheye mapping
  double vfov = (vfovd/180.0)*CV_PI;
  double hfov = (hfovd/180.0)*CV_PI;
  double vstart = ((180.0-vfovd)/180.00)*CV_PI/2.0;
  double hstart = ((180.0-hfovd)/180.00)*CV_PI/2.0;
  size_t count = 0;

  // need to scale to changed range from our
  // smaller cirlce traced by the fov
  double xmax = sin(CV_PI/2.0)*cos(vstart);
  double xmin = sin(CV_PI/2.0)*cos(vstart+vfov);
  double xscale = xmax-xmin;
  double xoff = xscale/2.0;
  double zmax = cos(hstart);
  double zmin = cos(hfov+hstart);
  double zscale = zmax-zmin;
  double zoff = zscale/2.0;

  // Fill in the map, this is slow but
  // we could probably speed it up
  // since we only calc it once, whatever
  for (int y=0; y<dst_size.height; y++)
    for (int x=0; x<dst_size.width; x++) {
      count++;
      double phi = vstart+(vfov*((float(x)/float(dst_size.width))));
      double theta = hstart+(hfov*((float(y)/float(dst_size.height))));
      double xp = ((sin(theta)*cos(phi))+xoff)/zscale;
      double zp = ((cos(theta))+zoff)/zscale;
      double xS = src_size.width-(xp*src_size.width);
      double yS = src_size.height-(zp*src_size.height);
      map_x.at<float>(y,x) = floor(xS +0.5);
      map_y.at<float>(y,x) = floor(yS +0.5);
    }

  return;
}

// ----------------------------------------------------------------------

/*
void unwarp(Mat &img, Mat &output, const Mat &xmap, const Mat &ymap) {
  // apply the unwarping map to our image
  output = remap(img, xmap, ymap);
  return;
}
*/

// ----------------------------------------------------------------------

int main(int ac, char** av) {
  
  if (ac <= 1) {
    help(av);
    return 1;
  }

  bool write_video = false;
  string outputfn = "output.avi";
  VideoCapture capture;
  size_t framerate = 5;

  for (int i=1; i<ac; i++) {
    string arg(av[i]);

    if (boost::starts_with(arg, "--todisk")) {
      write_video = true;
      if (arg.size()>9)
	outputfn = arg.substr(9);
      continue;
    } else if (boost::starts_with(arg, "--fps=") && arg.size()>6) {
      framerate = atoi(arg.substr(6).c_str());
      continue;
    }

    capture.open(arg);
    if (!capture.isOpened()) {
      cerr << "ERROR: Failed to open a video file [" << arg << "]" << endl;
      return 1;
    }
    break;
  }

  size_t nframe = 0; // total number of frame processed
  string window_name = "press q or esc to quit";
  namedWindow(window_name); //resizable window;

  int fourcc = CV_FOURCC('m','p','4','v');
  VideoWriter video;
  if (write_video)
    video.open(outputfn, fourcc, framerate, Size(1280, 640));

  Size src_size(640, 640);
  Size dst_size(640, 640);
  Mat map_x = Mat::zeros(dst_size, CV_32F);
  Mat map_y = Mat::zeros(dst_size, CV_32F);

  cout <<  "BUILDING MAP..." << endl;
  buildMap(map_x, map_y, src_size, dst_size);
  cout <<  "MAP DONE"<< endl;

  for (;; nframe++) {
    Mat frame;
    bool frameok = capture.read(frame);
    if (!frameok)
      return 0;

    Mat newframe = Mat::zeros(Size(1280, 640), CV_8UC3);

    Mat l_roi_in(frame, Rect(0, 0, 640, 640));
    Mat l_out = Mat::zeros(Size(640, 640), CV_8UC3);

    transpose(l_roi_in, l_roi_in);
    flip(l_roi_in, l_roi_in, 1);
    
    remap(l_roi_in, l_out, map_x, map_y, CV_INTER_LINEAR);

    int crop = 90;

    Mat l_out_crop(l_out, Rect(crop, crop, 640-crop*2, 640-crop*2));
    Mat l_out2;
    resize(l_out_crop, l_out2, Size(640,640));

    Mat l_roi_out(newframe, Rect(0, 0, 640, 640));
    l_out2.copyTo(l_roi_out);

    Mat r_roi_in(frame, Rect(640, 0, 640, 640));
    Mat r_out = Mat::zeros(Size(640, 640), CV_8UC3);

    transpose(r_roi_in, r_roi_in);
    flip(r_roi_in, r_roi_in, 0);
    
    remap(r_roi_in, r_out, map_x, map_y, CV_INTER_LINEAR);

    Mat r_out_crop(r_out, Rect(crop, crop, 640-crop*2, 640-crop*2));
    Mat r_out2;
    resize(r_out_crop, r_out2, Size(640,640));

    Mat r_roi_out(newframe, Rect(640, 0, 640, 640));
    r_out2.copyTo(r_roi_out);

    if (write_video)
	video << newframe;
    else {

      imshow(window_name, newframe);
      //delay N millis, usually long enough to display and capture input
      char key = (char)waitKey(50);
      switch (key) {
      case 'q':
      case 'Q':
      case 27: //escape key
        return 0;
      }
    }
  }

  return 0;
}

// ----------------------------------------------------------------------

