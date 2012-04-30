/*
 *  Charge.h
 *  eField3d
 *
 *  Created by alex hornbake on 4/10/11.
 *
 */

#include "ofVec3f.h"
#include "ofMain.h"
#include "FieldShift.h"


class Charge : public ofVec3f {

	public:
	
	float magn;
	float ax,bx, ay,by;

	void set(float _x, float _y, float _z, float _magn);
	
	float getNorm();
	float getNorm2();
	void setNorm(float _norm);

	void aff();
	void champ(Charge _charge[], int _nbCharges);
	
	FieldShift fieldshift(Charge _charge[], int _nbCharges);
	
};

