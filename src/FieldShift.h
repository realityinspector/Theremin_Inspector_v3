/*
 *  FieldShift.h
 *  eField3d
 *
 *  Created by alex hornbake on 4/10/11.
 *
 */

#include "ofVectorMath.h"
//#include "ofMain.h"
//#include "Charge.h"

class FieldShift {	
public:
	
	ofVec3f shift;
	bool trapped;
	int cTrap;
	
	FieldShift(ofVec3f _shift, bool _trapped, int _cTrap);
	
	FieldShift();
	
};