#include "testApp.h"

//Globals
//----------------------------------

//E Field Quality/Efficiency
extern float intensPas = 10; // Distance between segments
extern float dMin2 = 100;// Last Mile Drawing close to Charges
extern float numFieldIterations = 1600; // How many cycles do we draw for

//E Field Line Style
extern float lineWeight = 1.8;
extern float lineAlpha = 86;
extern float fieldScale = 3;
float touchedDist = 0;
bool lineTouched = false;
ofColor lineColor;

//Theremin Markers
const extern float realMarkerDist=360; //real distance between markers in mm.

//--------------------------------------------------------------
void testApp::setup() {
    
    //ofSetDataPathRoot("./");

    //glEnable (GL_BLEND);
    //glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    ofEnableAlphaBlending();
    
    //Define camera width and height
    width = 640; //ofGetWidth();
	height = 480; //ofGetHeight();
    
	isLive			= true;
	isTracking		= true;
	isCloud			= false;
	isCPBkgnd		= true;
	isMasking		= true;
    isCalibrating   = true;
    isDebug         = false;

	nearThreshold = 500;
	farThreshold  = 1000;

	filterFactor = 0.5f;

	setupRecording();

    colorImage.allocate(width, height);
	grayImage.allocate(width, height);
	grayThres.allocate(width, height);
	
	// This uses the default camera calibration and marker file
	artk.setup(width, height);

	// ARTK+ does the thresholding for us
    threshold=70;
    artk.activateAutoThreshold(true);
    
    //SETUP EFIELD AND CHARGES
	//------------------------
	dbFlag = false;
	cCote = 4;
	rotY = 0.0;
	incRotY = 0.01;
	distZ = 100;
	
	//set Line Color to be WHITE
	lineColor.r=200;
	lineColor.g=255;
	lineColor.b=211;
	lineColor.a=86;
    

	//Setup Charges
    
    //Hands
	charge[0].set(width/2,height/2,0,1000.0);
	charge[1].set(width/2,height/2,0.0,1000.0);

	//ground
	charge[6].set(0,-1000,0,400.0);
	
	for (int i = 0; i < nbPLignes; i++)
	{
		pLigne[i].set(ofRandom(0,width),ofRandom(0,height),(ofRandom(0,0)),1);
	}
    
	ofBackground(0, 0, 0);
    
    marker0.set(0,0,0);
    marker1.set(0,0,0);

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
	recordUser.setMaxNumberOfUsers(8);					// use this to set dynamic max number of users (NB: that a hard upper limit is defined by MAX_NUMBER_USERS in ofxUserGenerator)
    
    
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
		//depthRangeMask.setFromPixels(recordDepth.getDepthPixels(nearThreshold, farThreshold),
		//							 recordDepth.getWidth(), recordDepth.getHeight(), OF_IMAGE_GRAYSCALE);

		// update tracking/recording nodes
		if (isTracking) {
            recordUser.update();
            
        }

		// demo getting pixels from user gen
		
        if (isTracking && isMasking) {
			allUserMasks.setFromPixels(recordUser.getUserPixels(), recordUser.getWidth(), recordUser.getHeight(), OF_IMAGE_GRAYSCALE);
			user1Mask.setFromPixels(recordUser.getUserPixels(1), recordUser.getWidth(), recordUser.getHeight(), OF_IMAGE_GRAYSCALE);
			user2Mask.setFromPixels(recordUser.getUserPixels(2), recordUser.getWidth(), recordUser.getHeight(), OF_IMAGE_GRAYSCALE);
		}
        
        //colorImage.setFromPixels(recordImage.getPixels(), width, height);
		colorImage.setFromPixels(recordImage.getPixels(), recordUser.getWidth(), recordUser.getHeight());
        colorImage.mirror(false,true);
        
        if(isCalibrating)
        {
		// convert our camera image to grayscale
		grayImage = colorImage;
		// apply a threshold so we can see what is going on
		grayThres = grayImage;
		grayThres.threshold(threshold);
		
		// Pass in the new image pixels to artk
		artk.update(grayImage.getPixels());
        }
        
	} 
	
}

//--------------------------------------------------------------
void testApp::draw(){
    
//Scale to make Camera Image Fullscreen
ofScale((float)ofGetWidth()/width,(float)ofGetHeight()/height);
    
ofPushMatrix();    
    
ofSetColor(255, 255, 255);

	if (isLive) {
        //Draw RGB CAM image
		recordImage.draw(0, 0, width, height);
		if (isTracking) {
			
            // DEBUG Draw Skeleton
            if(isDebug) recordUser.draw();
            if(isCalibrating) calibrateThereminPosition();
            trackClosestUser();            
            
            
            // DEBUG Draw Charges
            if(isDebug){
                for(int c = 0; c < nbCharges; c++){
                        charge[c].aff();
                }
            }
            
            //Draw the Magic
            drawEFieldLines();
                
		}
    }
    
ofPopMatrix();    
    
// On Screen Debugging and Status
//--------------------------------------------------------------
if(isDebug){
    
	ofSetColor(255, 255, 0);

	string statusSkeleton	= (string)(isTracking ? "TRACKING USERS: " + (string)(isLive ? ofToString(recordUser.getNumberOfTrackedUsers()) : ofToString(playUser.getNumberOfTrackedUsers())) + "" : "NOT TRACKING USERS");
	string statusMask		= (string)(!isMasking ? "HIDE" : (isTracking ? "SHOW" : "YOU NEED TO TURN ON TRACKING!!"));
    
    //new
    string statusCalibrating = (string)(isCalibrating ? "ON" : "OFF");
    string statusDebug = (string)(isDebug ? "ON" : "OFF");

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
    << "    c : calibrate Theremin pos: " << ofToString(statusCalibrating) << endl
    << "    d : debug toggle          : " << statusDebug << endl
	<< "    m : drawing masks         : " << statusMask << endl
	<< "- / + : nearThreshold         : " << ofToString(nearThreshold) << endl
	<< "< / > : farThreshold          : " << ofToString(farThreshold) << endl
	<< "File  : " << oniRecorder.getCurrentFileName() << endl
	<< "FPS   : " << ofToString(ofGetFrameRate()) << "  " << statusHardware << endl;

	ofDrawBitmapString(msg.str(), 10, 80);
    
    
    stringstream guiMsg;
    
    int r =lineColor.r;
    int g =lineColor.g;
    int b =lineColor.b;
    
	guiMsg
    << "  + / - : Globals can be adjusted up or down using the listed keys" << endl
    << "  1 / Q : Line segment length,   lower is more complex : " << ofToString(intensPas) << endl
    << "  2 / W : Last Mile Drawing,     lower is more complex : " << ofToString(dMin2) << endl
	<< "  3 / E : Number of Iterations, higher is more complex : " << ofToString(numFieldIterations) << endl
	<< "  4 / R : Line Weight                                  : " << ofToString(lineWeight) << endl
	<< "  5 / T : Line Color,                    RED component : " << ofToString(r) << endl
	<< "  6 / Y : Line Color,                  GREEN component : " << ofToString(g) << endl
	<< "  7 / U : Line Color,                   BLUE component : " << ofToString(b) << endl
    << "  8 / I : Line Alpha,                  ALPHA component : " << ofToString(lineAlpha) << endl
	<< "  FPS   : " << ofToString(ofGetFrameRate());
    
	ofDrawBitmapString(guiMsg.str(), 10, 340);
}
}

void testApp::drawEFieldLines(){
    //Draw Field Lines
    for(int i = 0; i < nbPLignes; i++)  {
        //lineColor.a=(charge[0].distance(pLigne[i])/100.0)*25;
        
        pLigne[i].champ(charge,nbCharges);
    }
}

//--------------------------------------------------------------
void testApp::trackClosestUser() {
    ofxTrackedUser* tracked;
    int closestUser = -1;
    float closestUserZ = -1;
    
    for(int i = 0; i < recordUser.getNumberOfTrackedUsers() ; i++){  
        tracked = recordUser.getTrackedUser(i+1);
        
        if( recordUser.getXnUserGenerator().GetSkeletonCap().IsTracking(tracked->id)){  
            if(tracked->neck.found){
                if ( closestUser > 0){
                    if ( tracked->neck.position[0].Z < closestUserZ){
                        closestUserZ = tracked->neck.position[0].Z;
                        closestUser = i+1;
                    }
                }
                else {
                    closestUserZ = tracked->neck.position[0].Z;
                    closestUser = i+1;
                }
            }
            
        }
    }
    
    
    if (closestUser > 0 )
    {
        tracked = recordUser.getTrackedUser(closestUser);
        if(tracked->left_lower_arm.found)
        {
            //Put Charges in your hands
            charge[0].set(tracked->left_lower_arm.position[1].X,tracked->left_lower_arm.position[1].Y,0,1500);
            //Draw Debug Hands
            if (isDebug){
                ofSetHexColor(0xffff77);
                ofCircle(tracked->left_lower_arm.position[1].X,tracked->left_lower_arm.position[1].Y,10);
            }
        }
        if(tracked->right_lower_arm.found)
        {
            //Put Charges in your hands
            charge[1].set(tracked->right_lower_arm.position[1].X,tracked->right_lower_arm.position[1].Y,0,1500);
            //Draw Debug Hands
            if(isDebug){
                ofSetHexColor(0xffff77);
                ofCircle(tracked->right_lower_arm.position[1].X,tracked->right_lower_arm.position[1].Y,10);
            }
        }
    }    
}

//----------------------------------------------------------------------------
//    Theremin Position Calibration
//
//    If in Calibration Mode, then Detect Markers and move charges accordingly
//----------------------------------------------------------------------------
void testApp::calibrateThereminPosition(){
        // Draw Threshold image and allow user to adjust for lighting conditions
        ofSetHexColor(0x555555);
        grayThres.mirror(false,true);
        grayThres.draw(0, 0);
        
        // Display Marker Data nad instruction for calibration.
        ofSetHexColor(0xff0000);  
        ofDrawBitmapString(ofToString(artk.getNumDetectedMarkers()) + " marker(s) found", 10, 20);
        ofDrawBitmapString("Threshold: " + ofToString(threshold), 10, 30);
        ofDrawBitmapString("Use the Up/Down keys to adjust the threshold until both markers are visible.", 10, 40); 
        ofDrawBitmapString("blue circles will apear over the markers when it's good.", 10, 50);
        ofDrawBitmapString("Press the 'c' key when done.", 10,60);
        ofDrawBitmapString("Press the 'd' key to show more debug info.", 10,70);    
        
        // See if marker ID '0' was detected
        // and draw a circle in the center with the ID number.
        int myIndex = artk.getMarkerIndex(0);
        ofPoint center;
        int depth;
        if(myIndex >= 0) {	
            //Get the center of Marker '0'
            center = artk.getDetectedMarkerCenter(myIndex);
            
            //Draw location
            ofSetHexColor(0x000055);
            ofCircle(width-center.x,center.y,30);
            ofSetHexColor(0x00ff00);
            ofDrawBitmapString("0",width-center.x,center.y,10);
            depth = recordDepth.getPixelDepth(width-center.x,center.y);
            ofDrawBitmapString(ofToString(depth),width-center.x,center.y+20,10);
            
            //Store Position
            marker0.set(center.x,center.y,depth);
        }
        
        // See if marker ID '1' was detected
        // and draw a circle in the center with the ID number.
        myIndex = artk.getMarkerIndex(1);
        if(myIndex >= 0) {	
            
            //Get the center of Marker '1'
            center = artk.getDetectedMarkerCenter(myIndex);
            
            //Draw Location
            ofSetHexColor(0x000055);
            ofCircle(width-center.x,center.y,30);
            ofSetHexColor(0x00ff00);
            ofDrawBitmapString("1",width-center.x,center.y,10);
            depth = recordDepth.getPixelDepth(width-center.x,center.y);
            ofDrawBitmapString(ofToString(depth),width-center.x,center.y+20,10);
            
            //Store Position
            marker1.set(center.x,center.y,depth);
        }
      
        //Calculate Distance (pixels) Between Markers
        ofVec2f temp0;
        temp0.set(marker0.x, marker0.y);
        ofVec2f temp1;
        temp1.set(marker1.x,marker1.y);
        ofVec2f temp2 = temp0 - temp1;
        
    float distFactor = abs(temp0.distance(temp1))/realMarkerDist;
    
    //Theremin volume
    charge[2].set(width-marker0.x-distFactor*75,marker0.y-distFactor*50,0,-1000);
    charge[3].set(width-marker0.x-distFactor*200,marker0.y-distFactor*50,0,-1000.0);
    
    //Theremin pitch
    charge[4].set(width-marker1.x+distFactor*50,marker1.y-distFactor*485,0,-1000.0);
    charge[5].set(width-marker1.x+distFactor*50,marker1.y-distFactor*100,0,-1000.0);
  
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

//--------------------------------------------------------------
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
		
		case 'm':
		case 'M':
			isMasking = !isMasking;
			recordUser.setUseMaskPixels(isMasking);
			playUser.setUseMaskPixels(isMasking);
			break;
		case 'd':
		case 'D':
			isDebug = !isDebug;
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
        case '1':
            intensPas += 1; // Distance between segments
            dMin2 = intensPas * intensPas;
            break;
        case 'q':
        case 'Q':
            if (intensPas>1)
            intensPas -= 1;
            dMin2 = intensPas * intensPas;
            break;
        case '2':
            dMin2 +=1;// Last Mile Drawing close to Charges
            break;
        case 'w':
        case 'W':
            if(dMin2>1)
                dMin2-=1;
            break;
        case '3':
            numFieldIterations += 200;
            break;
        case 'e':
            case 'E':
            numFieldIterations -= 200;
            break;
        case '4':
            lineWeight += .1;
            break;
        case 'r':
        case 'R':    
            if(lineWeight>.1)
                lineWeight-=.1;
            break;
        case '5':
            lineColor.set(lineColor.r+1,lineColor.g,lineColor.b);
            break;
        case 't':
        case 'T':
            lineColor.set(lineColor.r-1,lineColor.g,lineColor.b);
            break;
        case '6':
            lineColor.set(lineColor.r,lineColor.g+1,lineColor.b);
            break;
        case 'y':
        case 'Y':
            lineColor.set(lineColor.r,lineColor.g-1,lineColor.b);
            break;
        case '7':
            lineColor.set(lineColor.r,lineColor.g,lineColor.b+1);
            break;
        case 'u':
        case 'U':
            lineColor.set(lineColor.r,lineColor.g,lineColor.b-1);
            break;
        case '8':
            lineAlpha++;
            break;
        case 'i':
        case 'I':
            lineAlpha--;
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

