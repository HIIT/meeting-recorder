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
#define REKNOW_LOGO_BIG  REKNOW_GROUP_DISK_PATH "instrumented_meeting_room/ReKnow_logo_rgb.png"

// ----------------------------------------------------------------------

struct capturestruct {
  capturestruct(int _idx, time_t _start_epoch, VideoCapture _cap) : 
    idx(_idx), start_epoch(_start_epoch), status(true), 
    current_frame(0), cap(_cap), successor(0), transform(false) { }
  int idx;
  time_t start_epoch;
  bool status;
  int current_frame;
  map <int, string> matches;
  VideoCapture cap; 
  size_t successor;
  bool transform;
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
       << "  offset    : optional offset for extracted starting timestamp," 
       << "              the value \"C\" is for continuing from previous video"
       << endl << endl
       << "Options:" << endl
       << "  [--todisk|--todisk=X]  : " 
       << "save output video to disk as \"output.avi\" or as \"X\"" << endl
       << "  [--slides=X]           : "
       << "path to the slide pngs" << endl
       << "  [--slideidx=X]         : " 
       << "idx of videos to match slides against, use \"-1\" for none" << endl
       << "  [--transidx=X]         : " 
       << "idx of videos to perspective-transform with \"transform.yml\"" << endl
       << "  [--startidx=X]         : " 
       << "idx of videos to start the actual recording, by default not used"
       << endl
       << "  [--fixedslide=X]       : " 
       << "filename of a fixed slide shown continously" << endl
       << "  [--fps=X]              : "
       << "set framerate to X, default is 25" << endl
       << "  [--title=X]            : "
       << "set title of video to X" << endl
       << "  [--hr=X]               : " 
       << "filename of heart rate CSV data to be shown" << endl;
}

// ----------------------------------------------------------------------

void print_captures(vector<capturestruct> &c) {
  vector<capturestruct>::const_iterator iter;
  size_t n=0;
  for (iter = c.begin(); iter != c.end(); ++iter, ++n) {
    cout << n << ": idx=[" << iter->idx << "] start_epoch=[" 
	 << iter->start_epoch << "] status=[" << iter->status 
	 << "] current_frame=[" << iter->current_frame 
	 << "] matches.size()=[" << iter->matches.size() 
	 << "] successor=[" << iter->successor << "]"
	 << endl;
  }  
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
  
  string timestring, timefn = timestampfn(fn);
  ifstream timefile(timefn);
  if (timefile) {
    cout << "Found timestamp file [" << timefn << "]" 
	 << endl;
    if (!getline(timefile, timestring)) {
      cerr << "ERROR:  Timestamp file not readable [" << timefn << "]" << endl;
      return 0;
    }
  } else {
   cout << "No timestamp file [" << timefn << "] found, looking for metadata next" 
	<< endl;

   MediaInfoLib::MediaInfo MI;
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
  }
  
  if (!timestring.size()) {
    cerr << "ERROR: Timestamp not found" << endl;
    return 0;
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

void transform_frame(Mat &src, const Mat &M) {

  Mat image = Mat::zeros(360*2, 640*2, CV_8UC3);
  Mat roi(image, Rect(640/2, 360/2, 640, 360));
  src.copyTo(roi);

  warpPerspective(image, src, M, Size(src.cols, src.rows));
}

// ----------------------------------------------------------------------

void initialize_output(size_t width, size_t height, Size &totalsize,
		       vector<Rect> &capture_rects) {
  totalsize = Size(640*width,360*height);
  for (size_t jj=0; jj<height; jj++)
    for (size_t ii=0; ii<width; ii++)
      capture_rects.push_back(Rect(640*ii,360*jj,640,360));
}


// ----------------------------------------------------------------------

int main(int ac, char** av) {

  if (ac <= 1) {
    help(av);
    return 1;
  }

  int nidx = -1, slideidx = 0, startidx = -1, transidx = -1;
  time_t min_epoch = 9999999999, recstart_epoch = 9999999999;
  vector<capturestruct> captures;
  bool write_video = false;
  string outputfn = "output.avi", slidedir = ".", fixedslidefn = "";
  string title;
  size_t framerate = 25;
  map<time_t, double> hr;
  bool debug_printcaptures = false;

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

    } else if (boost::starts_with(arg, "--transidx=") && arg.size()>11) {
      transidx = atoi(arg.substr(11).c_str());
      continue;

    } else if (boost::starts_with(arg, "--fps=") && arg.size()>6) {
      framerate = atoi(arg.substr(6).c_str());
      continue;

    } else if (boost::starts_with(arg, "--startidx=") && arg.size()>11) {
      startidx = atoi(arg.substr(11).c_str());
      continue;

    } else if (boost::starts_with(arg, "--hr=") && arg.size()>5) {
      process_hr(arg.substr(5), hr);
      continue;

    } else if (boost::starts_with(arg, "--title=") && arg.size()>8) {
      title = arg.substr(8);
      continue;

    } else if (boost::starts_with(arg, "--debugcaps")) {
      debug_printcaptures = true;
      continue;
    }

    vector<string> parts;
    boost::split(parts, arg, boost::is_any_of(":"));

    int epoch_offset = 0;
    bool continue_from_previous = false;
    if (parts.size() == 3) {
      if (parts[2] == "C" || parts[2] == "c")
	continue_from_previous = true;
      else
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

    if (startidx < 0) {
      recstart_epoch = min_epoch;
    } else if (idx == startidx && epoch<recstart_epoch) {
      recstart_epoch = epoch;
    }

    capturestruct c(idx, epoch, capture);

    if (continue_from_previous) {
      if (captures.size()) {
	captures.back().successor = captures.size(); 
	c.status = false;
      } else {
	cerr << "ERROR: First video cannot continue from previous"<< endl;
	return 1;
      }
    }

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

    if (idx == transidx)
      c.transform = true;

    captures.push_back(c);

    cout << "Added a video file idx=[" << idx << "] filename=[" << fn
	 << "] epoch= [" << epoch << "] (" << timedatestr(epoch)
	 << ") matchesfn=[" << matchesfn << "]" << endl;

  }
  nidx++;
  cout << "nidx = " << nidx << ", min_epoch=" << min_epoch 
       << " (" << timedatestr(min_epoch) << ")" << endl;

  if (debug_printcaptures)
    print_captures(captures);

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
    default:
      cerr << "ERROR: unsupported nidx=" << nidx
	   << " for slideidx=" << slideidx << endl;
      return 1;
    }
  } else {
    switch (nidx) {
    case (2):
      initialize_output(1, 2, totalsize, capture_rects);
      break;
    case (3):
      totalsize = Size(640*3,360*2);
      capture_rects.push_back(Rect(640,0,640*2,360*2));
      capture_rects.push_back(Rect(0,0,640,360));
      capture_rects.push_back(Rect(0,360,640,360));
      break;
    case (4):
      initialize_output(2, 2, totalsize, capture_rects);
      break;
    case (5):
    case (6):
      initialize_output(3, 2, totalsize, capture_rects);
      break;
    case (7):
    case (8):
    case (9):
      initialize_output(3, 3, totalsize, capture_rects);
      break;
    default:
      cerr << "ERROR: unsupported nidx=" << nidx
	   << " for slideidx=" << slideidx << endl;
      return 1;
    }
  }

  if (!totalsize.width || !totalsize.height) {
    cerr << "ERROR: totalsize not specified" << endl;
    return 1;
  }

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

  Mat logo_big = imread(REKNOW_LOGO_BIG, CV_LOAD_IMAGE_COLOR);
  resize(logo_big, logo_big, Size(), 0.1, 0.1);

  string window_name = "video | q or esc to quit | s to take screenshot";
  namedWindow(window_name); //resizable window;

  vector<pair<bool, Mat> > frames;
  for (int i=0; i<nidx; i++) {
    Mat frame;
    frames.push_back(make_pair(true, frame));
  }

  map<string, Mat> slides;

  double hrvalue = -1.0;

  Mat M;
  if (transidx > -1) {
    FileStorage fs;
    string transfn = "transform.yml";
    fs.open(transfn, FileStorage::READ);
    if (!fs.isOpened()) {
      cerr << "ERROR: failed to open " << transfn << endl;
      return 1;
    }
    fs["M"] >> M;
  }

  size_t nframe = 0; // total number of frame processed
  size_t nf = 1; // number of frame within a second

  string prevslidefn = "";

  for (;; nf++, nframe++) {

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

	if (idx==slideidx && captures.at(c).matches.find(current_frame) != 
	    captures.at(c).matches.end()) {
	  slidefn = slidedir + "/" + captures.at(c).matches[current_frame];
	} else if (fixedslidefn!="") {
	  slidefn = fixedslidefn;
	}
	if (idx==transidx)
	  transform_frame(fr, M);
	current_frame++;
      } else {
	fr = Mat::zeros(360, 640, CV_8UC3);
	captures.at(c).status = false;
	if (captures.at(c).successor>0)
	  captures.at(captures.at(c).successor).status = true;
      }
    }

    if (all_done)
      break;

    Mat frame = Mat::zeros(totalsize, CV_8UC3);

    if (current_epoch >= recstart_epoch && current_epoch < recstart_epoch+5) {
      frame = Scalar(255,255,255);
      Mat roi(frame, Rect((frame.cols-logo_big.cols)/2,20,
			  logo_big.cols,logo_big.rows));
      logo_big.copyTo(roi);
      putText(frame, title.c_str(),
	      Point(20, logo_big.rows+70), FONT_HERSHEY_SIMPLEX, 1.0,
	      Scalar(0,0,0), 2.5);
      string current_time = timedatestr(recstart_epoch);
      putText(frame, current_time.c_str(),
	      Point(20, logo_big.rows+110), FONT_HERSHEY_SIMPLEX, 1.0,
	      Scalar(0,0,0), 2.5);

    } else {
    
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
      rectangle(frame, Point(frame.cols-260,frame.rows-22),
		Point(frame.cols-10, frame.rows-8),
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
      if (current_epoch < recstart_epoch) {
	string recstart_text = "Recording will start at " + 
	  timedatestr(recstart_epoch);
	putText(frame, recstart_text.c_str(), Point(10,20), FONT_HERSHEY_PLAIN, 
		1.0, Scalar(0,0,255));
      }
    }

    if (write_video) {
      if (current_epoch >= recstart_epoch)
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
