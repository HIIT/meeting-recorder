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

#define DEFAULT_FRAMERATE 5
#define DEFAULT_METHOD 2
#define DEFAULT_CROP_OUT 40
#define DEFAULT_CROP_IN 0
#define DEFAULT_FOV 170

// ----------------------------------------------------------------------

void help(char** av) {
  cout << "Usage:" << endl << av[0] 
       << " [options] videofile"
       << endl << endl
       << "Arguments:" << endl
       << "  videofile : full path to video file" << endl
       << endl
       << "Options:" << endl
       << "  [--todisk|--todisk=X]  : " 
       << "save output video to disk as \"output.avi\" or as \"X\"" 
       << endl
       << "  [--fps=X]              : "
       << "set framerate to X, default is " << DEFAULT_FRAMERATE
       << endl
       << "  [--method=X]           : "
       << "method to use, default is " << DEFAULT_METHOD
       << endl
       << "  [--crop_out=X]         : "
       << "crop X pixels from output, default is " << DEFAULT_CROP_OUT
       << endl
       << "  [--crop_in=X]          : "
       << "crop X pixels from input, default is " << DEFAULT_CROP_IN
       << endl
       << "  [--fov=X]              : "
       << "set FOV to X, default is " << DEFAULT_FOV << " (only method 1)"
       << endl;
}

// ----------------------------------------------------------------------
// Method 1, adapted from:
// https://github.com/kscottz/dewarp/blob/master/fisheye/defish.py


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
// Method 2, adapted from: http://paulbourke.net/dome/fish2/

void fish2sphere(Mat &map_x, Mat &map_y, Size src_size, Size dst_size) {

  Point2f pfish;
  float theta,phi,r;
  Point3f psph;

  float FOV = 3.141592654; // FOV of the fisheye, eg: 180 degrees
  float width = src_size.width;
  float height = src_size.height;

  for (int y=0; y<dst_size.height; y++)
    for (int x=0; x<dst_size.width; x++) {

      // Polar angles
      theta = 3.14159265 * (x / width - 0.5); // -pi/2 to pi/2
      phi = 3.14159265 * (y / height - 0.5);	// -pi/2 to pi/2

      // Vector in 3D space
      psph.x = cos(phi) * sin(theta);
      psph.y = cos(phi) * cos(theta);
      psph.z = sin(phi);

      // Calculate fisheye angle and radius
      theta = atan2(psph.z,psph.x);
      phi = atan2(sqrt(psph.x*psph.x+psph.z*psph.z),psph.y);
      r = width * phi / FOV;

      // Pixel in fisheye space
      map_x.at<float>(y,x) = floor(0.5 * width + r * cos(theta) + 0.5);
      map_y.at<float>(y,x) = floor(0.5 * width + r * sin(theta) + 0.5);
    }

  return;
}

// ----------------------------------------------------------------------

int main(int ac, char** av) {
  
  if (ac <= 1) {
    help(av);
    return 1;
  }

  bool write_video = false, imgmode = false;
  string outputfn = "output.avi", imgfn = "";
  VideoCapture capture;
  size_t framerate = DEFAULT_FRAMERATE, method = DEFAULT_METHOD;
  size_t crop_out = DEFAULT_CROP_OUT, crop_in = DEFAULT_CROP_IN;
  size_t fov = DEFAULT_FOV;

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
    } else if (boost::starts_with(arg, "--method=") && arg.size()>9) {
      method = atoi(arg.substr(9).c_str());
      continue;
    } else if (boost::starts_with(arg, "--imgmode")) {
      imgmode = true;
      continue;
    } else if (boost::starts_with(arg, "--crop_in=") && arg.size()>10) {
      crop_in = atoi(arg.substr(10).c_str());
      continue;
    } else if (boost::starts_with(arg, "--crop_out=") && arg.size()>11) {
      crop_out = atoi(arg.substr(11).c_str());
      continue;
    } else if (boost::starts_with(arg, "--fov=") && arg.size()>6) {
      fov = atoi(arg.substr(6).c_str());
      continue;
    }

    if (imgmode) {
      imgfn = arg;
    }  else {
      capture.open(arg);
      if (!capture.isOpened()) {
        cerr << "ERROR: Failed to open a video file [" << arg << "]" << endl;
        return 1;
      }
    }
    break;
  }

  if (imgmode && write_video) {
    cerr << "ERROR: --todisk and --imgmode cannot be used at the same time" 
         << endl;
    return 1;
  }

  size_t nframe = 0; // total number of frame processed
  string window_name = "press q or esc to quit | s to save image";
  namedWindow(window_name); //resizable window;

  int fourcc = CV_FOURCC('m','p','4','v');
  VideoWriter video;
  if (write_video)
    video.open(outputfn, fourcc, framerate, Size(1280, 640));

  Size src_size(640-crop_in*2, 640-crop_in*2);
  Size dst_size(640, 640);
  Mat map_x = Mat::zeros(dst_size, CV_32F);
  Mat map_y = Mat::zeros(dst_size, CV_32F);

  cout << "Building map using method " << method << endl;
  switch (method) {
  case 1:
    buildMap(map_x, map_y, src_size, dst_size, fov, fov);
    break;
  case 2:
    fish2sphere(map_x, map_y, src_size, dst_size);
    break;
  default:
    cerr << "ERROR: unsupported method: " << method
         << endl;
    return 1;
  }
  cout << "Map done" << endl;

  for (;; nframe++) {
    Mat frame;
    bool frameok = true;
    if (imgmode)
      frame = imread(imgfn);
    else 
      frameok = capture.read(frame);
    if (!frameok)
      return 0;

    Mat newframe = Mat::zeros(Size(1280, 640), CV_8UC3);

    Mat l_roi_in(frame, Rect(crop_in, crop_in, 640-crop_in*2, 640-crop_in*2));
    Mat l_out = Mat::zeros(Size(640, 640), CV_8UC3);

    transpose(l_roi_in, l_roi_in);
    flip(l_roi_in, l_roi_in, 1);
    
    remap(l_roi_in, l_out, map_x, map_y, CV_INTER_LINEAR);

    Mat l_out_crop(l_out, Rect(crop_out, crop_out, 640-crop_out*2, 640-crop_out*2));
    Mat l_out2;
    resize(l_out_crop, l_out2, Size(640,640));

    Mat l_roi_out(newframe, Rect(0, 0, 640, 640));
    l_out2.copyTo(l_roi_out);

    Mat r_roi_in(frame, Rect(640+crop_in, crop_in, 640-crop_in*2, 640-crop_in*2));
    Mat r_out = Mat::zeros(Size(640, 640), CV_8UC3);

    transpose(r_roi_in, r_roi_in);
    flip(r_roi_in, r_roi_in, 0);
    
    remap(r_roi_in, r_out, map_x, map_y, CV_INTER_LINEAR);

    Mat r_out_crop(r_out, Rect(crop_out, crop_out, 640-crop_out*2, 640-crop_out*2));
    Mat r_out2;
    resize(r_out_crop, r_out2, Size(640,640));

    Mat r_roi_out(newframe, Rect(640, 0, 640, 640));
    r_out2.copyTo(r_roi_out);

    if (write_video)
	video << newframe;
    else {

      imshow(window_name, newframe);
      //delay N millis, usually long enough to display and capture input
      char key = (char)waitKey(imgmode?0:50);
      switch (key) {
      case 'q':
      case 'Q':
      case 27: //escape key
        return 0;
      case 's':
      case 'S': {
	stringstream ss;
	ss << "unfish-output.jpg";
	cout << "Saved output image: " << ss.str() << endl;
	imwrite(ss.str(), newframe);
      }

      }
    }
  }

  return 0;
}

// ----------------------------------------------------------------------

