/*
 *  charge.cpp
 *  eField3d
 *
 *  Created by alex hornbake on 4/10/11.
 * 
 *
 */

#include "Charge.h"

//Globals
extern const float intensPas;
extern const float numFieldIterations;
extern const float dMin2;
extern const float lineWeight;
extern ofColor lineColor;
extern bool lineTouched;

void Charge::set(float _x, float _y, float _z, float _magn){
	x = _x;
	y = _y;
	z = _z;
	magn = _magn;
}

float Charge::getNorm(){
	return sqrt(x*x + y*y + z*z);
}

float Charge::getNorm2(){
	return x*x + y*y + z*z;
}

void Charge::setNorm(float _norm){
	x *= _norm/(sqrt(x*x + y*y + z*z));
	y *= _norm/(sqrt(x*x + y*y + z*z));
	z *= _norm/(sqrt(x*x + y*y + z*z));
}

void Charge::aff(){
	
	//draw a little box for a charge
	ofSetColor(0, 255, 255);
    glBegin(GL_QUADS);
	glVertex3f(x, y, z);
	glVertex3f(x, y+3, z);
	glVertex3f(x+3, y+3, z);
	glVertex3f(x+3, y, z);
    glEnd();
}

void Charge::champ(Charge _charge[], int _nbCharges)
{
	bool highQual=true;
	
	ofVec3f newPos;
	newPos.set(0,0,0);
	
	FieldShift fs;
	Charge mobile;
	mobile.set(x,y,z,magn);
	
	if(highQual){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.05f);
		glEnable(GL_LINE_SMOOTH); 
		glHint(GL_LINE_SMOOTH_HINT,GL_FASTEST);
	}
	glLineWidth(lineWeight);
	
	for (int _magn = -1; _magn < 2; _magn +=2)
	{
		mobile.set(x,y,z,_magn);
		mobile.magn=_magn;
		
		for (int i = 0; i < numFieldIterations; i++)
		{
			fs = mobile.fieldshift(_charge,_nbCharges);
			
			if (fs.trapped) 
			{
				newPos.set(_charge[fs.cTrap]);
				break;
			}
            
            float alpher = 255;
            
			/*
			float alpher =255-sqrt(fs.shift.x*fs.shift.x + fs.shift.y*fs.shift.y + fs.shift.z*fs.shift.z)*2;
			
			if ( mobile.getNorm() >250)
			{
				alpher = 255-(mobile.getNorm()-250)*3;
			}
			
			if(alpher <0)
			{
				alpher =0;
			}else if(alpher >255)
			{
				alpher = 255;
			}
			
			if(lineTouched)
			{
				lineColor.a = alpher;
			} else { lineColor.a = alpher/7;}
			*/
			
			
			//shift.setNorm(intensPas);
			fs.shift.x *= intensPas/(sqrt(x*x + y*y + z*z));
			fs.shift.y *= intensPas/(sqrt(x*x + y*y + z*z));
			fs.shift.z *= intensPas/(sqrt(x*x + y*y + z*z));
			
			newPos.set(mobile.x,mobile.y,mobile.z);
			//newPos += fs.shift;
			newPos.set(newPos.x+fs.shift.x,newPos.y+fs.shift.y,newPos.z+fs.shift.z);
			
			glBegin(GL_LINES);
			glColor4ub((unsigned char)lineColor.r,(unsigned char)lineColor.g,(unsigned char)lineColor.b,(unsigned char)lineColor.a);
			glVertex3f(mobile.x, mobile.y, mobile.z);
			glVertex3f(newPos.x, newPos.y, newPos.z);
			glEnd();
			
			//printf("%f, %f, %f, %f \n",mobile.x, mobile.y, mobile.z,mobile.magn);
			
			mobile.set(newPos.x,newPos.y,newPos.z,mobile.magn);
		}
		
		 
	}
}

FieldShift Charge::fieldshift(Charge _charge[], int _nbCharges)
{
	ofVec3f shift;
	shift.set(0,0,0);
	ofVec3f delta;
	ofVec3f vec;
	
	float dist2, intens;
	int cTrap = 0;
	bool trapped = false;
	
	for ( int c = 0; c < _nbCharges; c++)
	{
		delta.set(_charge[c].x,_charge[c].y,_charge[c].z);

		delta.x -= x;
		delta.y -= y;
		delta.z -= z;
		
		//compute normal squared
		dist2 = (delta.x*delta.x + delta.y*delta.y + delta.z*delta.z);
		
		intens = (-_charge[c].magn*magn/dist2);
		
		if (dist2 <dMin2)
		{
			trapped = true;
			cTrap = c;
			break;
		}
		
		vec.set(delta.x,delta.y,delta.z);
		vec *= intens;
		shift += vec;
		
	}
	
	FieldShift _fs (shift,trapped,cTrap);
	
	return _fs;
}


