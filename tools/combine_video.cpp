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
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>

#include <MediaInfo/MediaInfo.h>

using namespace cv;
using namespace std;

#define REKNOW_GROUP_DISK_PATH "/home/jmakoske/mnt/reknow/"

#define REKNOW_LOGO      REKNOW_GROUP_DISK_PATH "instrumented_meeting_room/ReKnow_logo_rgb_small.png"
#define REKNOW_LOGO_MASK REKNOW_GROUP_DISK_PATH "instrumented_meeting_room/ReKnow_logo_rgb_small_mask.png"

// ----------------------------------------------------------------------

struct capturestruct {
  capturestruct(int _idx, time_t _start_epoch, VideoCapture _cap) : 
    idx(_idx), start_epoch(_start_epoch), status(true), 
    current_frame(0), cap(_cap) { }
  int idx;
  time_t start_epoch;
  bool status;
  int current_frame;
  map <int, string> matches;
  VideoCapture cap; 
};

// ----------------------------------------------------------------------

time_t calc_epoch_timestring(const string&, bool = true);

// ----------------------------------------------------------------------

void help(char** av) {
  cout << "Usage:" << endl << av[0] 
       << " [options] idx:videofile[:offset] [idx:videofile[:offset] ...] "
       << endl << endl
       << "Arguments:" << endl
       << "  idx       : index of video file, starting from 0" << endl
       << "  videofile : full path to video file" << endl
       << "  offset    : optional offset for extracted starting timestamp" 
       << endl << endl
       << "Options:" << endl
       << "  [--todisk|--todisk=X]  : " 
       << "save output video to disk as \"output.avi\" or as \"X\"" << endl
       << "  [--slides=X]           : "
       << "path to the slide pngs" << endl
       << "  [--slideidx=X]         : " 
       << "idx of videos to match slides against, use \"-1\" for none" << endl
       << "  [--fixedslide=X]       : " 
       << "filename of a fixed slide shown continously" << endl
       << "  [--hr=X]               : " 
       << "filename of heart rate CSV data to be shown" << endl;
}

// ----------------------------------------------------------------------

bool FileExists(const string &filename) {
  ifstream ifile(filename.c_str());
  if (ifile.good()) {
    ifile.close();
    return true;
  } else {
    ifile.close();
    return false;
  }
}

// ----------------------------------------------------------------------

string timestampfn(const string &fn) {
  string ret = fn;
  if (boost::find_first(ret, "capture0.avi"))
    boost::replace_first(ret, "capture0.avi", "starttime.txt");
  else if (boost::find_first(ret, "capture1.avi"))
    boost::replace_first(ret, "capture1.avi", "starttime.txt");
  else
    ret += ".txt";
  
  return ret;
}

// ----------------------------------------------------------------------

time_t calc_epoch(const string &fn) {

  MediaInfoLib::MediaInfo MI;
  string timestring;      
  wstring wfn(fn.begin(), fn.end());
  if (MI.Open(wfn)) {
    cout << "Analyzing " << fn << " with MediaInfo" << endl;
    wstring wrdate = MI.Get(MediaInfoLib::Stream_General, 0, __T("Recorded_Date"));
    string tmp(wrdate.begin(), wrdate.end());
    timestring = tmp;
    if (timestring.size())
      cout << "  found Recorded_Date: " << timestring << endl;
    else {
      cout << "  Recorded_Date not found" << endl;
      wrdate = MI.Get(MediaInfoLib::Stream_General, 0, __T("Encoded_Date"));
      string tmp2(wrdate.begin(), wrdate.end());
      if (tmp2.size()) {
	vector<string> tmpv;
	boost::split(tmpv, tmp2, boost::is_any_of(" "));
	timestring = tmpv[1]+"T"+tmpv[2]+"EEST";
	cout << "  found Encoded_date: " << timestring << endl;
      } else
	cout << "  Encoded_date not found" << endl;
    }
    //wcout << MI.Option(__T("Info_Parameters"));
    MI.Close();
  }
  
  if (!timestring.size()) {
    string timefn = timestampfn(fn);
    cout << "Date not found with MediaInfo, expecting a timestamp file [" 
	 << timefn << "]" << endl;

    ifstream timefile(timefn);
    if (!timefile) {
      cerr << "ERROR:  Timestamp file not found [" << timefn << "]" << endl;
      return 0;
    }
    if (!getline(timefile, timestring)) {
      cerr << "ERROR:  Timestamp file not readable [" << timefn << "]" << endl;
      return 0;
    }

  }

  return calc_epoch_timestring(timestring);
}

// ----------------------------------------------------------------------

time_t calc_epoch_timestring(const string &_timestring, bool verbose) {

  string timestring = _timestring;
  string timezone;
  int tzinsecs = 0;

  if (boost::find_first(timestring, "EET")) {
    boost::erase_first(timestring, "EET");
    timezone = "EET";
    tzinsecs = 2*60*60;
  } else if (boost::find_first(timestring, "EEST")) {
    boost::erase_first(timestring, "EEST");
    timezone = "EEST";
    tzinsecs = 3*60*60;
  } else {
    vector<string> tmp;  
    boost::split(tmp, timestring, boost::is_any_of("+"));
    if (tmp.size()<2) {
      cerr << "ERROR:  Time zone missing from timestring [" 
	   << timestring << "]" << endl;
      return 0;
    }
    timestring = tmp.at(0);
    timezone = tmp.at(1);
    if (timezone == "0200")
      tzinsecs = 2*60*60;
    else if (timezone == "0300")
      tzinsecs = 3*60*60;
    else {
      cerr << "ERROR:  Unrecognized timezone: [" << timezone << "]" << endl;
      return 0;
    }
  }

  boost::replace_first(timestring, "T", " ");

  if (verbose)
    cout << "Final timestring=[" << timestring << "] tz=[" << timezone 
	 << "] tzinsecs=[" << tzinsecs << "]" 
	 << endl;

  boost::posix_time::ptime t(boost::posix_time::time_from_string(timestring));
  boost::posix_time::ptime start(boost::gregorian::date(1970,1,1)); 
  boost::posix_time::time_duration dur = t - start; 

  return dur.total_seconds() - tzinsecs;
}

// ----------------------------------------------------------------------

string timedatestr(time_t epoch) {
  string timedate(ctime(&epoch));
  if (timedate.size()>0)
    timedate.erase(timedate.size()-1);
  return timedate;
}

// ----------------------------------------------------------------------

bool process_hr(string fn, map<time_t, double> &data) {

  ifstream hrfile(fn);
  if (!hrfile) {
    cerr << "ERROR: HR data file not found [" << fn << "]" << endl;
    return false;
  }
  
  string line;
  double hr_value;
  time_t hr_ts;
  bool first = true;
  while (getline(hrfile, line)) {
    if (first) {
      first = false;
      continue;
    }
    //cout << line << endl;
    vector<string> parts;
    boost::split(parts, line, boost::is_any_of(";"));
   
    if (parts.size() != 6) {
      cerr << "ERROR: Unable to parse [" << line << "]" << endl;
      return false;
    }    


    hr_value = atof(parts.at(2).c_str());
    string ts = parts.at(5) + "EEST";
    hr_ts = calc_epoch_timestring(ts.c_str(), false);
    //cout << hr_ts << " : " << hr_value << endl;
    data[hr_ts] = hr_value;
  }

  return true;
}

// ----------------------------------------------------------------------

int main(int ac, char** av) {

  if (ac <= 1) {
    help(av);
    return 1;
  }

  int nidx = -1, slideidx = 0;
  time_t min_epoch = 9999999999;
  vector<capturestruct> captures;
  bool write_video = false;
  string outputfn = "output.avi", slidedir = ".", fixedslidefn = "";
  map<time_t, double> hr;

  for (int i=1; i<ac; i++) {
    string arg(av[i]);

    if (boost::starts_with(arg, "--todisk")) {
      write_video = true;
      if (arg.size()>9)
	outputfn = arg.substr(9);
      continue;

    } else if (boost::starts_with(arg, "--slides=") && arg.size()>9) {
      slidedir = arg.substr(9);
      if (boost::starts_with(slidedir, "/"))
	cout << "WARNING: Slide path is absolute, relative path preferred" 
	     << endl;
      continue;

    } else if (boost::starts_with(arg, "--fixedslide=") && arg.size()>13) {
      fixedslidefn = arg.substr(13);
      slideidx = -2;
      continue;

    } else if (boost::starts_with(arg, "--slideidx=") && arg.size()>11) {
      slideidx = atoi(arg.substr(11).c_str());
      continue;

    } else if (boost::starts_with(arg, "--hr=") && arg.size()>5) {
      process_hr(arg.substr(5), hr);
      continue;
    }

    vector<string> parts;
    boost::split(parts, arg, boost::is_any_of(":"));

    int epoch_offset = 0;
    if (parts.size() == 3) {
      epoch_offset = atoi(parts[2].c_str());    
    } else if (parts.size() != 2) {
      cerr << "ERROR: Unable to parse [" << arg << "]" << endl;
      return 1;
    }
    int idx = atoi(parts[0].c_str());    
    string &fn = parts[1];
    if (idx > nidx)
      nidx = idx;
    VideoCapture capture(fn);
    if (!capture.isOpened()) {
      cerr << "ERROR: Failed to open a video file [" << fn << "]" << endl;
      return 1;
    }

    if (epoch_offset != 0)
      cout << "Using an offset of " << epoch_offset << " for " << fn << endl;
    time_t epoch = calc_epoch(fn) + epoch_offset;
    if (epoch == 0)
      continue;

    if (epoch<min_epoch)
      min_epoch = epoch;

    capturestruct c(idx, epoch, capture);

    string matchesfn;
    if (idx == slideidx) {
      matchesfn = fn + ".matches.txt";
      ifstream matchfile(matchesfn);
      if (!matchfile) {
	cerr << "ERROR:  Matches file not found [" << matchesfn << "]" << endl;
	return 1;
      }

      int a, prev_a = -1; 
      string b, prev_b = "";
      string line; 
      while (getline(matchfile, line)) {
	//cout << line << endl;
	vector<string> parts;
	boost::split(parts, line, boost::is_any_of(" "));
	a = atoi(parts.at(0).c_str());
	if (parts.size()==1)
	  b = prev_b;
	else
	  b = parts[1];
	if (prev_a<0)
	  prev_a = a;
	//cout << "[" << a << "] [" << b << "] [" << prev_a << "] [" << prev_b << "]" << endl;
	c.matches[a] = b;
	if (b == prev_b)
	  for (int i=a; i>prev_a; i--) {
	    //cout << "  " << i << endl;
	    c.matches[i] = b;
	  }
	prev_a = a;
	prev_b = b;
      }
    }

    captures.push_back(c);

    cout << "Added a video file idx=[" << idx << "] filename=[" << fn
	 << "] epoch= [" << epoch << "] (" << timedatestr(epoch)
	 << ") matchesfn=[" << matchesfn << "]" << endl;

  }
  nidx++;
  cout << "nidx = " << nidx << ", min_epoch=" << min_epoch 
       << " (" << timedatestr(min_epoch) << ")" << endl;

  Size totalsize(0,0);
  vector<Rect> capture_rects;
  Rect slide_rect(0,0,0,0);

  if (slideidx!=-1) {
    switch (nidx) {
    case (1):
      totalsize = Size(640,360*2);
      capture_rects.push_back(Rect(0,360,640,360));
      slide_rect = Rect(0,0,640,360);
      break;
    case (2):
      totalsize = Size(640*3,360*2);
      capture_rects.push_back(Rect(0,0,640,360));
      capture_rects.push_back(Rect(0,360,640,360));
      slide_rect = Rect(640,0,640*2,360*2);
      break;
    case (3):
      totalsize = Size(640*2,360*2);
      capture_rects.push_back(Rect(640,0,640,360));
      capture_rects.push_back(Rect(0,0,640,360));
      capture_rects.push_back(Rect(0,360,640,360));
      slide_rect = Rect(640,360,640,360);
      break;
    }
  } else {
    switch (nidx) {
    case (2):
      totalsize = Size(640,360*2);
      capture_rects.push_back(Rect(0,0,640,360));
      capture_rects.push_back(Rect(0,360,640,360));
      break;
    case (3):
      totalsize = Size(640*3,360*2);
      capture_rects.push_back(Rect(640,0,640*2,360*2));
      capture_rects.push_back(Rect(0,0,640,360));
      capture_rects.push_back(Rect(0,360,640,360));
      break;
    }
  }

  if (!totalsize.width || !totalsize.height) {
    cerr << "ERROR: totalsize not specified" << endl;
    return 1;
  }

  int framerate = 15;
  int fourcc = CV_FOURCC('m','p','4','v');

  time_t current_epoch = min_epoch; 

  VideoWriter video;
  ofstream slideoutfile;
  if (write_video) {
    video.open(outputfn, fourcc, framerate, totalsize);
    string slideoutputfn = outputfn + ".txt";
    slideoutfile.open(slideoutputfn.c_str());
    slideoutfile << current_epoch << endl;
  }

  Mat logo = imread(REKNOW_LOGO, CV_LOAD_IMAGE_COLOR);
  Mat logo_mask = imread(REKNOW_LOGO_MASK, 0);

  string window_name = "video | q or esc to quit | s to take screenshot";
  cout << "press q or esc to quit" << endl;
  namedWindow(window_name); //resizable window;

  vector<pair<bool, Mat> > frames;
  for (int i=0; i<nidx; i++) {
    Mat frame;
    frames.push_back(make_pair(true, frame));
  }

  map<string, Mat> slides;

  double hrvalue = -1.0;

  size_t nframe = 0;

  int nf = 1;
  string prevslidefn = "";

  for (;; nf++) {

    for (int i=0; i<nidx; i++)
      frames.at(i).first = false;

    bool all_done = true;
    
    string slidefn = "";

    for (size_t c=0; c<captures.size(); c++) {
      if (captures.at(c).status)
	all_done = false;
      else 
	continue;
      if (current_epoch<captures.at(c).start_epoch)
	continue;

      int idx            = captures.at(c).idx;
      VideoCapture& vc   = captures.at(c).cap;
      int& current_frame = captures.at(c).current_frame;

      bool& frameok = frames.at(idx).first;
      Mat& fr       = frames.at(idx).second;

      if (frameok)
	cout << "Overwriting idx=" << idx << " c=" << c << endl;
      frameok = vc.read(fr);
      if (frameok) {
	resize(fr, fr, Size(640, 360));
	stringstream ss;
	ss << current_frame;
	rectangle(fr, Point(2,fr.rows-22), Point(100, fr.rows-8),
		  Scalar(0,0,0), CV_FILLED);
	putText(fr, ss.str().c_str(),
		Point(10,fr.rows-10), FONT_HERSHEY_PLAIN, 1.0,
		Scalar(255,255,255));

	if (idx==slideidx && captures.at(c).matches.find(current_frame) != captures.at(c).matches.end()) {
	  slidefn = slidedir + "/" + captures.at(c).matches[current_frame];
	} else if (fixedslidefn!="") {
	  slidefn = fixedslidefn;
	}
	current_frame++;
      } else {
	fr = Mat::zeros(360, 640, CV_8UC3);
	captures.at(c).status = false;
      }
    }

    if (all_done)
      break;

    Mat frame = Mat::zeros(totalsize, CV_8UC3);
    
    for (int i=0; i<nidx; i++) {
      Mat roi(frame, capture_rects.at(i));
      Mat& fr = frames.at(i).second;
      fr.copyTo(roi);
    }

    if (slideidx!=-1 && slidefn != "") {
      if (slides.find(slidefn) == slides.end()) {	
	cout << "Loading " << slidefn << endl;
	Mat slide_frame;
	if (FileExists(slidefn))
	  slide_frame = imread(slidefn, CV_LOAD_IMAGE_COLOR);
	else {
	  cerr << "ERROR: Slide file not found [" << slidefn << "]" << endl;
	  logo.copyTo(slide_frame);	  
	}
	resize(slide_frame, slide_frame, slide_rect.size());
	slides[slidefn] = slide_frame;
      }
      Mat roi(frame, slide_rect);
      Mat& slide = slides[slidefn];
      slide.copyTo(roi);
    }

    Mat roi(frame, Rect(frame.cols-logo.cols, 0, logo.cols, logo.rows));
    logo.copyTo(roi, logo_mask);

    string current_time = timedatestr(current_epoch);
    rectangle(frame, Point(frame.cols-260,frame.rows-22), Point(frame.cols-10, frame.rows-8),
	      Scalar(0,0,0), CV_FILLED);
    putText(frame, current_time.c_str(),
	    Point(frame.cols-250,frame.rows-10), FONT_HERSHEY_PLAIN, 1.0,
	    Scalar(255,255,255));

    if (hr.find(current_epoch) != hr.end()) {
      hrvalue = hr[current_epoch];
      //cout << "Setting HR to: " << hrvalue << endl;
    }
    if (hrvalue > 0.0) {
      std::string hrstr = boost::lexical_cast<std::string>(round(hrvalue));
      putText(frame, hrstr.c_str(),
	      Point(frame.cols-150,frame.rows-50), FONT_HERSHEY_PLAIN, 5.0,
	      Scalar(0,0,255), 8);
    }
      

    if (write_video) {
      video << frame;

      if (slidefn != "" && slidefn != prevslidefn) {
	slideoutfile << current_epoch << " " << slidefn << endl;
	prevslidefn = slidefn;
      }

    } else {
      imshow(window_name, frame);
      //delay N millis, usually long enough to display and capture input
      char key = (char)waitKey(5);
      switch (key) {
      case 'q':
      case 'Q':
      case 27: //escape key
	return 0;
      case 's':
      case 'S': {
	stringstream ss;
	ss << "frame-" << nframe << ".jpg";
	cout << "Saved output image: " << ss.str() << endl;
	imwrite(ss.str(), frame);
      }
      default:
	break;
      }
    }

    if (nf==framerate) {
      current_epoch++;
      nf=0;
    }
  }


  return 0;
}
