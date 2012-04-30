#include "testApp.h"

//Globals
//----------------------------------

//E Field Quality/Efficiency
extern const float intensPas = 5; // Distance between segments
extern const float dMin2 = 10;// Last Mile Drawing close to Charges
extern const float numFieldIterations = 1000; // How many cycles do we draw for

//E Field Line Style
extern const float lineWeight = 9;
extern const float fieldScale = 3;
float touchedDist = 150;
bool lineTouched = false;
ofColor lineColor;

//--------------------------------------------------------------
void testApp::setup() {

    width = 640;
	height = 480;
    
	isLive			= true;
	isTracking		= true;
	isTrackingHands	= true;
	isFiltering		= false;
	isRecording		= false;
	isCloud			= false;
	isCPBkgnd		= true;
	isMasking		= true;
    isCalibrating   = false;

	nearThreshold = 500;
	farThreshold  = 1000;

	filterFactor = 0.1f;

	setupRecording();

    colorImage.allocate(width, height);
	grayImage.allocate(width, height);
	grayThres.allocate(width, height);
	
	// This uses the default camera calibration and marker file
	artk.setup(width, height);
    
	// The camera calibration file can be created using GML:
	// http://graphics.cs.msu.ru/en/science/research/calibration/cpp
	// and these instructions:
	// http://studierstube.icg.tu-graz.ac.at/doc/pdf/Stb_CamCal.pdf
	// This only needs to be done once and will aid with detection
	// for the specific camera you are using
	// Put that file in the data folder and then call setup like so:
	// artk.setup(width, height, "myCamParamFile.cal", "markerboard_480-499.cfg");
	
	// Set the threshold
	// ARTK+ does the thresholding for us
	// We also do it in OpenCV so we can see what it looks like for debugging
	threshold = 70;
	artk.setThreshold(threshold);
    
    
    //SETUP EFIELD AND CHARGES
	//------------------------
	dbFlag = false;
	cCote = 4;
	rotY = 0.0;
	incRotY = 0.01;
	distZ = 100;
	
	//set Line Color to be WHITE
	lineColor.r=255;
	lineColor.g=255;
	lineColor.b=255;
	lineColor.a=255;
	
	
	//Setup Charges on Theremin
	charge[0].set(-100.0,0.0,0.0,1000.0);
	charge[1].set(-100.0,0.0,0.0,1000.0);
    
	tempInc = 100;
	tempX = -30;
	tempY = -180;
	tempZ = 60;
	scale = 4;
	
	//volume
	charge[2].set(300,200,0,-1000);
	charge[3].set(320,250,0,-1000.0);
	//pitch
	charge[4].set(340,200,0,-1000.0);
	charge[5].set(360,200,0,-1000.0);
	//ground
	charge[6].set(0,-1000,0,400.0);
	
	for (int i = 0; i < nbPLignes; i++)
	{
		pLigne[i].set(ofRandom(0,640),ofRandom(0,480),(ofRandom(0,0)),1);
	}
    
	ofBackground(0, 0, 0);

}

void testApp::setupRecording(string _filename) {

#if defined (TARGET_OSX) //|| defined(TARGET_LINUX) // only working on Mac/Linux at the moment (but on Linux you need to run as sudo...)
	hardware.setup();				// libusb direct control of motor, LED and accelerometers
	hardware.setLedOption(LED_OFF); // turn off the led just for yacks (or for live installation/performances ;-)
#endif

	recordContext.setup();	// all nodes created by code -> NOT using the xml config file at all
	//recordContext.setupUsingXMLFile();
	recordDepth.setup(&recordContext);
	recordImage.setup(&recordContext);

	recordUser.setup(&recordContext);
	recordUser.setSmoothing(filterFactor);				// built in openni skeleton smoothing...
	recordUser.setUseMaskPixels(isMasking);
	recordUser.setUseCloudPoints(isCloud);
	recordUser.setMaxNumberOfUsers(2);					// use this to set dynamic max number of users (NB: that a hard upper limit is defined by MAX_NUMBER_USERS in ofxUserGenerator)

	recordContext.toggleRegisterViewport();
	recordContext.toggleMirror();

	oniRecorder.setup(&recordContext, ONI_STREAMING);
	//oniRecorder.setup(&recordContext, ONI_CYCLIC, 60);
	//read the warning in ofxOpenNIRecorder about memory usage with ONI_CYCLIC recording!!!

}

void testApp::setupPlayback(string _filename) {

}

//--------------------------------------------------------------
void testApp::update(){

#ifdef TARGET_OSX // only working on Mac at the moment
	hardware.update();
#endif

	if (isLive) {

		// update all nodes
		recordContext.update();
		recordDepth.update();
		recordImage.update();

		// demo getting depth pixels directly from depth gen
		depthRangeMask.setFromPixels(recordDepth.getDepthPixels(nearThreshold, farThreshold),
									 recordDepth.getWidth(), recordDepth.getHeight(), OF_IMAGE_GRAYSCALE);

		// update tracking/recording nodes
		if (isTracking) {
            recordUser.update();
            
        }
		if (isRecording) oniRecorder.update();

		// demo getting pixels from user gen
		if (isTracking && isMasking) {
			allUserMasks.setFromPixels(recordUser.getUserPixels(), recordUser.getWidth(), recordUser.getHeight(), OF_IMAGE_GRAYSCALE);
			user1Mask.setFromPixels(recordUser.getUserPixels(1), recordUser.getWidth(), recordUser.getHeight(), OF_IMAGE_GRAYSCALE);
			user2Mask.setFromPixels(recordUser.getUserPixels(2), recordUser.getWidth(), recordUser.getHeight(), OF_IMAGE_GRAYSCALE);
		}
        
        
        
        //colorImage.setFromPixels(recordImage.getPixels(), width, height);
		colorImage.setFromPixels(recordImage.getPixels(), recordUser.getWidth(), recordUser.getHeight());
        colorImage.mirror(false,true);
        
		// convert our camera image to grayscale
		grayImage = colorImage;
		// apply a threshold so we can see what is going on
		grayThres = grayImage;
		grayThres.threshold(threshold);
		
		// Pass in the new image pixels to artk
		artk.update(grayImage.getPixels());

	} 
	
}

//--------------------------------------------------------------
void testApp::draw(){

	ofSetColor(255, 255, 255);

	if (isLive) {
        
		//recordDepth.draw(0,0,640,480);
		recordImage.draw(0, 0, 640, 480);

		depthRangeMask.draw(0, 480, 320, 240);	// can use this with openCV to make masks, find contours etc when not dealing with openNI 'User' like objects

		if (isTracking) {
			recordUser.draw();

			if (isMasking) drawMasks();
			if (isCloud) drawPointCloud(&recordUser, 1);	// 0 gives you all point clouds; use userID to see point clouds for specific users
            
            for(int i = 0; i < recordUser.getNumberOfTrackedUsers() ; i++){  
                ofxTrackedUser* tracked = recordUser.getTrackedUser(i+1);
                
                if( recordUser.getXnUserGenerator().GetSkeletonCap().IsTracking(tracked->id)){  
                    if(tracked->left_lower_arm.found){
                        
                        ofSetHexColor(0xffffff);
                        
                        ofDrawBitmapString(ofToString(tracked->left_lower_arm.position[1].X),650,10);
                        ofDrawBitmapString(ofToString(tracked->left_lower_arm.position[1].Y),750,10);
                        ofDrawBitmapString(ofToString(tracked->left_lower_arm.position[1].Z),850,10);
                        
                        //Put Charges in your hands
                        charge[0].set(tracked->left_lower_arm.position[1].X,tracked->left_lower_arm.position[1].Y,0,1500);
                        
                        ofSetHexColor(0xffff77);
                        
                        ofCircle(tracked->left_lower_arm.position[1].X,tracked->left_lower_arm.position[1].Y,10);
                    }
                    if(tracked->right_lower_arm.found){
                        
                        ofSetHexColor(0xffffff);
                        
                        ofDrawBitmapString(ofToString(tracked->right_lower_arm.position[1].X),650,30);
                        ofDrawBitmapString(ofToString(tracked->right_lower_arm.position[1].Y),750,30);
                        ofDrawBitmapString(ofToString(tracked->right_lower_arm.position[1].Z),850,30);
                        
                        //Put Charges in your hands
                        charge[1].set(tracked->right_lower_arm.position[1].X,tracked->right_lower_arm.position[1].Y,0,1500);
                        
                        ofSetHexColor(0xffff77);
                        
                        ofCircle(tracked->right_lower_arm.position[1].X,tracked->right_lower_arm.position[1].Y,10);
                    }
                }
                
                //Draw Field Lines
                for(int i = 0; i < nbPLignes; i++)
                {
                   // if ((charge[0].distance(pLigne[i]) < touchedDist) || (charge[1].distance(pLigne[i]) < touchedDist))
                    {
                        lineTouched = true;
                        //set Line Color to be PINK
                        lineColor.r=198;
                        lineColor.g=199;
                        lineColor.b=0;
                        lineColor.a=(charge[0].distance(pLigne[i])/100.0)*25;
                        pLigne[i].champ(charge,nbCharges);
                    }
                    
                    pLigne[i].champ(charge,nbCharges);
                    lineTouched=false;
                    //set Line Color to be WHITE
                    lineColor.r=255;
                    lineColor.g=255;
                    lineColor.b=255;
                    lineColor.a=255;
                }
                
            }
		}
        
//**********************************************************************
//    Theremin Position Calibration
//
//    If in Calibration Mode, then Detect Markers and Setup "the world"
//**********************************************************************
        
        if (isCalibrating)
        {
        // Draw Threshold image and allow user to adjust for lighting conditions
        ofSetHexColor(0xffffff);
        grayThres.mirror(false,true);
        grayThres.draw(0, 0);
            
        // How many Markers have we found?
        ofSetHexColor(0x00ff00);	
        ofDrawBitmapString(ofToString(artk.getNumDetectedMarkers()) + " marker(s) found", 10, 20);
            
        ofSetHexColor(0x00ff00);	
        ofDrawBitmapString("Threshold: " + ofToString(threshold), 10, 30);
        ofDrawBitmapString("Use the Up/Down keys to adjust the threshold", 10, 40);
        
        // See if marker ID '0' was detected
        // and draw a circle in the center with the ID number.
        int myIndex = artk.getMarkerIndex(0);
        ofPoint center;
            int depth;
        if(myIndex >= 0) {	
            // Can also get the center like this:
            center = artk.getDetectedMarkerCenter(myIndex);
            ofSetHexColor(0x000055);
            ofCircle(width-center.x,center.y,30);
            ofSetHexColor(0x00ff00);
            ofDrawBitmapString("0",width-center.x,center.y,10);
            depth = recordDepth.getPixelDepth(width-center.x,center.y);
            ofDrawBitmapString(ofToString(depth),width-center.x,center.y+20,10);
        }
        
        // See if marker ID '1' was detected
        // and draw a circle in the center with the ID number.
        myIndex = artk.getMarkerIndex(1);
        if(myIndex >= 0) {	
            // Can also get the center like this:
            center = artk.getDetectedMarkerCenter(myIndex);
            ofSetHexColor(0x000055);
            ofCircle(width-center.x,center.y,30);
            ofSetHexColor(0x00ff00);
            ofDrawBitmapString("1",width-center.x,center.y,10);
            depth = recordDepth.getPixelDepth(width-center.x,center.y);
            ofDrawBitmapString(ofToString(depth),width-center.x,center.y+20,10);
        }
        }

	}     
    
//*******************************
//
// On Screen Debugging and Status
//
//*******************************
    
	ofSetColor(255, 255, 0);

	string statusPlay		= (string)(isLive ? "LIVE STREAM" : "PLAY STREAM");
	string statusRec		= (string)(!isRecording ? "READY" : "RECORDING");
	string statusSkeleton	= (string)(isTracking ? "TRACKING USERS: " + (string)(isLive ? ofToString(recordUser.getNumberOfTrackedUsers()) : ofToString(playUser.getNumberOfTrackedUsers())) + "" : "NOT TRACKING USERS");
	string statusSmoothSkel = (string)(isLive ? ofToString(recordUser.getSmoothing()) : ofToString(playUser.getSmoothing()));
	string statusHands		= (string)(isTrackingHands ? "TRACKING HANDS: " + (string)(isLive ? ofToString(recordHandTracker.getNumTrackedHands()) : ofToString(playHandTracker.getNumTrackedHands())) + ""  : "NOT TRACKING");
	string statusFilter		= (string)(isFiltering ? "FILTERING" : "NOT FILTERING");
	string statusFilterLvl	= ofToString(filterFactor);
	string statusSmoothHand = (string)(isLive ? ofToString(recordHandTracker.getSmoothing()) : ofToString(playHandTracker.getSmoothing()));
	string statusMask		= (string)(!isMasking ? "HIDE" : (isTracking ? "SHOW" : "YOU NEED TO TURN ON TRACKING!!"));
	string statusCloud		= (string)(isCloud ? "ON" : "OFF");
	string statusCloudData	= (string)(isCPBkgnd ? "SHOW BACKGROUND" : (isTracking ? "SHOW USER" : "YOU NEED TO TURN ON TRACKING!!"));
    
    //new
    string statusCalibrating = (string)(isCalibrating ? "ON" : "OFF");

	string statusHardware;

#ifdef TARGET_OSX // only working on Mac at the moment
	ofPoint statusAccelerometers = hardware.getAccelerometers();
	stringstream	statusHardwareStream;

	statusHardwareStream
	<< "ACCELEROMETERS:"
	<< " TILT: " << hardware.getTiltAngle() << "/" << hardware.tilt_angle
	<< " x - " << statusAccelerometers.x
	<< " y - " << statusAccelerometers.y
	<< " z - " << statusAccelerometers.z;

	statusHardware = statusHardwareStream.str();
#endif

	stringstream msg;

	msg
	<< "    s : start/stop recording  : " << statusRec << endl
	<< "    p : playback/live streams : " << statusPlay << endl
	<< "    t : skeleton tracking     : " << statusSkeleton << endl
	<< "( / ) : smooth skely (openni) : " << statusSmoothSkel << endl
	<< "    m : drawing masks         : " << statusMask << endl
	<< "    d : draw cloud points     : " << statusCloud << endl
	<< "    b : cloud user data       : " << statusCloudData << endl
	<< "- / + : nearThreshold         : " << ofToString(nearThreshold) << endl
	<< "< / > : farThreshold          : " << ofToString(farThreshold) << endl
	<< "    c : calibrate Theremin pos: " << ofToString(statusCalibrating) << endl
	<< "File  : " << oniRecorder.getCurrentFileName() << endl
	<< "FPS   : " << ofToString(ofGetFrameRate()) << "  " << statusHardware << endl;

	ofDrawBitmapString(msg.str(), 20, 560);

}

void testApp:: drawMasks() {
	glPushMatrix();
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
	allUserMasks.draw(640, 0, 640, 480);
	glDisable(GL_BLEND);
    glPopMatrix();
	user1Mask.draw(320, 480, 320, 240);
	user2Mask.draw(640, 480, 320, 240);
	
}

void testApp::drawPointCloud(ofxUserGenerator * user_generator, int userID) {

	glPushMatrix();

	int w = user_generator->getWidth();
	int h = user_generator->getHeight();

	glTranslatef(w, h/2, -500);
	ofRotateY(pointCloudRotationY);

	glBegin(GL_POINTS);

	int step = 1;

	for(int y = 0; y < h; y += step) {
		for(int x = 0; x < w; x += step) {
			ofPoint pos = user_generator->getWorldCoordinateAt(x, y, userID);
			if (pos.z == 0 && isCPBkgnd) continue;	// gets rid of background -> still a bit weird if userID > 0...
			ofColor color = user_generator->getWorldColorAt(x,y, userID);
			glColor4ub((unsigned char)color.r, (unsigned char)color.g, (unsigned char)color.b, (unsigned char)color.a);
			glVertex3f(pos.x, pos.y, pos.z);
		}
	}

	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);

	glPopMatrix();
}


//--------------------------------------------------------------
void testApp::keyPressed(int key){

	float smooth;

	switch (key) {
#ifdef TARGET_OSX // only working on Mac at the moment
		case 357: // up key
			//hardware.setTiltAngle(hardware.tilt_angle++);
            artk.setThreshold(++threshold);
			break;
		case 359: // down key
			//hardware.setTiltAngle(hardware.tilt_angle--);
            artk.setThreshold(--threshold);
			break;
			
#endif
		case 's':
		case 'S':
			if (isRecording) {
				oniRecorder.stopRecord();
				isRecording = false;
				break;
			} else {
				oniRecorder.startRecord(generateFileName());
				isRecording = true;
				break;
			}
			break;
		case 'p':
		case 'P':
			if (oniRecorder.getCurrentFileName() != "" && !isRecording && isLive) {
				setupPlayback(oniRecorder.getCurrentFileName());
				isLive = false;
			} else {
				isLive = true;
			}
			break;
		case 't':
		case 'T':
			isTracking = !isTracking;
			break;
		case 'h':
		case 'H':
			isTrackingHands = !isTrackingHands;
			if(isLive) recordHandTracker.toggleTrackHands();
			if(!isLive) playHandTracker.toggleTrackHands();
			break;
		case 'f':
		case 'F':
			isFiltering = !isFiltering;
			recordHandTracker.isFiltering = isFiltering;
			playHandTracker.isFiltering = isFiltering;
			break;
		case 'm':
		case 'M':
			isMasking = !isMasking;
			recordUser.setUseMaskPixels(isMasking);
			playUser.setUseMaskPixels(isMasking);
			break;
		case 'd':
		case 'D':
			isCloud = !isCloud;
			recordUser.setUseCloudPoints(isCloud);
			playUser.setUseCloudPoints(isCloud);
			break;
		case 'b':
		case 'B':
			isCPBkgnd = !isCPBkgnd;
			break;
        case 'c':
        case 'C':
            isCalibrating = !isCalibrating;
            break;
		case '9':
		case '(':
			smooth = recordUser.getSmoothing();
			if (smooth - 0.1f > 0.0f) {
				recordUser.setSmoothing(smooth - 0.1f);
				playUser.setSmoothing(smooth - 0.1f);
			}
			break;
		case '0':
		case ')':
			smooth = recordUser.getSmoothing();
			if (smooth + 0.1f <= 1.0f) {
				recordUser.setSmoothing(smooth + 0.1f);
				playUser.setSmoothing(smooth + 0.1f);
			}
			break;
		case '[':
		//case '{':
			if (filterFactor - 0.1f > 0.0f) {
				filterFactor = filterFactor - 0.1f;
				recordHandTracker.setFilterFactors(filterFactor);
				if (oniRecorder.getCurrentFileName() != "") playHandTracker.setFilterFactors(filterFactor);
			}
			break;
		case ']':
		//case '}':
			if (filterFactor + 0.1f <= 1.0f) {
				filterFactor = filterFactor + 0.1f;
				recordHandTracker.setFilterFactors(filterFactor);
				if (oniRecorder.getCurrentFileName() != "") playHandTracker.setFilterFactors(filterFactor);
			}
			break;
		case ';':
		case ':':
			smooth = recordHandTracker.getSmoothing();
			if (smooth - 0.1f > 0.0f) {
				recordHandTracker.setSmoothing(smooth -  0.1f);
				playHandTracker.setSmoothing(smooth -  0.1f);
			}
			break;
		case '\'':
		case '\"':
			smooth = recordHandTracker.getSmoothing();
			if (smooth + 0.1f <= 1.0f) {
				recordHandTracker.setSmoothing(smooth +  0.1f);
				playHandTracker.setSmoothing(smooth +  0.1f);
			}
			break;
		case '>':
		case '.':
			farThreshold += 50;
			if (farThreshold > recordDepth.getMaxDepth()) farThreshold = recordDepth.getMaxDepth();
			break;
		case '<':
		case ',':
			farThreshold -= 50;
			if (farThreshold < 0) farThreshold = 0;
			break;

		case '+':
		case '=':
			nearThreshold += 50;
			if (nearThreshold > recordDepth.getMaxDepth()) nearThreshold = recordDepth.getMaxDepth();
			break;

		case '-':
		case '_':
			nearThreshold -= 50;
			if (nearThreshold < 0) nearThreshold = 0;
			break;
		case 'r':
			recordContext.toggleRegisterViewport();
			break;
		default:
			break;
	}
}

string testApp::generateFileName() {

	string _root = "kinectRecord";

	string _timestamp = ofToString(ofGetDay()) +
	ofToString(ofGetMonth()) +
	ofToString(ofGetYear()) +
	ofToString(ofGetHours()) +
	ofToString(ofGetMinutes()) +
	ofToString(ofGetSeconds());

	string _filename = (_root + _timestamp + ".oni");

	return _filename;

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

	if (isCloud) pointCloudRotationY = x;

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

