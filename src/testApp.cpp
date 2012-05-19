#include "testApp.h"
#include "hGui_all.h"

//Globals
//----------------------------------

//E Field Quality/Efficiency
extern const float intensPas = 5; // Distance between segments
extern const float dMin2 = 10;// Last Mile Drawing close to Charges
extern const float numFieldIterations = 1000; // How many cycles do we draw for

//E Field Line Style
extern const float lineWeight = 2;
extern const float fieldScale = 3;
float touchedDist = 0;
bool lineTouched = false;
ofColor lineColor;

//--------------------------------------------------------------
void testApp::setup() {

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

	filterFactor = 0.1f;
    
    //Setup a GUI for debugging
    gui = hGui::getInstance();
    gui->setup("fonts/frabk.ttf", 9);
    
    font = new ofTrueTypeFont;
	font->loadFont("fonts/AvantGarde-Book.ttf", 16, true, true);
	myString = "";
    
    hPanel * mainPanel =
	gui->addPanel("mainPanel", NULL, HGUI_ABSOLUTE_POSITION, 380, 20 , 200, 400, true);
    // addPanel(std::string name, hPanel * parent, int dispMode, int x, int y, int width, int height, bool hasBorder);
    
    mainPanel->setVisibleBackground(true);
    gui->hide();
	// by default, the background of a panel is not visible, so make it visible
    
    // Let our gui engine know what is our root widget (can also be done or changed later)
    // All events will first process the root widget
    // Note: A program can eventually set another root widget during its execution
    
	gui->setRootWidget(mainPanel);
    
	gui->addListeners();
	// void addListeners(void);
    
	
	//----------------------------------------
	// Create 3 sliders:
    
	hSlider* slider1 =
	gui->addSlider("slider1", mainPanel, HGUI_NEXT_ROW, 10, 10, 100);
	hSlider* slider2 =
	gui->addSlider("slider2", mainPanel, HGUI_BOTTOM, 0, -1, 100);
    // ^ start y of new slider has to be end the y of previous
    // else an extra line is showing
    
	hSlider* slider3 =
	gui->addSlider("slider3", mainPanel, HGUI_BOTTOM, 0, -1, 100);
    
    
    
    // Set the range and value of the sliders
	slider1->setRange(0, 255); slider1->setValue(234);
	slider2->setRange(0, 255); slider2->setValue(234);
	slider3->setRange(0, 255); slider3->setValue(234);
    
	// Change the color of the sliders
	slider1->setColor(0xFF0000); // slider1->setBackgroundColor(0xFF8888);
	slider2->setColor(0x00FF00); // slider2->setBackgroundColor(0x88FF88);
	slider3->setColor(0x0000FF); // slider3->setBackgroundColor(0x8888FF);
    
    red = lineColor.r;
    green = lineColor.g;
    blue = lineColor.b;
    slider1->setIntVar(&red);
	slider2->setIntVar(&green);
	slider3->setIntVar(&blue);
    
    
    // Create and initialize the events engine. (It's a singleton too)
    
	hEvents * events = hEvents::getInstance();
	events->setup();
    
	events->addObject("testApp", this);
    
    
    /////////////////////////////////////////////////////////////////////

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
		//depthRangeMask.setFromPixels(recordDepth.getDepthPixels(nearThreshold, farThreshold),
		//							 recordDepth.getWidth(), recordDepth.getHeight(), OF_IMAGE_GRAYSCALE);

		// update tracking/recording nodes
		if (isTracking) {
            recordUser.update();
            
        }

		// demo getting pixels from user gen
		/*
        if (isTracking && isMasking) {
			allUserMasks.setFromPixels(recordUser.getUserPixels(), recordUser.getWidth(), recordUser.getHeight(), OF_IMAGE_GRAYSCALE);
			user1Mask.setFromPixels(recordUser.getUserPixels(1), recordUser.getWidth(), recordUser.getHeight(), OF_IMAGE_GRAYSCALE);
			user2Mask.setFromPixels(recordUser.getUserPixels(2), recordUser.getWidth(), recordUser.getHeight(), OF_IMAGE_GRAYSCALE);
		}
        */
        
        
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
			
            // Draw Debug Skeleton
            if(isDebug) recordUser.draw();
            
            for(int i = 0; i < recordUser.getNumberOfTrackedUsers() ; i++){  
                ofxTrackedUser* tracked = recordUser.getTrackedUser(i+1);
                
                if( recordUser.getXnUserGenerator().GetSkeletonCap().IsTracking(tracked->id)){  
                    if(tracked->left_lower_arm.found){
                        //Put Charges in your hands
                        charge[0].set(tracked->left_lower_arm.position[1].X,tracked->left_lower_arm.position[1].Y,0,1500);
                        
                        //Draw Debug Hands
                        if (isDebug){
                            ofSetHexColor(0xffff77);
                            ofCircle(tracked->left_lower_arm.position[1].X,tracked->left_lower_arm.position[1].Y,10);
                        }
                    }
                    if(tracked->right_lower_arm.found){
                        
                        //Put Charges in your hands
                        charge[1].set(tracked->right_lower_arm.position[1].X,tracked->right_lower_arm.position[1].Y,0,1500);
                        
                        //Draw Debug Hands
                        if(isDebug){
                            ofSetHexColor(0xffff77);
                            ofCircle(tracked->right_lower_arm.position[1].X,tracked->right_lower_arm.position[1].Y,10);
                        }
                    }
                }
                
                
                //Draw Field Lines

                //Draw Charges REMOVE OR MAKE DEBUG ONLY
                if(isDebug){
                    for(int c = 0; c < nbCharges; c++)
                    {
                        charge[c].aff();
                    }
                }
                
                for(int i = 0; i < nbPLignes; i++)
                {
                    pLigne[i].champ(charge,nbCharges);
                }
                
                
                /*
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
                 
                */
            }
		}
        
//****************************************************************************
//    Theremin Position Calibration
//
//    If in Calibration Mode, then Detect Markers and move charges accordingly
//****************************************************************************
        
        if (isCalibrating)
        {
            
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
            ofDrawBitmapString(ofToString(center.x),width-center.x,center.y+30,10);
            ofDrawBitmapString(ofToString(center.y),width-center.x,center.y+40,10);
            
            //Store Position
            marker0.set(center.x,center.y,depth);
            
            //Theremin volume
            charge[2].set(width-marker0.x,marker0.y,0,-1000);
            charge[3].set(width-marker0.x,marker0.y+10,0,-1000.0);


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
            ofDrawBitmapString(ofToString(center.x),width-center.x,center.y+30,10);
            ofDrawBitmapString(ofToString(center.y),width-center.x,center.y+40,10);
            
            //Store Position
            marker1.set(center.x,center.y,depth);
            //Theremin pitch
            charge[4].set(width-marker1.x,marker1.y,0,-1000.0);
            charge[5].set(width-marker1.x,marker1.y+10,0,-1000.0);
        
        }
            
            //Calculate Distance (pixels) Between Markers
            ofVec2f temp0;
            temp0.set(marker0.x, marker0.y);
            ofVec2f temp1;
            temp1.set(marker1.x,marker1.y);
            
            ofVec2f temp2 = temp0 - temp1;
            
            ofDrawBitmapString(ofToString(temp2.x)+ " " + ofToString(temp2.y), width - marker0.x, marker0.y+50);
            
        }

	}
    
ofPopMatrix();    
    
//*******************************
//
// On Screen Debugging and Status
//
//*******************************
    
    
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
	<< "    t : skeleton tracking     : " << statusSkeleton << endl
	<< "    m : drawing masks         : " << statusMask << endl
	<< "- / + : nearThreshold         : " << ofToString(nearThreshold) << endl
	<< "< / > : farThreshold          : " << ofToString(farThreshold) << endl
	<< "File  : " << oniRecorder.getCurrentFileName() << endl
	<< "FPS   : " << ofToString(ofGetFrameRate()) << "  " << statusHardware << endl;

	ofDrawBitmapString(msg.str(), 10, 80);
    }
   // ofTranslate(-ofGetWidth()/4,-ofGetHeight()/4,0);
    
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
//					Methods called by widgets
//					(using prototypes defined in hObject.h)
//--------------------------------------------------------------

void testApp::start(void){
	myString = "start";
}

void testApp::stop(void){
	myString = "stop";
}

void testApp::clear(void){
	myString = "";
}

//--------------------------------------------------------------

void testApp::setValue(double val)
{
	myString = ofToString(val);
}

void testApp::setValueToItem(double val, int index)
{
	myString = ofToString(index) + " =>" + ofToString(val);
}

void testApp::setXY(double x, double y)
{
	myString = ofToString(x) + "\n" + ofToString(y);
	xPct = x; yPct = y;
}

//--------------------------------------------------------------

void testApp::selectItem(int item)
{
	myString = ofToString(item);
}

void testApp::itemSetSelected(int item, bool flag)
{
	if(flag == true)
        myString = ofToString(item) + "(true)";
	else myString = ofToString(item) + "(false)";
}

//--------------------------------------------------------------

void testApp::setLabel(std::string label)
{
	myString = label;
}

void testApp::setText(std::string text)
{
	myString = text; // Not very clever processing...
	// it's just a test
}

void testApp::addText(std::string text)
{
	myString += text; // another possibility, add instead of set text
	// (you have to change setMessage to "addText" to work)
}

void testApp::clearText(void)
{
	myString.clear();
}

void testApp::openItem(int item)
// Open dialogs:
// #1 is a message box
// #2 is an alert
{
	hEvents * events = hEvents::getInstance();
    
	switch(item) {
		case 1:
			events->sendEvent("msgBoxDialog.clear");
			events->sendEvent("msgBoxDialog.display", "line1");
			events->sendEvent("msgBoxDialog.display", "line2");
			events->sendEvent("msgBoxDialog.display", "line3");
			events->sendEvent("msgBoxDialog.display", "line4");
			events->sendEvent("msgBoxDialog.display", "line5");
			events->sendEvent("msgBoxDialog.display", "line6 (scrolling...)");
			events->sendEvent("msgBoxDialog.display", "line7");
            break;
            
		case 2:
			events->sendEvent("alertDialog.clear");
			events->sendEvent("alertDialog.display", "Are you ready?");
            break;
	}
}

void testApp::answerDialog(int buttonID)
// Called to process the answer of the alert dialog
{
	switch(buttonID) {
		case 1: myString = "answer = yes"; break;
		case 2: myString = "answer = no";  break;
	}
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
		
		case 't':
		case 'T':
			isTracking = !isTracking;
			break;
		case 'm':
		case 'M':
			isMasking = !isMasking;
			recordUser.setUseMaskPixels(isMasking);
			playUser.setUseMaskPixels(isMasking);
			break;
		case 'd':
		case 'D':
			isDebug = !isDebug;
            if(isDebug) {
               gui->show(); 
            }
            else gui->hide();
            
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

