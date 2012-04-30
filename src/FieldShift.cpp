/*
 *  FieldShift.cpp
 *  eField3d
 *
 *  Created by alex hornbake on 4/10/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "FieldShift.h"

FieldShift::FieldShift(ofVec3f _shift, bool _trapped, int _cTrap){
	shift = _shift;
	trapped = _trapped;
	cTrap = _cTrap;
}

FieldShift::FieldShift(){
	shift.set(0,0,0);
	trapped = 0;
	cTrap = 0;
}