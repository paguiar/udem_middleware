#include "Vision.h"
#include "VisionBridge.h"

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <opencv2/nonfree/nonfree.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <json/json.h>

#define DEBUG

using namespace cv;

std::vector<std::string> images;
int target_count;

/* Globals are ugly... but fast for prototyping. Change later */
Mat *t_images = NULL;
Mat *t_descriptors = NULL;
std::vector<KeyPoint> * t_kp = NULL;
std::vector<Point2f> * t_ob_corners = NULL;
SurfFeatureDetector * detector = NULL;
SurfDescriptorExtractor * extractor = NULL;
FlannBasedMatcher * matcher = NULL;
int targets_encoded=0xffff;
unsigned char *cam_data=NULL;

static const char* vision_spec[] =
  {
    "implementation_id", "Vision",
    "type_name",         "Vision",
    "description",       "Computer vision RTC",
    "version",           "0.1.0",
    "vendor",            "UDEM",
    "category",          "Artificial intelligence",
    "activity_type",     "PERIODIC",
    "kind",              "DataFlowComponent",
    "max_instance",      "1",
    "language",          "C++",
    "lang_type",         "compile",

    "conf.default.int_camera", "99",
    "conf.default.int_img_width", "320",
    "conf.default.int_img_height", "240",
    "conf.default.str_files_path", "/home/paguiar/Dropbox/UDEM/Middleware/git/RTCs/vision/src/",
    "conf.default.str_images", "fast.jpg,slow.jpg,left.jpg,right.jpg,stairs.jpg,elevator.jpg,stop.jpg",
    "conf.default.str_img_rostopic", "/vrep/visionSensorData",
    "conf.default.str_rosbridge_host", "127.0.0.1",
    "conf.default.int_rosbridge_port", "9090",
    "conf.default.int_area_min", "15000",
    "conf.default.int_area_max", "300000",
    "conf.default.int_exec_delay", "200000",
    "conf.default.int_drawflag", "1",
    ""
  };

/* Evan Teran's string-splitting code from: http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c */
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s); std::string item;
    while (std::getline(ss, item, delim)) { elems.push_back(item); }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems; split(s, delim, elems);
    return elems;
}

Vision::Vision(RTC::Manager* manager)

  : RTC::DataFlowComponentBase(manager),
    m_p_optionIn("Option", m_p_option),
    m_p_resultOut("Result", m_p_result),
    m_p_statusOut("Status", m_p_status)
{
}

Vision::~Vision()
{
}

RTC::ReturnCode_t Vision::onInitialize()
{
  // Set InPort buffers
  addInPort("Option", m_p_optionIn);
  
  // Set OutPort buffer
  addOutPort("Result", m_p_resultOut);
  addOutPort("Status", m_p_statusOut);

  bindParameter("int_camera", m_int_camera, "99");
  bindParameter("int_img_width", m_int_img_width, "320");
  bindParameter("int_img_height", m_int_img_height, "240");
  bindParameter("str_files_path", m_str_files_path, "/home/paguiar/Dropbox/UDEM/Middleware/git/RTCs/vision/src/");
  bindParameter("str_images", m_str_images, "fast.jpg,slow.jpg,left.jpg,right.jpg,stairs.jpg,elevator.jpg,stop.jpg");
  bindParameter("str_img_rostopic", m_str_img_rostopic, "/vrep/visionSensorData");
  bindParameter("str_rosbridge_host", m_str_rosbridge_host, "127.0.0.1");
  bindParameter("int_rosbridge_port", m_int_rosbridge_port, "9090");
  bindParameter("int_area_min", m_int_area_min, "15000");
  bindParameter("int_area_max", m_int_area_max, "300000");
  bindParameter("int_exec_delay", m_int_exec_delay, "200000");
  bindParameter("int_drawflag", m_int_drawflag, "1");

  return RTC::RTC_OK;
}


RTC::ReturnCode_t Vision::onFinalize()
{
  return RTC::RTC_OK;
}

RTC::ReturnCode_t Vision::onActivated(RTC::UniqueId ec_id)
{
	images = split(m_str_images, ',');
	target_count = images.size();
	t_images = new Mat[target_count];
	t_descriptors = new Mat[target_count];
	t_kp = new std::vector<KeyPoint>[target_count];
	t_ob_corners = new std::vector<Point2f>[target_count];
	detector = new SurfFeatureDetector(300);
	extractor = new SurfDescriptorExtractor();
	matcher = new FlannBasedMatcher();
	cam_data = (unsigned char *)malloc(m_int_img_width*m_int_img_height*3);

	for (int i=0; i<target_count; i++){
		t_images[i] = imread( m_str_files_path+images[i], CV_LOAD_IMAGE_GRAYSCALE );
		detector->detect( t_images[i], t_kp[i]);
		extractor->compute( t_images[i], t_kp[i], t_descriptors[i] );
		t_ob_corners[i].push_back(cvPoint(0,0));
		t_ob_corners[i].push_back(cvPoint( t_images[i].cols, 0 ));
		t_ob_corners[i].push_back(cvPoint( t_images[i].cols, t_images[i].rows ));
		t_ob_corners[i].push_back(cvPoint( 0, t_images[i].rows ));
	}

	if(m_int_drawflag){
		namedWindow("Result");
	}

  return RTC::RTC_OK;
}

RTC::ReturnCode_t Vision::onDeactivated(RTC::UniqueId ec_id)
{
	if(cam_data){
		free(cam_data); cam_data = NULL;
	}

	delete[] t_images;
	delete[] t_descriptors;
	delete[] t_kp;
	delete[] t_ob_corners;
	delete detector;
	delete extractor;
	delete matcher;
	destroyWindow("Result");
	base64_cleanup();

 return RTC::RTC_OK;
}

RTC::ReturnCode_t Vision::onExecute(RTC::UniqueId ec_id)
{
	Json::Value jsonDetected;

	double *large = new double[target_count];
	double *width = new double[target_count];
	double *area = new double[target_count];

	/* Follow master's orders */
	if (m_p_optionIn.isNew())
	{
		m_p_optionIn.read();
		std::string port_str = (char*)m_p_option.data;

		/* Parse received data */
		{
			Json::Value root;
			Json::Reader reader;
			bool parsedSuccess = reader.parse(port_str, root, false);
			if(!parsedSuccess)
			{
				//panic out
			}

			if(root["targets_encoded"].asBool())
			{
				targets_encoded = root["targets_encoded"].asInt();
			}
		}
	}

	Mat *frame=NULL;
	VideoCapture *cap=NULL;
	if(m_int_camera == 99){
		getImageRaw(m_str_rosbridge_host.c_str(), m_int_rosbridge_port, m_str_img_rostopic.c_str());
		frame = new Mat(m_int_img_height, m_int_img_width, CV_8UC3, cam_data);
	}else{
		frame = new Mat();
	        cap = new VideoCapture(m_int_camera);
	        (*cap) >> (*frame);
	}

	std::vector<vector<DMatch > > *t_matches = new std::vector<vector<DMatch > >[target_count];
	std::vector<DMatch > *t_good_matches = new std::vector<DMatch >[target_count];
	std::vector<Point2f> *t_ob = new std::vector<Point2f>[target_count];
	std::vector<Point2f> *t_ob_center = new std::vector<Point2f>[target_count];
	std::vector<Point2f> *t_scene = new std::vector<Point2f>[target_count];
	std::vector<Point2f> *t_scene_corners = new std::vector<Point2f>[target_count];

	Mat des_image, img_matches;
	std::vector<KeyPoint> kp_image;

	Mat H;
	Mat image;

	cvtColor(*frame, image, CV_RGB2GRAY);

	detector->detect( image, kp_image );
	extractor->compute( image, kp_image, des_image );

	for(int t=0; t<target_count; t++){

		if (!(targets_encoded & (1 << t)))
		{
			continue;
		}

		t_scene_corners[t].resize(4);
		matcher->knnMatch(t_descriptors[t], des_image, t_matches[t], 2);
		for(int i = 0; i < min(des_image.rows-1,(int) t_matches[t].size()); i++) //THIS LOOP IS SENSITIVE TO SEGFAULTS
		{
		    if((t_matches[t][i][0].distance < 0.6*(t_matches[t][i][1].distance)) && ((int) t_matches[t][i].size()<=2 && (int) t_matches[t][i].size()>0))
		    {
			t_good_matches[t].push_back(t_matches[t][i][0]);
		    }
		}
		//Draw only "good" matches
		drawMatches( t_images[t], t_kp[t], image, kp_image, t_good_matches[t], img_matches, Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );

		if (t_good_matches[t].size() >= 4)
		{

			for(unsigned int i = 0; i < t_good_matches[t].size(); i++ )
			{
				t_ob[t].push_back( t_kp[t][ t_good_matches[t][i].queryIdx ].pt );
				t_scene[t].push_back( kp_image[ t_good_matches[t][i].trainIdx ].pt );
			}

			H = findHomography( t_ob[t], t_scene[t], CV_RANSAC );
			perspectiveTransform( t_ob_corners[t], t_scene_corners[t], H);

			large[t]=cv::norm(t_scene_corners[t][1]-t_scene_corners[t][0]);
			width[t]=cv::norm(t_scene_corners[t][2]-t_scene_corners[t][1]);
			area[t]=large[t]*width[t];

			if(area[t] > m_int_area_min && area[t] < m_int_area_max) /* Image detected! */
			{
				t_ob_center[t].push_back((t_scene_corners[t][0]+t_scene_corners[t][1])*.5);
				jsonDetected[images[t]]["id"] = t;
				jsonDetected[images[t]]["area"] = area[t];
				jsonDetected[images[t]]["center"]["x"] = t_ob_center[t][0].x;
				jsonDetected[images[t]]["center"]["y"] = t_ob_center[t][0].y;
			}

			if(m_int_drawflag)
			{
				line( image, t_scene_corners[t][0], t_scene_corners[t][1], Scalar(0, 255, 0), 4 );
				line( image, t_scene_corners[t][1], t_scene_corners[t][2], Scalar( 0, 255, 0), 4 );
				line( image, t_scene_corners[t][2], t_scene_corners[t][3], Scalar( 0, 255, 0), 4 );
				line( image, t_scene_corners[t][3], t_scene_corners[t][0], Scalar( 0, 255, 0), 4 );
			}
		}
	}

	if(jsonDetected.size()){
		m_p_result.data = jsonDetected.toStyledString().c_str();
		m_p_resultOut.write();
	}

	//Show detected matches
	if(m_int_drawflag){
		imshow( "Result", image );
		waitKey(1); //Sleep 1 ms, triggers OpenCV events, do not remove
	}

	delete[] t_matches;
	delete[] t_good_matches;
	delete[] t_ob;
	delete[] t_ob_center;
	delete[] t_scene;
	delete[] t_scene_corners;
	delete[] large;
	delete[] width;
	delete[] area;
	delete frame;
	delete cap;
	coil::usleep(m_int_exec_delay);
  return RTC::RTC_OK;
}

extern "C"
{
  void VisionInit(RTC::Manager* manager)
  {
    coil::Properties profile(vision_spec);
    manager->registerFactory(profile,
                             RTC::Create<Vision>,
                             RTC::Delete<Vision>);
  }
};
