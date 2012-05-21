#ifndef _TEST_APP
#define _TEST_APP

//#define USE_IR // Uncomment this to use infra red instead of RGB cam...

#include "ofxOpenNI.h"
#include "ofxOpenCv.h"
#include "ofxARToolkitPlus.h"

#include "Charge.h"

#include "ofMain.h"

class testApp : public ofBaseApp{

public:
	
    //OF Functions
    void setup();
	void update();
	void draw();

	void keyPressed  (int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);

    //ofxOpenNI functions
	void	setupRecording(string _filename = "");
	void	setupPlayback(string _filename);
	string	generateFileName();
    
    //Theremin Inspector Functions
    void    trackClosestUser();
    void    calibrateThereminPosition();
    void    drawEFieldLines();
   // int     autoFindThreshold();

	bool	isLive, isTracking, isCloud, isCPBkgnd, isMasking;
    
    //new flags
    bool                isCalibrating;
    bool                isDebug;

	ofxOpenNIContext	recordContext, playContext;
	ofxDepthGenerator	recordDepth, playDepth;

#ifdef USE_IR
	ofxIRGenerator		recordImage, playImage;
#else
	ofxImageGenerator	recordImage, playImage;
#endif

	ofxHandGenerator	recordHandTracker, playHandTracker;

	ofxUserGenerator	recordUser, playUser;
	ofxOpenNIRecorder	oniRecorder;

#if defined (TARGET_OSX) //|| defined(TARGET_LINUX) // only working on Mac/Linux at the moment (but on Linux you need to run as sudo...)
	ofxHardwareDriver	hardware;
#endif

	void				drawMasks();
	void				drawPointCloud(ofxUserGenerator * user_generator, int userID);

	int					nearThreshold, farThreshold;
	int					pointCloudRotationY;

	ofImage				allUserMasks, user1Mask, user2Mask, depthRangeMask;

	float				filterFactor;
    
    
    
    /* Size of the image */
    int width, height;
    
    /* ARToolKitPlus class */	
    ofxARToolkitPlus artk;	
    int threshold;
	
    /* OpenCV images */
    ofxCvColorImage colorImage;
    ofxCvGrayscaleImage grayImage;
    ofxCvGrayscaleImage	grayThres;
	
    /* Image to distort on to the marker */
    ofImage displayImage;
    /* The four corners of the image */
    vector<ofPoint> displayImageCorners;
    
    //Marker Locations
    ofVec3f marker0;
    ofVec3f marker1;	
	
	//EFIELD MEMBERS
	bool dbFlag;	
	static const int nbCharges = 7;
	static const int nbPLignes = 50;
	
	float cCote;
	
	float rotY;
	float incRotY;
	//ofColor lineColor;
	
	int distZ;
	
	Charge charge[nbCharges];
	Charge pLigne[nbPLignes];
    
};

#endif
