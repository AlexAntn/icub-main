/* 
* Copyright (C) 2010 RobotCub Consortium, European Commission FP6 Project IST-004370
* Author: Serena Ivaldi, Matteo Fumagalli
* email:   serena.ivaldi@iit.it, matteo.fumagalli@iit.it
* website: www.robotcub.org
* Permission is granted to copy, distribute, and/or modify this program
* under the terms of the GNU General Public License, version 2 or any
* later version published by the Free Software Foundation.
*
* A copy of the license can be found at
* http://www.robotcub.org/icub/license/gpl.txt
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
* Public License for more details
*/

#include <iCub/iDyn/iDyn.h>
#include <iCub/ctrl/ctrlMath.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace yarp;
using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::math;
using namespace ctrl;
using namespace iKin;
using namespace iDyn;

//================================
//
//		I DYN LINK
//
//================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLink::iDynLink(double _A, double _D, double _Alpha, double _Offset, double _Min, double _Max)
: iKinLink( _A,  _D,  _Alpha,  _Offset,  _Min,  _Max)
{
	zero();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLink::iDynLink(const double _m, const Matrix &_HC, const Matrix &_I, double _A, double _D, double _Alpha, double _Offset, double _Min, double _Max)
: iKinLink( _A,  _D,  _Alpha,  _Offset,  _Min,  _Max)
{
	zero();
	m = _m;
	setInertia(_I);
	setCOM(_HC);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLink::iDynLink(const double _m, const Vector &_C, const Matrix &_I, double _A, double _D, double _Alpha, double _Offset, double _Min, double _Max)
: iKinLink( _A,  _D,  _Alpha,  _Offset,  _Min,  _Max)
{
	zero();
	m = _m;
	setInertia(_I);
	setCOM(_C);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLink::iDynLink(const double _m, const double _rCx, const double _rCy, const double _rCz, const double Ixx, const double Ixy, const double Ixz, const double Iyy, const double Iyz, const double Izz, double _A, double _D, double _Alpha, double _Offset, double _Min, double _Max)
: iKinLink( _A,  _D,  _Alpha,  _Offset,  _Min,  _Max)
{
	zero();
	m = _m;
	setInertia(Ixx,Ixy,Ixz,Iyy,Iyz,Izz);
	setCOM(_rCx,_rCy,_rCz);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLink::iDynLink(const iDynLink &c)
: iKinLink(c)
{
	clone(c);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLink &iDynLink::operator=(const iDynLink &c)
{
	clone(c);
	return *this;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLink::clone(const iDynLink &c)
{
	iKinLink::clone(c);

	m = c.getMass();
	I = c.getInertia();
	HC = c.getCOM();
	Im = c.getIm();
	Fv = c.getFv();
	Fs = c.getFs();
	kr = c.getKr();
	dq = c.getDAng();
	ddq = c.getD2Ang();
	w = c.getW();
	dw = c.getdW();
	dwM = c.getdWM();
	dp = c.getLinVel();
	dpC = c.getLinVelC();
	ddp = c.getLinAcc();
	ddpC = c.getLinAccC();
	F = c.getForce();
	Mu = c.getMoment();
	Tau = c.getTorque();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


	//~~~~~~~~~~~~~~~~~~~~~~
	//   set methods
	//~~~~~~~~~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLink::setDynamicParameters(const double _m, const yarp::sig::Matrix &_HC, const yarp::sig::Matrix &_I, const double _kr, const double _Fv, const double _Fs, const double _Im)
{
	m = _m;
	kr = _kr;
	Fv = _Fv;
	Fs = _Fs;
	Im = _Im;
	return setInertia(_I) && setCOM(_HC);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLink::setDynamicParameters(const double _m, const yarp::sig::Matrix &_HC, const yarp::sig::Matrix &_I)
{
	m = _m;
	return setInertia(_I) && setCOM(_HC);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLink::setStaticParameters(const double _m,  const yarp::sig::Matrix &_HC)
{
	m = _m;
	return setCOM(_HC);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLink::setInertia(const yarp::sig::Matrix &_I)
{
	if( (_I.rows()==3)&&(_I.cols()==3) )
	{
		I = _I;
		return true;
	}
	else
	{
		I.resize(3,3); I.zero();
		if(verbose)
			cerr<<"iDynLink: error in setting Inertia due to wrong matrix size: ("
			<<_I.rows()<<","<<_I.cols()<<") instead of (3,3). Inertia matrix now set automatically to zero."<<endl;
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLink::setInertia(const double Ixx, const double Ixy, const double Ixz, const double Iyy, const double Iyz, const double Izz)
{
	I.resize(3,3); I.zero();
	I(0,0) = Ixx;
	I(0,1) = I(1,0) = Ixy;
	I(0,2) = I(2,0) = Ixz;
	I(1,1) = Iyy;
	I(1,2) = I(2,1) = Iyz;
	I(2,2) = Izz;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLink::setMass(const double _m)
{
	m = _m;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double iDynLink::setDAng(const double _dteta)
{
	dq = _dteta;
	return dq;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double iDynLink::setD2Ang(const double _ddteta)
{
	ddq = _ddteta;
	return ddq;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLink::setAngPosVelAcc(const double _teta,const double _dteta,const double _ddteta)
{
	setAng(_teta);
	setDAng(_dteta);
	setD2Ang(_ddteta);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLink::setCOM(const yarp::sig::Matrix &_HC)
{
	if((_HC.rows()==4) && (_HC.cols()==4))
	{
		HC = _HC;
		return true;
	}
	else
	{
		HC.resize(4,4); HC.eye();
		if(verbose)
			cerr<<"iDynLink: error in setting COM roto-translation due to wrong matrix size: ("
			<<_HC.rows()<<","<<_HC.cols()<<") instead of (4,4). HC matrix now set automatically as eye."<<endl;
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLink::setCOM(const yarp::sig::Vector &_rC)
{
	if(_rC.length()==3)
	{
		HC.resize(4,4); HC.eye();
		HC(0,3) = _rC(0);
		HC(1,3) = _rC(1);
		HC(2,3) = _rC(2);
		return true;
	}
	else	
	{
		if(verbose)
			cerr<<"iDynLink error, cannot set distance from COM due to wrong sized vector: "<<_rC.length()<<" instead of 3"<<endl;
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLink::setCOM(const double _rCx, const double _rCy, const double _rCz)
{
	HC.resize(4,4); HC.eye();
	HC(0,3) = _rCx;
	HC(1,3) = _rCy;
	HC(2,3) = _rCz;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLink::setForce(const yarp::sig::Vector &_F)
{
	if( _F.length()==3)	
	{
		F = _F;
		return true;
	}
	else
	{
		if(verbose)
			cerr<<"iDynLink error, cannot set forces due to wrong size: "<<_F.length()<<" instead of 3"<<endl;
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLink::setMoment(const yarp::sig::Vector &_Mu)
{
	if(_Mu.length()==3)
	{
		Mu = _Mu;
		return true;
	}
	else
	{
		if(verbose)
			cerr<<"iDynLink error, cannot set moments due to wrong size: "<<_Mu.length()<<" instead of 3"<<endl;
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLink::setForceMoment(const yarp::sig::Vector &_F, const yarp::sig::Vector &_Mu)
{
	return setForce(_F) && setMoment(_Mu);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLink::setTorque(const double _Tau)
{
	Tau = _Tau;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLink::zero()
{
	m = 0.0;
	dq = 0.0; ddq = 0.0;
	I.resize(3,3);	I.zero();
	w.resize(3);	w.zero();
	dw.resize(3);	dw.zero();
	dwM.resize(3);	dwM.zero();
	dp.resize(3);	dp.zero();
	dpC.resize(3);	dpC.zero();
	ddp.resize(3);	ddp.zero();
	ddpC.resize(3);	ddpC.zero();
	F.resize(3);	F.zero();
	Mu.resize(3);	Mu.zero();
	Tau = 0.0;
	Im = 0.0; kr = 0.0;	Fv = 0.0;	Fs = 0.0;	
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	 //~~~~~~~~~~~~~~~~~~~~~~
	 //   get methods
	 //~~~~~~~~~~~~~~~~~~~~~~
	     
Matrix	iDynLink::getInertia()	const	{return I;}
double	iDynLink::getMass()		const	{return m;}
double	iDynLink::getIm()		const	{return Im;}
double	iDynLink::getKr()		const	{return kr;}
double	iDynLink::getFs()		const	{return Fs;}
double	iDynLink::getFv()		const	{return Fv;}
Matrix	iDynLink::getCOM()		const	{return HC;}
double	iDynLink::getDAng()		const	{return dq;}
double	iDynLink::getD2Ang()	const	{return ddq;}
Vector	iDynLink::getW()		const	{return w;}
Vector	iDynLink::getdW()		const	{return dw;}
Vector	iDynLink::getdWM()		const	{return dwM;}
Vector  iDynLink::getLinVel()   const   {return dp;}
Vector  iDynLink::getLinVelC()	const	{return dpC;}
Vector	iDynLink::getLinAcc()	const	{return ddp;}
Vector	iDynLink::getLinAccC()	const	{return ddpC;}
Vector	iDynLink::getForce()	const	{return F;}
Vector	iDynLink::getMoment()	const	{return Mu;}
double	iDynLink::getTorque()	const	{return Tau;}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix	iDynLink::getR()			{return (getH(true).submatrix(0,2,0,2));}
Matrix	iDynLink::getRC()			{return getCOM().submatrix(0,2,0,2);}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector	iDynLink::getr(bool proj) 	
{
	if(proj==false)
		return getH(true).submatrix(0,2,0,3).getCol(3);
	else
		return (-1.0 * getR().transposed() * getH(true).submatrix(0,2,0,3).getCol(3));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector	iDynLink::getrC(bool proj) 	
{
	if(proj==false)
		return getCOM().submatrix(0,2,0,3).getCol(3);
	else
		return (-1.0 * getRC().transposed() * getCOM().submatrix(0,2,0,3).getCol(3));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



//================================
//
//		I DYN CHAIN
//
//================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynChain::iDynChain()
: iKinChain()
{
	NE=NULL;
	setIterMode(KINFWD_WREBWD);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynChain::iDynChain(const Matrix &_H0)
:iKinChain(_H0)
{
	NE=NULL;
	setIterMode(KINFWD_WREBWD);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::clone(const iDynChain &c)
{
	iKinChain::clone(c);
	curr_dq = c.curr_dq;
	curr_ddq = c.curr_ddq;
	iterateMode_kinematics = c.iterateMode_kinematics;
	iterateMode_wrench = c.iterateMode_wrench;
	NE = c.NE;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::build()
{
	iKinChain::build();
	if(DOF)
	{
		curr_dq = getDAng();
		curr_ddq = getD2Ang();
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynChain::iDynChain(const iDynChain &c)
{
    clone(c);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::dispose()
{
	iKinChain::dispose();
	if(NE)
	{
		delete NE;
		NE=NULL;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynChain::~iDynChain()
{
	dispose();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynChain &iDynChain::operator=(const iDynChain &c)
{
    clone(c);
    return *this;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::setDAng(const Vector &dq)
{
    if(!DOF)
        return Vector(0);

    curr_dq.resize(DOF);

    if(dq.length()>=(int)DOF)
	{
		for(unsigned int i=0; i<DOF; i++)
            curr_dq[i]=quickList[hash_dof[i]]->setDAng(dq[i]);
	}
    else 
		if(verbose)
        cerr<<"setVel() failed: " << DOF << " joint angles needed" <<endl;

    return curr_dq;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::setD2Ang(const Vector &ddq)
{
    if(!DOF)
        return Vector(0);

    curr_ddq.resize(DOF);

    if(ddq.length()>=(int)DOF)      
	{
		for(unsigned int i=0; i<DOF; i++)
            curr_ddq[i]=quickList[hash_dof[i]]->setD2Ang(ddq[i]);
	}
    else 
		if(verbose)
        cerr<<"setVel() failed: " << DOF << " joint angles needed" <<endl;

    return curr_ddq;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getDAng()
{
    if(!DOF)
        return Vector(0);

    curr_dq.resize(DOF);

    for(unsigned int i=0; i<DOF; i++)
        curr_dq[i]=quickList[hash_dof[i]]->getDAng();

    return curr_dq;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getD2Ang()
{
	if(!DOF)
       return Vector(0);

    curr_ddq.resize(DOF);

    for(unsigned int i=0; i<DOF; i++)
        curr_ddq[i]=quickList[hash_dof[i]]->getD2Ang();

    return curr_ddq;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double iDynChain::setDAng(const unsigned int i, double _dq)
{
    if(i<N)
        return allList[i]->setDAng(_dq);
    else 
	{	if(verbose)
		{
			cerr<<"setVel() failed due to out of range index: ";
			cerr<<i<<">="<<N<<endl;
		}
		return 0.0;
		
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double iDynChain::setD2Ang(const unsigned int i, double _ddq)
{
    if(i<N)
         return allList[i]->setD2Ang(_ddq);
    else 
    {
		if(verbose)
		{
			cerr<<"setAcc() failed due to out of range index: ";
			cerr<<i<<">="<<N<<endl;
		}
		return 0.0;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double iDynChain::getDAng(const unsigned int i)
{
    if(i<N)
        return allList[i]->getDAng();
    else 
    {
		if(verbose)
		{
			cerr<<"getVel() failed due to out of range index: ";
			cerr<<i<<">="<<N<<endl;
		}
		return 0.0;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double iDynChain::getD2Ang(const unsigned int i)
{
    if(i<N)
        return allList[i]->getD2Ang();
    else 
    {
		if(verbose)
		{
			cerr<<"getAcc() failed due to out of range index: ";
			cerr<<i<<">="<<N<<endl;
		}
		return 0.0;
    }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getLinAcc(const unsigned int i) const
{
    if(i<N)
        return allList[i]->getLinAcc();
    else 
    {
		if(verbose)
		{
			cerr<<"getLinAcc() failed due to out of range index: ";
			cerr<<i<<">="<<N<<endl;
		}
		return Vector(0);
    }	
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getLinAccCOM(const unsigned int i) const
{
    if(i<N)
		return allList[i]->getLinAccC();
    else 
    {
		if(verbose)
		{
			cerr<<"getLinAccCOM() failed due to out of range index: ";
			cerr<<i<<">="<<N<<endl;
		}
		return Vector(0);
    }	
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLink * iDynChain::refLink(const unsigned int i)
{
	if(i<N)
		return dynamic_cast<iDynLink *>(allList[i]);
	else 
	{
		if(verbose)
		{
			cerr<<"getAcc() failed due to out of range index: ";
			cerr<<i<<">="<<N<<endl;
		}
		return NULL;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getForce(const unsigned int iLink)	const		
{
	if(iLink<N)
		return allList[iLink]->getForce();
	else 
	{
		if(verbose)
		{
			cerr<<"getForce() failed due to out of range index: "
				<<iLink<<">="<<N<<endl;
		}
		return Vector(0);
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getMoment(const unsigned int iLink) const	
{
	if(iLink<N)
		return allList[iLink]->getMoment();
	else 
	{
		if(verbose)
		{
			cerr<<"getMoment() failed due to out of range index: ";
			cerr<<iLink<<">="<<N<<endl;
		}
		return Vector(0);
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double iDynChain::getTorque(const unsigned int iLink) const	
{
	if(iLink<N)
		return allList[iLink]->getTorque();
	else 
	{
		if(verbose)
		{
			cerr<<"getTorque() failed due to out of range index: ";
			cerr<<iLink<<">="<<N<<endl;
		}
		return 0.0;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getMasses() const
{
	Vector ret(N); ret.zero();	
	for(unsigned int i=0;i<N;i++)
		ret[i] = allList[i]->getMass();
	return ret;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::setMasses(Vector _m) 
{
	if(_m.length()==N)
	{
		for(unsigned int i=0; i<N; i++)
			allList[i]->setMass(_m[i]);
		return true;
	}
	else
	{
		if(verbose)
			cerr<<"iDynChain: setMasses() failed due to wrong vector size: "
				<<_m.length()<<" instead of "<<N<<endl;
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
double iDynChain::getMass(const unsigned int i) const
{
	if(i<N)
		return allList[i]->getMass();
	else
	{
		if(verbose)
			cerr<<"iDynChain: getMass() failed due to out of range index: "
				<<i<<">="<<N<<endl;
		return 0.0;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::setMass(const unsigned int i, const double _m)
{
	if(i<N)
	{
		allList[i]->setMass(_m);
		return true;
	}
	else
	{
		if(verbose)
			cerr<<"iDynChain: setMass() failed due to out of range index: "
				<<i<<">="<<N<<endl;
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix iDynChain::getForces() const
{
	Matrix ret(3,N);
	for(unsigned int i=0;i<N;i++)
		ret.setCol(i,allList[i]->getForce());
	return ret;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix iDynChain::getMoments() const
{
	Matrix ret(3,N);
	for(unsigned int i=0;i<N;i++)
		ret.setCol(i,allList[i]->getMoment());
	return ret;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getTorques() const
{
	Vector ret(N);
	for(unsigned int i=0;i<N;i++)
		ret[i]= allList[i]->getTorque();
	return ret;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::setDynamicParameters(const unsigned int i, const double _m, const Matrix &_HC, const Matrix &_I, const double _kr, const double _Fv, const double _Fs, const double _Im)
{
	if(i<N)
		return 	allList[i]->setDynamicParameters(_m,_HC,_I,_kr,_Fv,_Fs,_Im);	
	else
	{
		if(verbose)
		{
			cerr<<"iDynChain: setDynamicParameters() failed due to out of range index: "
				<<i<<">="<<N<<endl;
		}
		return false;
	}

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::setDynamicParameters(const unsigned int i, const double _m, const Matrix &_HC, const Matrix &_I)
{
	if(i<N)
		return allList[i]->setDynamicParameters(_m,_HC,_I);
	else
	{
		if(verbose)
		{
			cerr<<"iDynChain: setDynamicParameters() failed due to out of range index: "
				<<i<<">="<<N<<endl;
		}
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::setStaticParameters(const unsigned int i, const double _m, const Matrix &_HC)
{
	if(i<N)
		return allList[i]->setStaticParameters(_m,_HC);
	else
	{
		if(verbose)
		{
			cerr<<"iDynChain: setStaticParameters() failed due to out of range index: "
				<<i<<">="<<N<<endl;
		}
		return false;
	}

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::prepareNewtonEuler(const NewEulMode NewEulMode_s)
{
	string info;
	info = "[Chain] ";
	char buffer[60]; 
	int j = sprintf(buffer,"DOF=%d N=%d",DOF,N);
	info.append(buffer);

	if( NE == NULL)
		NE = new OneChainNewtonEuler(const_cast<iDynChain *>(this),info,NewEulMode_s,verbose);
	else
	{
		delete NE;
		NE = new OneChainNewtonEuler(const_cast<iDynChain *>(this),info,NewEulMode_s,verbose);
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::computeNewtonEuler(const Vector &w0, const Vector &dw0, const Vector &ddp0, const Vector &F0, const Vector &Mu0 )
{ 
	if( NE == NULL)
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call computeNewtonEuler() without having prepared Newton-Euler method in the class. "<<endl
			<<"iDynChain: prepareNewtonEuler() called autonomously in the default mode. "<<endl;
		prepareNewtonEuler();
	}

	if((w0.length()==3)&&(dw0.length()==3)&&(ddp0.length()==3)&&(F0.length()==3)&&(Mu0.length()==3))
	{
		if(iterateMode_kinematics == FORWARD)	
			NE->ForwardKinematicFromBase(w0,dw0,ddp0);
		else 
			NE->BackwardKinematicFromEnd(w0,dw0,ddp0);

		if(iterateMode_wrench == BACKWARD)	
			NE->BackwardWrenchFromEnd(F0,Mu0);
		else 
			NE->ForwardWrenchFromBase(F0,Mu0);
		return true;
	}
	else
	{
		if(verbose)
		{
			cerr<<"iDynChain error: could not compute with Newton Euler due to wrong sized initializing vectors: "
				<<" w0,dw0,ddp0,Fend,Muend have size "
				<< w0.length() <<","<< dw0.length() <<","
				<< ddp0.length() <<","<< F0.length() <<","<< Mu0.length() <<","
				<<" instead of 3,3,3,3,3"<<endl;
		}
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::computeNewtonEuler()
{ 
	if( NE == NULL)
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call computeNewtonEuler() without having prepared Newton-Euler method in the class. "<<endl
				<<"iDynChain: prepareNewtonEuler() called autonomously in the default mode. "<<endl
				<<"iDynChain: initNewtonEuler() called autonomously with default values. "<<endl;
		prepareNewtonEuler();
		initNewtonEuler();
	}

	if(iterateMode_kinematics == FORWARD)	
		NE->ForwardKinematicFromBase();
	else 
		NE->BackwardKinematicFromEnd();

	if(iterateMode_wrench == BACKWARD)	
		NE->BackwardWrenchFromEnd();
	else 
		NE->ForwardWrenchFromBase();

	return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::computeKinematicNewtonEuler()
{
	if(iterateMode_kinematics == FORWARD)	
		NE->ForwardKinematicFromBase();
	else 
		NE->BackwardKinematicFromEnd();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::computeWrenchNewtonEuler()
{
	if(iterateMode_wrench == BACKWARD)	
		NE->BackwardWrenchFromEnd();
	else 
		NE->ForwardWrenchFromBase();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::getKinematicNewtonEuler(Vector &w, Vector &dw, Vector &ddp)
{
	if( NE == NULL)
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call getKinematicNewtonEuler() without having prepared Newton-Euler method in the class. "<<endl
				<<"iDynChain: prepareNewtonEuler() called autonomously in the default mode. "<<endl
				<<"iDynChain: initNewtonEuler() called autonomously with default values. "<<endl;
		prepareNewtonEuler();
		initNewtonEuler();
	}
	
	w.resize(3); dw.resize(3); ddp.resize(3); w=dw=ddp=0.0;
	if(iterateMode_kinematics == FORWARD)	
	{
		//get kinematics from the end-effector
		NE->getVelAccEnd(w,dw,ddp);
	}
	else 
	{
		//get kinematics from the base
		NE->getVelAccBase(w,dw,ddp);
	}

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::getFrameKinematic(unsigned int i, Vector &w, Vector &dw, Vector &ddp)
{
	Vector dwM(3);dwM=0.0;
	Vector ddpC(3);ddpC=0.0;
	NE->getVelAccAfterForward(i,w,dw,dwM,ddp,ddpC);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::getFrameWrench(unsigned int i, Vector &F, Vector &Mu)
{
	NE->getWrenchAfterForward(i,F,Mu);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::getWrenchNewtonEuler(Vector &F, Vector &Mu) 
{
	if( NE == NULL)
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call getWrenchNewtonEuler() without having prepared Newton-Euler method in the class. "<<endl
				<<"iDynChain: prepareNewtonEuler() called autonomously in the default mode. "<<endl
				<<"iDynChain: initNewtonEuler() called autonomously with default values. "<<endl;
		prepareNewtonEuler();
		initNewtonEuler();
	}
	F.resize(3); Mu.resize(3); F=Mu=0.0;
	if(iterateMode_wrench == BACKWARD)			
	{
		//get wrench from the base
		NE->getWrenchBase(F,Mu);
	}
	else
	{
		//get wrench from the end-effector
		NE->getWrenchEnd(F,Mu);
	}

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::initNewtonEuler()
{
	Vector w0(3); w0.zero();
	Vector dw0(3); dw0.zero();
	Vector ddp0(3); ddp0.zero();
	Vector Fend(3); Fend.zero();
	Vector Muend(3); Muend.zero();

	return initNewtonEuler(w0,dw0,ddp0,Fend,Muend);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::initNewtonEuler(const Vector &w0, const Vector &dw0, const Vector &ddp0, const Vector &Fend, const Vector &Muend)
{
	if( NE == NULL)
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call initNewtonEuler() without having prepared Newton-Euler method in the class. "<<endl
				<<"iDynChain: prepareNewtonEuler() called autonomously in the default mode. "<<endl;
		prepareNewtonEuler();
	}

	if((w0.length()==3)&&(dw0.length()==3)&&(ddp0.length()==3)&&(Fend.length()==3)&&(Muend.length()==3))
	{
		bool ret=true;

		if(iterateMode_kinematics == FORWARD)	
			ret = ret && NE->initKinematicBase(w0,dw0,ddp0);
		else 
			ret = ret && NE->initKinematicEnd(w0,dw0,ddp0);

		if(iterateMode_wrench == BACKWARD)	
			ret = ret && NE->initWrenchEnd(Fend,Muend);
		else 
			ret = ret && NE->initWrenchBase(Fend,Muend);

		return ret;
	}
	else
	{
		if(verbose)
		{
			cerr<<"iDynChain error: could not initialize Newton Euler due to wrong sized initializing vectors: "
				<<" w0,dw0,ddp0,Fend,Muend have size "
				<< w0.length() <<","<< dw0.length() <<","
				<< ddp0.length() <<","<< Fend.length() <<","<< Muend.length() <<","
				<<" instead of 3,3,3,3,3"<<endl;
		}
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::initKinematicNewtonEuler(const Vector &w0, const Vector &dw0, const Vector &ddp0)
{
	if(NE == NULL)
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call initKinematicNewtonEuler() without having prepared Newton-Euler method in the class. "<<endl
				<<"iDynChain: prepareNewtonEuler() called autonomously in the default mode. "<<endl;
		prepareNewtonEuler();
	}

	if((w0.length()==3)&&(dw0.length()==3)&&(ddp0.length()==3))
	{
		if(iterateMode_kinematics == FORWARD)	
			return NE->initKinematicBase(w0,dw0,ddp0);
		else 
			return NE->initKinematicEnd(w0,dw0,ddp0);
	}
	else
	{
		if(verbose)
		{
			cerr<<"iDynChain error: could not initialize Newton Euler due to wrong sized initializing vectors: "
				<<" w0,dw0,ddp0 have size "
				<< w0.length() <<","<< dw0.length() <<","
				<< ddp0.length() <<","
				<<" instead of 3,3,3"<<endl;
		}
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::initWrenchNewtonEuler(const Vector &Fend, const Vector &Muend)
{
	if( NE == NULL)
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call initWrenchNewtonEuler() without having prepared Newton-Euler method in the class. "<<endl
				<<"iDynChain: prepareNewtonEuler() called autonomously in the default mode. "<<endl;
		prepareNewtonEuler();
	}

	if((Fend.length()==3)&&(Muend.length()==3))
	{
		if(iterateMode_wrench == BACKWARD)	
			return NE->initWrenchEnd(Fend,Muend);
		else 
			return NE->initWrenchBase(Fend,Muend);
	}
	else
	{
		if(verbose)
		{
			cerr<<"iDynChain error: could not initialize Newton Euler due to wrong sized initializing vectors: "
				<<" Fend,Muend have size "
				<< Fend.length() <<","<< Muend.length() <<","
				<<" instead of 3,3"<<endl;
		}
		return false;
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::setModeNewtonEuler(const NewEulMode mode)
{
	if( NE == NULL)
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call setModeNewtonEuler() without having prepared Newton-Euler method in the class. "<<endl
				<<"iDynChain: prepareNewtonEuler() called autonomously in the default mode. "<<endl;
		prepareNewtonEuler();
	}

	NE->setMode(mode);
	if(verbose)
		cerr<<"iDynChain: Newton-Euler mode set to "<<NewEulMode_s[mode]<<endl;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

			//~~~~~~~~~~~~~~
			//	plus get
			//~~~~~~~~~~~~~~

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix iDynChain::getForcesNewtonEuler() const
{
	Matrix ret(3,N+2); ret.zero();
	if( NE != NULL)
	{
		for(unsigned int i=0;i<N+2;i++)
			ret.setCol(i,NE->neChain[i]->getForce());
	}
	else
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call getForcesNewtonEuler() without having prepared Newton-Euler."<<endl;
	}

	return ret;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix iDynChain::getMomentsNewtonEuler() const
{
	Matrix ret(3,N+2); ret.zero();
	if( NE != NULL)
	{
		for(unsigned int i=0;i<N+2;i++)
			ret.setCol(i,NE->neChain[i]->getMoment());
	}
	else
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call getMomentsNewtonEuler() without having prepared Newton-Euler."<<endl;
	}
	return ret;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getTorquesNewtonEuler() const
{
	Vector ret(N); ret.zero();
	if( NE != NULL)
	{
		for(unsigned int i=0;i<N;i++)
			ret[i] = NE->neChain[i+1]->getTorque();
	}
	else
	{
		if(verbose)
			cerr<<"iDynChain error: trying to call getTorquesNewtonEuler() without having prepared Newton-Euler."<<endl;
	}
	return ret;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector iDynChain::getForceMomentEndEff() const
{
	Vector ret(6); ret.zero();
	Vector f = allList[N-1]->getForce();
	Vector m = allList[N-1]->getMoment();
	ret[0]=f[0]; ret[1]=f[1]; ret[2]=f[2];
	ret[3]=m[0]; ret[4]=m[1]; ret[5]=m[2];
	return ret;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::setIterModeKinematic(const ChainIterationMode _iterateMode_kinematics) 
{
	iterateMode_kinematics = _iterateMode_kinematics;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::setIterModeWrench(const ChainIterationMode _iterateMode_wrench) 
{
	iterateMode_wrench = _iterateMode_wrench;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ChainIterationMode iDynChain::getIterModeKinematic() const
{
	return iterateMode_kinematics;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ChainIterationMode iDynChain::getIterModeWrench() const
{
	return iterateMode_wrench;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynChain::setIterMode(const ChainComputationMode mode)
{
	switch(mode)
	{
	case KINFWD_WREBWD: setIterModeKinematic(FORWARD); setIterModeWrench(BACKWARD);
		break;
	case KINFWD_WREFWD: setIterModeKinematic(FORWARD); setIterModeWrench(FORWARD);
		break;
	case KINBWD_WREBWD: setIterModeKinematic(BACKWARD); setIterModeWrench(BACKWARD);
		break;
	case KINBWD_WREFWD: setIterModeKinematic(BACKWARD); setIterModeWrench(FORWARD);
		break;
	default:
		if(verbose)
			cerr<<"iDynChain error: in setIterMode() could not set iteration mode due to unexisting mode"<<endl; 
	}
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix iDynChain::getH0() const
{
 return H0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynChain::setH0(const Matrix &_H0)
{
 if((_H0.cols()==4)&&(_H0.rows()==4))
 {
  H0 = _H0;
  return true;
 }
 else
 {
  if(verbose) cerr<<"iDynChain: could not set H0 due to wrong sized matrix: "
      <<_H0.rows()<<"x"<<_H0.cols()<<" instead of 4x4."<<endl;
  return false;
 }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	//-----------
	//  jacobian
	//-----------

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix iDynChain::computeGeoJacobian(const unsigned int iLinkN, const Matrix &Pn)
{
	if(DOF==0)
    {
		if(verbose)cerr<<"iDynChain: computeGeoJacobian() failed since DOF==0"<<endl;
        return Matrix(0,0);
    }
    if(iLinkN>=N)
    {
		if(verbose) cerr<<"iDynChain: computeGeoJacobian() failed due to out of range indexes: "
						<<"from 0 to "<<iLinkN<<" >= "<<N<<endl;
        return Matrix(0,0);
    }

	// the jacobian size is linkN+1: eg, index=2, Njoints=0,1,2=3
    Matrix J(6, iLinkN+1 );J.zero();
    Matrix Z;
    Vector w;

    deque<Matrix> intH;
    intH.push_back(H0);
    for (unsigned int i=0; i<iLinkN; i++)
        intH.push_back(intH[i]*allList[i]->getH(true));

    for (unsigned int i=0; i<iLinkN; i++)
    {
		unsigned int j=hash[i];
        Z=intH[j];
        w=cross(Z,2,Pn-Z,3,verbose);
        J(0,j)=w[0];
        J(1,j)=w[1];
        J(2,j)=w[2];
        J(3,j)=Z(0,2);
        J(4,j)=Z(1,2);
        J(5,j)=Z(2,2);
    }
    return J;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix iDynChain::computeGeoJacobian(const Matrix &Pn)
{
	if (DOF==0)
    {
		if(verbose)cerr<<"iDynChain: computeGeoJacobian() failed since DOF==0"<<endl;
        return Matrix(0,0);
    }

	//the jacobian is the same of iKin, but Pn is an input
	Matrix J(6,DOF); J.zero();
    Matrix Z;
    Vector w;

    deque<Matrix> intH;
    intH.push_back(H0);
    for(unsigned int i=0; i<N; i++)
        intH.push_back(intH[i]*allList[i]->getH(true));

    for(unsigned int i=0; i<DOF; i++)
    {
        unsigned int j=hash[i];
        Z=intH[j];
        w=cross(Z,2,Pn-Z,3,verbose);
        J(0,i)=w[0];
        J(1,i)=w[1];
        J(2,i)=w[2];
        J(3,i)=Z(0,2);
        J(4,i)=Z(1,2);
        J(5,i)=Z(2,2);
    }
    return J;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix iDynChain::computeGeoJacobian(const Matrix &Pn, const Matrix &_H0)
{
	if (DOF==0)
    {
		if(verbose)cerr<<"iDynChain: computeGeoJacobian() failed since DOF==0"<<endl;
        return Matrix(0,0);
    }

	//the jacobian is the same of iKin, but Pn is an input
	Matrix J(6,DOF); J.zero();
    Matrix Z;
    Vector w;

    deque<Matrix> intH;
    intH.push_back(_H0);
    for(unsigned int i=0; i<N; i++)
        intH.push_back(intH[i]*allList[i]->getH(true));

    for(unsigned int i=0; i<DOF; i++)
    {
        unsigned int j=hash[i];
        Z=intH[j];
        w=cross(Z,2,Pn-Z,3,verbose);
        J(0,i)=w[0];
        J(1,i)=w[1];
        J(2,i)=w[2];
        J(3,i)=Z(0,2);
        J(4,i)=Z(1,2);
        J(5,i)=Z(2,2);
    }
    return J;
}


//================================
//
//		I DYN LIMB
//
//================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLimb::iDynLimb()
{
    allocate("right");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLimb::iDynLimb(const string &_type)
{
    allocate(_type);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLimb::iDynLimb(const iDynLimb &limb)
{
    clone(limb);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLimb::iDynLimb(const Property &option)
{
    fromLinksProperties(option);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool iDynLimb::fromLinksProperties(const Property &option)
{
    Property &opt=const_cast<Property&>(option);
    dispose();   
	int i,j;
	Matrix I(3,3);
	Matrix HC(4,4);

	// type: left/right
    type=opt.check("type",Value("right")).asString().c_str();
    if(type!="right" && type!="left")
    {
        cerr<<"Error: invalid handedness type specified!" <<endl;
        return false;
    }

	// H0 matrix
    if(Bottle *bH0=opt.find("H0").asList())
    {
        i=0;
        j=0;
        H0.zero();
        for(int cnt=0; (cnt<bH0->size()) && (cnt<H0.rows()*H0.cols()); cnt++)
        {    
            H0(i,j)=bH0->get(cnt).asDouble();
            if(++j>=H0.cols())
            {
                i++;
                j=0;
            }
        }
    }

	//number of links
    int numLinks=opt.check("numLinks",Value(0)).asInt();
    if(numLinks==0)
    {
        cerr<<"Error: invalid number of links specified!" <<endl;
        type="right";
        H0.eye();
        return false;
    }
    linkList.resize(numLinks,NULL);

    for(int iLink=0; iLink<numLinks; iLink++)
    {
        char link[255];
        sprintf(link,"link_%d",iLink);
		//look for link_i into the property parameters
        Bottle &bLink=opt.findGroup(link);
        if(bLink.isNull())
        {
            cerr<<"Error: " << link << " is missing!" <<endl;
            type="right";
            H0.eye();
            dispose();
            return false;
        }

		//kinematics parameters
        double A=bLink.check("A",Value(0.0)).asDouble();
        double D=bLink.check("D",Value(0.0)).asDouble();
        double alpha=CTRL_DEG2RAD*bLink.check("alpha",Value(0.0)).asDouble();
        double offset=CTRL_DEG2RAD*bLink.check("offset",Value(0.0)).asDouble();
        double min=CTRL_DEG2RAD*bLink.check("min",Value(0.0)).asDouble();
        double max=CTRL_DEG2RAD*bLink.check("max",Value(0.0)).asDouble();
		
		//dynamic parameters
		//mass
		double mass=bLink.check("mass",Value(0.0)).asDouble();
		//inertia
		if(Bottle *bI=opt.find("Inertia").asList())
		{
			i=0; j=0;
			I.zero(); 
			for(int cnt=0; (cnt<bI->size()) && (cnt<I.rows()*I.cols()); cnt++)
			{    
				I(i,j)=bI->get(cnt).asDouble();
				if(++j>=I.cols())
				{
					i++;
					j=0;
				}
			}
		}
		//HC
		if(Bottle *bHC=opt.find("H_COM").asList())
		{
			i=0; j=0;
			HC.zero(); 
			for(int cnt=0; (cnt<bHC->size()) && (cnt<HC.rows()*HC.cols()); cnt++)
			{    
				HC(i,j)=bHC->get(cnt).asDouble();
				if(++j>=HC.cols())
				{
					i++;
					j=0;
				}
			}
		}
		//create iDynLink from parameters
		linkList[iLink] = new iDynLink(mass,HC,I,A,D,alpha,offset,min,max);
		//insert the link in the list
        *this<<*linkList[iLink];

        if(bLink.check("blocked"))
            blockLink(iLink,CTRL_DEG2RAD*bLink.find("blocked").asDouble());
    }

    return configured=true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLimb &iDynLimb::operator=(const iDynLimb &limb)
{
    dispose();	
    clone(limb);

    return *this;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iDynLimb::~iDynLimb()
{
    dispose();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLimb::allocate(const string &_type)
{
    type=_type;

    if(type!="right" && type!="left")
        type="right";

    configured=true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLimb::clone(const iDynLimb &limb)
{
    type=limb.type;
	verbose = limb.verbose;
	N = limb.N;
	DOF = limb.DOF;

	H0=limb.H0;
	curr_q = limb.curr_q;
	curr_dq = limb.curr_dq;
	curr_ddq = limb.curr_ddq;

	unsigned int n,i;
	
	//now copy all lists

    if(n=limb.linkList.size())
    {
        linkList.resize(n);
        for(i=0; i<n; i++)
        {
            linkList[i]=new iDynLink(*limb.linkList[i]);
			//also push into allList
            *this << *linkList[i];

			//push into quick list and hash lists (see iKinChain::buildChain())
			quickList.push_back(allList[i]);
            hash_dof.push_back(quickList.size()-1);
            hash.push_back(i);
        }
    }
    configured=limb.configured;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iDynLimb::dispose()
{
    if(unsigned int n=linkList.size())
    {
        for(unsigned int i=0; i<n; i++)
            if(linkList[i])
                delete linkList[i];

        linkList.clear();
    }
	
	iDynChain::dispose();

    configured=false;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~





//======================================
//
//			  ICUB ARM DYN
//
//======================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubArmDyn::iCubArmDyn()
{
    allocate("right");
	setIterMode(KINFWD_WREBWD);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubArmDyn::iCubArmDyn(const string &_type, const ChainComputationMode _mode)
{
    allocate(_type);
	setIterMode(_mode);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubArmDyn::iCubArmDyn(const iCubArmDyn &arm)
{
    clone(arm);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iCubArmDyn::allocate(const string &_type)
{
    iDynLimb::allocate(_type);

    H0.zero();
    H0(0,1)=-1;	H0(1,2)=-1;
    H0(2,0)=1;	H0(3,3)=1;

    linkList.resize(10);

	//now set the parameters properly according to the part
    if (type=="right")
    {
		//set mass, inertia and COM

		//create iDynLink from parameters calling
		//          iDynLink(mass, rC (3x1), I(6x1),            A,         D,       alfa,            offset,         min,               max);
        linkList[0]=new iDynLink(0,					0,		  0,		 0,				0,			0,			0,			0,			0,			0,	      0.032,      0.0,  M_PI/2.0,                 0.0, -22.0*CTRL_DEG2RAD,  84.0*CTRL_DEG2RAD);
        linkList[1]=new iDynLink(0,					0,		  0,		 0,				0,			0,			0,			0,			0,			0,	        0.0,      0.0,  M_PI/2.0,           -M_PI/2.0, -39.0*CTRL_DEG2RAD,  39.0*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(0,					0,		  0,		 0,				0,			0,			0,			0,			0,			0,   -0.0233647,  -0.1433,  M_PI/2.0, -105.0*CTRL_DEG2RAD, -59.0*CTRL_DEG2RAD,  59.0*CTRL_DEG2RAD);
        linkList[3]=new iDynLink(0.189,		 0.005e-3,  18.7e-3,   1.19e-3,		 123.0e-6,   0.021e-6,  -0.001e-6,    24.4e-6,    4.22e-6,   113.0e-6,			0.0, -0.10774,  M_PI/2.0,           -M_PI/2.0, -95.5*CTRL_DEG2RAD,   5.0*CTRL_DEG2RAD);
        linkList[4]=new iDynLink(0.179,		-0.094e-3, -6.27e-3,  -16.6e-3,		 137.0e-6, -0.453e-06,  0.203e-06,    83.0e-6,    20.7e-6,    99.3e-6,			0.0,      0.0, -M_PI/2.0,           -M_PI/2.0,                0.0, 160.8*CTRL_DEG2RAD);
        linkList[5]=new iDynLink(0.884,		  1.79e-3, -62.9e-3, 0.064e-03,		 743.0e-6,    63.9e-6,  0.851e-06,   336.0e-6,   -3.61e-6,   735.0e-6, 			0.0, -0.15228, -M_PI/2.0, -105.0*CTRL_DEG2RAD, -37.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[6]=new iDynLink(0.074,		 -13.7e-3, -3.71e-3,   1.05e-3,		  28.4e-6,  -0.502e-6,  -0.399e-6,    9.24e-6,  -0.371e-6,    29.9e-6,		  0.015,      0.0,  M_PI/2.0,                 0.0,   0.0*CTRL_DEG2RAD, 106.0*CTRL_DEG2RAD);//5.5
        linkList[7]=new iDynLink(0.525,		-0.347e-3,  71.3e-3,  -4.76e-3,		 766.0e-6,    5.66e-6,    1.40e-6,   164.0e-6,    18.2e-6,   699.0e-6,	        0.0,  -0.1373,  M_PI/2.0,           -M_PI/2.0, -90.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[8]=new iDynLink(	 0,			    0,        0,         0,		 	    0,		    0,		    0,			0,			0,		    0,	        0.0,      0.0,  M_PI/2.0,            M_PI/2.0, -90.0*CTRL_DEG2RAD,   0.0*CTRL_DEG2RAD);
        linkList[9]=new iDynLink(0.213,		  7.73e-3, -8.05e-3,  -9.00e-3,		 154.0e-6,	  12.6e-6,   -6.08e-6,   250.0e-6,    17.6e-6,   378.0e-6,	     0.0625,    0.016,       0.0,                M_PI, -20.0*CTRL_DEG2RAD,  40.0*CTRL_DEG2RAD);
    }
    else
    {
        linkList[0]=new iDynLink(0,				0,		   0,		  0,				0,			0,			0,			0,			0,			0,		  0.032,     0.0,		M_PI/2.0,                 0.0,	-22.0*CTRL_DEG2RAD,  84.0*CTRL_DEG2RAD);
        linkList[1]=new iDynLink(0,				0,		   0,		  0,				0,			0,			0,			0,			0,			0,		    0.0,	 0.0,		M_PI/2.0,           -M_PI/2.0,	-39.0*CTRL_DEG2RAD,  39.0*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(0,				0,		   0,		  0,				0,			0,			0,			0,			0,			0,	  0.0233647, -0.1433,	   -M_PI/2.0,  105.0*CTRL_DEG2RAD,  -59.0*CTRL_DEG2RAD,  59.0*CTRL_DEG2RAD);
        linkList[3]=new iDynLink(0.13,	-0.004e-3, 14.915e-3, -0.019e-3,		54.421e-6,   0.009e-6,     0.0e-6,   9.331e-6,  -0.017e-6,  54.862e-6,			0.0, 0.10774,	   -M_PI/2.0,            M_PI/2.0,  -95.5*CTRL_DEG2RAD,   5.0*CTRL_DEG2RAD);
        linkList[4]=new iDynLink(0.178,  0.097e-3,  -6.271e-3, 16.622e-3,		 137.2e-6,   0.466e-6,   0.365e-6,  82.927e-6, -20.524e-6,  99.274e-6,			0.0,	 0.0,		M_PI/2.0,           -M_PI/2.0,				   0.0,	160.8*CTRL_DEG2RAD);
        linkList[5]=new iDynLink(0.894, -1.769e-3, 63.302e-3, -0.084e-3,	   748.531e-6,  63.340e-6,  -0.903e-6, 338.109e-6,  -4.031e-6, 741.022e-6,			0.0, 0.15228,	   -M_PI/2.0,   75.0*CTRL_DEG2RAD,  -37.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[6]=new iDynLink(0.074, 13.718e-3,  3.712e-3, -1.046e-3,		28.389e-6,  -0.515e-6,  -0.408e-6,   9.244e-6,  -0.371e-6,  29.968e-6,		 -0.015,     0.0,		M_PI/2.0,                 0.0,    0.0*CTRL_DEG2RAD, 106.0*CTRL_DEG2RAD);//5.5*CTRL_DEG2RAD, 106.0*CTRL_DEG2RAD);
        linkList[7]=new iDynLink(0.525, 0.264e-3, -71.327e-3,  4.672e-3,	   765.393e-6,   4.337e-6,   0.239e-6, 164.578e-6,  19.381e-6, 698.060e-6,			0.0,  0.1373,		M_PI/2.0,           -M_PI/2.0,	-90.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[8]=new iDynLink(	 0,		   0,		   0,		  0,				0,		    0,	        0,		    0,		    0,		    0,			0.0,	 0.0,		M_PI/2.0,            M_PI/2.0,	-90.0*CTRL_DEG2RAD,   0.0*CTRL_DEG2RAD);
        linkList[9]=new iDynLink(0.214, 7.851e-3, -8.319e-3, 9.284e-3,		   157.143e-6,  12.780e-6,   4.823e-6, 247.995e-6, -18.188e-6, 380.535e-6,		 0.0625,  -0.016,			 0.0,                 0.0,	-20.0*CTRL_DEG2RAD,  40.0*CTRL_DEG2RAD);
    }
	//insert in the allList
    for(unsigned int i=0; i<linkList.size(); i++)
        *this << *linkList[i];

    //blockLink(0,0.0);
    //blockLink(1,0.0);
    //blockLink(2,0.0);

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~





//======================================
//
//	      ICUB ARM NO TORSO DYN
//
//======================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubArmNoTorsoDyn::iCubArmNoTorsoDyn()
{
    allocate("right");
	setIterMode(KINFWD_WREBWD);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubArmNoTorsoDyn::iCubArmNoTorsoDyn(const string &_type, const ChainComputationMode _mode)
{
    allocate(_type);
	setIterMode(_mode);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubArmNoTorsoDyn::iCubArmNoTorsoDyn(const iCubArmNoTorsoDyn &arm)
{
    clone(arm);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iCubArmNoTorsoDyn::allocate(const string &_type)
{
    iDynLimb::allocate(_type);

    H0.zero();
	H0 = eye(4,4);

    linkList.resize(7);

	//now set the parameters properly according to the part
    if (type=="right")
    {
		//create iDynLink from parameters calling
		//          iDynLink(mass, rC (3x1), I(6x1),            A,         D,       alfa,            offset,         min,               max);
        
		//linkList[0]=new iDynLink(0.189,		 0.005e-3,  18.7e-3,   1.19e-3,		 123.0e-6,   0.021e-6,  -0.001e-6,    24.4e-6,    4.22e-6,   113.0e-6,			0.0,	  0.0,  M_PI/2.0,           -M_PI/2.0, -95.5*CTRL_DEG2RAD,   5.0*CTRL_DEG2RAD); //-0.10774		
		linkList[0]=new iDynLink(0.189,		 0.005e-3,  18.7e-3,   1.19e-3,		 123.0e-6,   0.021e-6,  -0.001e-6,    24.4e-6,    4.22e-6,   113.0e-6,			0.0, -0.0,  M_PI/2.0,           -M_PI/2.0, -95.5*CTRL_DEG2RAD,   5.0*CTRL_DEG2RAD);

        linkList[1]=new iDynLink(0.179,		-0.094e-3, -6.27e-3,  -16.6e-3,		 137.0e-6, -0.453e-06,  0.203e-06,    83.0e-6,    20.7e-6,    99.3e-6,			0.0,      0.0, -M_PI/2.0,           -M_PI/2.0,                0.0, 160.8*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(0.884,		  1.79e-3, -62.9e-3, 0.064e-03,		 743.0e-6,    63.9e-6,  0.851e-06,   336.0e-6,   -3.61e-6,   735.0e-6, 			0.0, -0.15228, -M_PI/2.0, -105.0*CTRL_DEG2RAD, -37.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[3]=new iDynLink(0.074,		 -13.7e-3, -3.71e-3,   1.05e-3,		  28.4e-6,  -0.502e-6,  -0.399e-6,    9.24e-6,  -0.371e-6,    29.9e-6,		  0.015,      0.0,  M_PI/2.0,                 0.0,   0.0*CTRL_DEG2RAD, 106.0*CTRL_DEG2RAD);//5.5
        linkList[4]=new iDynLink(0.525,		-0.347e-3,  71.3e-3,  -4.76e-3,		 766.0e-6,    5.66e-6,    1.40e-6,   164.0e-6,    18.2e-6,   699.0e-6,	        0.0,  -0.1373,  M_PI/2.0,           -M_PI/2.0, -90.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[5]=new iDynLink(	 0,			    0,        0,         0,		 	    0,		    0,		    0,			0,			0,		    0,	        0.0,      0.0,  M_PI/2.0,            M_PI/2.0, -90.0*CTRL_DEG2RAD,   0.0*CTRL_DEG2RAD);
        linkList[6]=new iDynLink(0.213,		  7.73e-3, -8.05e-3,  -9.00e-3,		 154.0e-6,	  12.6e-6,   -6.08e-6,   250.0e-6,    17.6e-6,   378.0e-6,	     0.0625,    0.016,       0.0,                M_PI, -20.0*CTRL_DEG2RAD,  40.0*CTRL_DEG2RAD);
 	}
    else
    {
        linkList[0]=new iDynLink(0.13,	-0.004e-3, 14.915e-3, -0.019e-3,		54.421e-6,   0.009e-6,     0.0e-6,   9.331e-6,  -0.017e-6,  54.862e-6,			0.0,	 0.0,	   -M_PI/2.0,            M_PI/2.0,  -95.5*CTRL_DEG2RAD,   5.0*CTRL_DEG2RAD);//0.10774
        linkList[1]=new iDynLink(0.178,  0.097e-3,  -6.271e-3, 16.622e-3,		 137.2e-6,   0.466e-6,   0.365e-6,  82.927e-6, -20.524e-6,  99.274e-6,			0.0,	 0.0,		M_PI/2.0,           -M_PI/2.0,				   0.0,	160.8*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(0.894, -1.769e-3, 63.302e-3, -0.084e-3,	   748.531e-6,  63.340e-6,  -0.903e-6, 338.109e-6,  -4.031e-6, 741.022e-6,			0.0, 0.15228,	   -M_PI/2.0,   75.0*CTRL_DEG2RAD,  -37.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[3]=new iDynLink(0.074, 13.718e-3,  3.712e-3, -1.046e-3,		28.389e-6,  -0.515e-6,  -0.408e-6,   9.244e-6,  -0.371e-6,  29.968e-6,		 -0.015,     0.0,		M_PI/2.0,                 0.0,    0.0*CTRL_DEG2RAD, 106.0*CTRL_DEG2RAD);//5.5*CTRL_DEG2RAD, 106.0*CTRL_DEG2RAD);
        linkList[4]=new iDynLink(0.525, 0.264e-3, -71.327e-3,  4.672e-3,	   765.393e-6,   4.337e-6,   0.239e-6, 164.578e-6,  19.381e-6, 698.060e-6,			0.0,  0.1373,		M_PI/2.0,           -M_PI/2.0,	-90.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[5]=new iDynLink(	 0,		   0,		   0,		  0,				0,		    0,	        0,		    0,		    0,		    0,			0.0,	 0.0,		M_PI/2.0,            M_PI/2.0,	-90.0*CTRL_DEG2RAD,   0.0*CTRL_DEG2RAD);
        linkList[6]=new iDynLink(0.214, 7.851e-3, -8.319e-3, 9.284e-3,		   157.143e-6,  12.780e-6,   4.823e-6, 247.995e-6, -18.188e-6, 380.535e-6,		 0.0625,  -0.016,			 0.0,                 0.0,	-20.0*CTRL_DEG2RAD,  40.0*CTRL_DEG2RAD);
    }
	//insert in the allList
    for(unsigned int i=0; i<linkList.size(); i++)
        *this << *linkList[i];
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~



//======================================
//
//			  ICUB TORSO DYN
//
//======================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubTorsoDyn::iCubTorsoDyn()
{
    allocate("lower");
	setIterMode(KINBWD_WREBWD);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubTorsoDyn::iCubTorsoDyn(const string &_type, const ChainComputationMode _mode)
{
	allocate(_type); 
	setIterMode(_mode);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubTorsoDyn::iCubTorsoDyn(const iCubTorsoDyn &torso)
{
    clone(torso);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iCubTorsoDyn::allocate(const string &_type)
{
	type = _type;
	if(type!="lower" && type!="upper")
		type = "lower";

    H0.zero();
    H0(0,1)=-1;	H0(1,2)=-1;
    H0(2,0)=1;	H0(3,3)=1;
	H0.eye();

    linkList.resize(3);

    if (type=="lower")
    {
		//      iDynLink(     mass,  rC (3x1),      I(6x1),					A,         D,       alfa,            offset,         min,               max);
        linkList[0]=new iDynLink(0,	0,	0,	0,		0,0,0,	0,0,0,	      0.032,      0.0,  M_PI/2.0,                   0.0,  -22.0*CTRL_DEG2RAD,  84.0*CTRL_DEG2RAD);
        linkList[1]=new iDynLink(0,	0,	0,	0,		0,0,0,	0,0,0,	        0.0,      0.001,  M_PI/2.0,            -M_PI/2.0, -39.0*CTRL_DEG2RAD,  39.0*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(0,	0,	0,	0,		0,0,0,	0,0,0,   -0.0233647,  -0.1933,  -M_PI/2.0,             -M_PI/2.0, -59.0*CTRL_DEG2RAD,  59.0*CTRL_DEG2RAD);
    }
    else
    {
        linkList[0]=new iDynLink(0,	0,	0,	0,		0,0,0,	0,0,0,	      0.032,      0.0,  M_PI/2.0,                 0.0,   -22.0*CTRL_DEG2RAD,  84.0*CTRL_DEG2RAD);
        linkList[1]=new iDynLink(0,	0,	0,	0,		0,0,0,	0,0,0,	        0.0,      0.001,  M_PI/2.0,           -M_PI/2.0, -39.0*CTRL_DEG2RAD,  39.0*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(0,	0,	0,	0,		0,0,0,	0,0,0,   -0.0233647,  -0.1933,  -M_PI/2.0,            -M_PI/2.0, -59.0*CTRL_DEG2RAD,  59.0*CTRL_DEG2RAD);
    }

	//insert in the allList
    for(unsigned int i=0; i<linkList.size(); i++)
        *this << *linkList[i];
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~





//======================================
//
//			  ICUB LEG DYN
//
//======================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

iCubLegDyn::iCubLegDyn()
{
    allocate("right");
	setIterMode(KINFWD_WREBWD);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubLegDyn::iCubLegDyn(const string &_type,const ChainComputationMode _mode)
{
    allocate(_type);
	setIterMode(_mode);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubLegDyn::iCubLegDyn(const iCubLegDyn &leg)
{
    clone(leg);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iCubLegDyn::allocate(const string &_type)
{
    iDynLimb::allocate(_type);

    H0.eye();//H0.zero();
    //H0(0,0)=1;	    H0(1,2)=1;    
    //H0(2,1)=-1;		
	H0(2,3)=-0.1199;
    //H0(3,3)=1;

    linkList.resize(6);

	//dynamical parameters: inertia and COM are matrices, they must be initialized before
	// mass

    if(type=="right")
    {
        H0(1,3)=0.0681;

		//create iDynLink from parameters calling
		//linkList[i] = new iDynLink(mass,HC,I,A,D,alfa,offset,min,max);

        linkList[0]=new iDynLink(0.754,         -0.0782,  -0.00637,  -0.00093,				0,			0,			0,			0,			0,			0,					0.0,       0.0,  M_PI/2.0,	 M_PI/2.0,  -44.0*CTRL_DEG2RAD, 132.0*CTRL_DEG2RAD);
        linkList[1]=new iDynLink(0.526,         0.00296,  -0.00072,	  0.03045,				0,			0,			0,			0,			0,			0,					0.0,	   0.0,  M_PI/2.0,	 M_PI/2.0, -17.0*CTRL_DEG2RAD,  119.0*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(2.175,         0.00144,   0.06417,	  0.00039,				0,			0,			0,			0,			0,			0,					0.0,	0.2236, -M_PI/2.0,	-M_PI/2.0,  -79.0*CTRL_DEG2RAD,  79.0*CTRL_DEG2RAD);
        linkList[3]=new iDynLink(1.264,          0.1059,   0.00182,	 -0.00211,			0.0,			0.0,			0.0,			0.0,			0.0,			0.0,				 -0.213,       0.0,      M_PI,  M_PI/2.0, -125.0*CTRL_DEG2RAD,  23.0*CTRL_DEG2RAD);
        linkList[4]=new iDynLink(0.746,         -0.0054,   0.00163,   -0.0172,				0,			0,			0,			0,			0,			0,					0.0,       0.0,  M_PI/2.0,		 0.0,  -42.0*CTRL_DEG2RAD,  21.0*CTRL_DEG2RAD);
        linkList[5]=new iDynLink(0,					  0,		 0,			0,				0,			0,			0,			0,			0,			0,				 -0.041,       0.0,      M_PI,		 0.0,  -24.0*CTRL_DEG2RAD,  24.0*CTRL_DEG2RAD);
    /*
		linkList[0]=new iDynLink(0,    0,     0,    0,    0,   0,   0,   0,   0,   0,   0.0,     0.0,  M_PI/2.0,  M_PI/2.0,  -44.0*CTRL_DEG2RAD, 132.0*CTRL_DEG2RAD);
		linkList[1]=new iDynLink(0,    0,     0,    0,    0,   0,   0,   0,   0,   0,   0.0,     0.0,  M_PI/2.0,  M_PI/2.0, -119.0*CTRL_DEG2RAD,  17.0*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(0,    0,     0,    0,    0,   0,   0,   0,   0,   0,   0.0,  0.2236, -M_PI/2.0, -M_PI/2.0,  -79.0*CTRL_DEG2RAD,  79.0*CTRL_DEG2RAD);
        //linkList[3]=new iDynLink(0,    0,     0,    0,    0,   0,   0,   0,   0,   0,   0.0,    0.0,      M_PI,  M_PI/2.0, -125.0*CTRL_DEG2RAD,  23.0*CTRL_DEG2RAD);
        linkList[3]=new iDynLink(1.264,    0.1059,   0.00182,  -0.00211,                0,          0,          0,          0,          0,          0, -0.213,    0.0,      M_PI,  M_PI/2.0, -125.0*CTRL_DEG2RAD,  23.0*CTRL_DEG2RAD);
		linkList[4]=new iDynLink(0,    0,     0,    0,    0,   0,   0,   0,   0,   0,   0.0,     0.0,  M_PI/2.0,       0.0,  -42.0*CTRL_DEG2RAD,  21.0*CTRL_DEG2RAD);
        linkList[5]=new iDynLink(0,    0,     0,    0,    0,   0,   0,   0,   0,   0, -0.041,     0.0,      M_PI,       0.0,  -24.0*CTRL_DEG2RAD,  24.0*CTRL_DEG2RAD);
	*/
	}
    else
    {
        H0(1,3)=-0.0681;

		//create iDynLink from parameters calling
		//linkList[i] = new iDynLink(mass,HC,I,A,D,alfa,offset,min,max);

        linkList[0]=new iDynLink(0.754,         -0.0782, -0.00637,   0.00093,	 471.076e-6,     2.059e-6,     1.451e-6,      346.478e-6,        1.545e-6,       510.315e-6,				   0.0,     0.0, -M_PI/2.0,  M_PI/2.0,  -44.0*CTRL_DEG2RAD, 132.0*CTRL_DEG2RAD);
        linkList[1]=new iDynLink(0.526,         0.00296, -0.00072, -0.03045,	738.0487e-6,	-0.074e-6,    -0.062e-6,      561.583e-6,       10.835e-6,       294.119e-6,				   0.0,     0.0, -M_PI/2.0,  M_PI/2.0, -17.0*CTRL_DEG2RAD,  119.0*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(2.175,         0.00144,  0.06417,	-0.00039,	7591.073e-6,   -67.260e-6,     2.267e-6,    1423.0245e-6,    36.372582e-6,		7553.8490e-6,				   0.0, -0.2236,  M_PI/2.0, -M_PI/2.0,  -79.0*CTRL_DEG2RAD,  79.0*CTRL_DEG2RAD);
        linkList[3]=new iDynLink(1.264,          0.1059,  0.00182,	 0.00211,	 998.950e-6,  -185.699e-6,   -63.147e-6,     4450.537e-6,        0.786e-6,		 4207.657e-6,				-0.213,     0.0,      M_PI,  M_PI/2.0, -125.0*CTRL_DEG2RAD,  23.0*CTRL_DEG2RAD);
        linkList[4]=new iDynLink(0.746,         -0.0054,  0.00163,    0.0172,    633.230e-6,	-7.081e-6,	  41.421e-6,	   687.760e-6,		 20.817e-6,		  313.897e-6,				   0.0,     0.0, -M_PI/2.0,       0.0,  -42.0*CTRL_DEG2RAD,  21.0*CTRL_DEG2RAD);
        linkList[5]=new iDynLink(0,					  0,		0,		   0,			  0,			0,			   0,				0,				 0,				   0,				-0.041,     0.0,       0.0,       0.0,  -24.0*CTRL_DEG2RAD,  24.0*CTRL_DEG2RAD);
    }

    for(unsigned int i=0; i<linkList.size(); i++)
        *this << *linkList[i];
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//======================================
//
//	    ICUB LEG NO TORSO DYN
//
//======================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

iCubLegNoTorsoDyn::iCubLegNoTorsoDyn()
:iCubLegDyn()
{
	H0.zero();	H0.eye();	H0(2,3)=-0.1199;
	if(type=="right")
        H0(1,3)=0.0681;
	else
        H0(1,3)=-0.0681;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubLegNoTorsoDyn::iCubLegNoTorsoDyn(const string &_type,const ChainComputationMode _mode)
:iCubLegDyn(_type,_mode)
{
	H0.zero();	H0.eye();	H0(2,3)=-0.1199;
	if(type=="right")
        H0(1,3)=0.0681;
	else
        H0(1,3)=-0.0681;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubLegNoTorsoDyn::iCubLegNoTorsoDyn(const iCubLegNoTorsoDyn &leg)
{
    clone(leg);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~






//======================================
//
//			  ICUB EYE DYN
//
//======================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubEyeDyn::iCubEyeDyn()
{
    allocate("right");
	setIterMode(KINFWD_WREBWD);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubEyeDyn::iCubEyeDyn(const string &_type,const ChainComputationMode _mode)
{
    allocate(_type);
	setIterMode(_mode);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubEyeDyn::iCubEyeDyn(const iCubEyeDyn &eye)
{
    clone(eye);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iCubEyeDyn::allocate(const string &_type)
{
    iDynLimb::allocate(_type);

    H0.zero();
    H0(0,1)=-1;
    H0(1,2)=-1;
    H0(2,0)=1;
    H0(3,3)=1;

    linkList.resize(8);

	//dynamical parameters: inertia and COM are matrices, they must be initialized before
	// mass
	Vector m(8);
	// inertia and COM
	deque<Matrix> HC;  
	deque<Matrix> I; 
	Matrix HCtmp(4,4); HCtmp.eye();
	Matrix Itmp(3,3); Itmp.zero();
	for(int i=0;i<8; i++)
	{
		HC.push_back(HCtmp);
		I.push_back(Itmp);
	}

    if(type=="right")
    {
		 m=0;

		//create iDynLink from parameters calling
		//linkList[i] = new iDynLink(mass,HC,I,A,D,alfa,offset,min,max);

        linkList[0]=new iDynLink(m[0],	HC[0],	I[0],   0.032,    0.0,  M_PI/2.0,       0.0, -22.0*CTRL_DEG2RAD, 84.0*CTRL_DEG2RAD);
        linkList[1]=new iDynLink(m[1],	HC[1],	I[1],     0.0,    0.0,  M_PI/2.0, -M_PI/2.0, -39.0*CTRL_DEG2RAD, 39.0*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(m[2],	HC[2],	I[2], 0.00231,-0.1933, -M_PI/2.0, -M_PI/2.0, -59.0*CTRL_DEG2RAD, 59.0*CTRL_DEG2RAD);
        linkList[3]=new iDynLink(m[3],	HC[3],	I[3],   0.033,    0.0,  M_PI/2.0,  M_PI/2.0, -40.0*CTRL_DEG2RAD, 30.0*CTRL_DEG2RAD);
        linkList[4]=new iDynLink(m[4],	HC[4],	I[4],     0.0,    0.0,  M_PI/2.0,  M_PI/2.0, -70.0*CTRL_DEG2RAD, 60.0*CTRL_DEG2RAD);
        linkList[5]=new iDynLink(m[5],	HC[5],	I[5],  -0.054, 0.0825, -M_PI/2.0, -M_PI/2.0, -55.0*CTRL_DEG2RAD, 55.0*CTRL_DEG2RAD);
        linkList[6]=new iDynLink(m[6],	HC[6],	I[6],     0.0,  0.034, -M_PI/2.0,       0.0, -35.0*CTRL_DEG2RAD, 15.0*CTRL_DEG2RAD);
        linkList[7]=new iDynLink(m[7],	HC[7],	I[7],     0.0,    0.0,  M_PI/2.0, -M_PI/2.0, -50.0*CTRL_DEG2RAD, 50.0*CTRL_DEG2RAD);
    }
    else
    {
		 m=0;

		//create iDynLink from parameters calling
		//linkList[i] = new iDynLink(mass,HC,I,A,D,alfa,offset,min,max);

        linkList[0]=new iDynLink(m[0],	HC[0],	I[0],   0.032,    0.0,  M_PI/2.0,       0.0, -22.0*CTRL_DEG2RAD, 84.0*CTRL_DEG2RAD);
        linkList[1]=new iDynLink(m[1],	HC[1],	I[1],     0.0,    0.0,  M_PI/2.0, -M_PI/2.0, -39.0*CTRL_DEG2RAD, 39.0*CTRL_DEG2RAD);
        linkList[2]=new iDynLink(m[2],	HC[2],	I[2], 0.00231,-0.1933, -M_PI/2.0, -M_PI/2.0, -59.0*CTRL_DEG2RAD, 59.0*CTRL_DEG2RAD);
        linkList[3]=new iDynLink(m[3],	HC[3],	I[3],   0.033,    0.0,  M_PI/2.0,  M_PI/2.0, -40.0*CTRL_DEG2RAD, 30.0*CTRL_DEG2RAD);
        linkList[4]=new iDynLink(m[4],	HC[4],	I[4],     0.0,    0.0,  M_PI/2.0,  M_PI/2.0, -70.0*CTRL_DEG2RAD, 60.0*CTRL_DEG2RAD);
        linkList[5]=new iDynLink(m[5],	HC[5],	I[5],  -0.054, 0.0825, -M_PI/2.0, -M_PI/2.0, -55.0*CTRL_DEG2RAD, 55.0*CTRL_DEG2RAD);
        linkList[6]=new iDynLink(m[6],	HC[6],	I[6],     0.0, -0.034, -M_PI/2.0,       0.0, -35.0*CTRL_DEG2RAD, 15.0*CTRL_DEG2RAD);
        linkList[7]=new iDynLink(m[7],	HC[7],	I[7],     0.0,    0.0,  M_PI/2.0, -M_PI/2.0, -50.0*CTRL_DEG2RAD, 50.0*CTRL_DEG2RAD);
    }

    for(unsigned int i=0; i<linkList.size(); i++)
        *this << *linkList[i];

    blockLink(0,0.0);
    blockLink(1,0.0);
    blockLink(2,0.0);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


//======================================
//
//		ICUB EYE NECK REF DYN   
//         
//======================================

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubEyeNeckRefDyn::iCubEyeNeckRefDyn()
{
    allocate("right");
	setIterMode(KINFWD_WREBWD);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubEyeNeckRefDyn::iCubEyeNeckRefDyn(const string &_type,const ChainComputationMode _mode)
{
    allocate(_type);
	setIterMode(_mode);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubEyeNeckRefDyn::iCubEyeNeckRefDyn(const iCubEyeNeckRefDyn &eye)
{
    clone(eye);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iCubEyeNeckRefDyn::allocate(const string &_type)
{
    rmLink(0);
    rmLink(0);
    rmLink(0);

    delete linkList[0];
    delete linkList[1];
    delete linkList[2];

    linkList.erase(linkList.begin(),linkList.begin()+2);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

////////////////////////////////////////
//		ICUB INERTIAL SENSOR DYN            
////////////////////////////////////////

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubInertialSensorDyn::iCubInertialSensorDyn(const ChainComputationMode _mode)
{
    allocate("right");
	setIterMode(_mode);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubInertialSensorDyn::iCubInertialSensorDyn(const iCubInertialSensorDyn &sensor)
{
    clone(sensor);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iCubInertialSensorDyn::allocate(const string &_type)
{
    iDynLimb::allocate(_type);

    H0.zero();
    H0(0,1)=-1;
    H0(1,2)=-1;
    H0(2,0)=1;
    H0(3,3)=1;

    linkList.resize(7);

	//create iDynLink from parameters calling
	//linkList[i] = new iDynLink(mass,HC,I,A,D,alfa,offset,min,max);

    // links of torso and neck
    linkList[0]=new iDynLink(0,							  0,		     0,		         0,					    0,				  0,				   0,			 0,				 0,				0,		  0.032,       0.0,  M_PI/2.0,       0.0, -22.0*CTRL_DEG2RAD, 84.0*CTRL_DEG2RAD);
    linkList[1]=new iDynLink(0,							  0,		     0,		         0,					    0,				  0,				   0,			 0,				 0,				0,			0.0,       0.0,  M_PI/2.0, -M_PI/2.0, -39.0*CTRL_DEG2RAD, 39.0*CTRL_DEG2RAD);
    linkList[2]=new iDynLink(0,							  0,		     0,		         0,			       	    0,			  	  0,				   0,			 0,				 0,				0,		0.00231,   -0.1933, -M_PI/2.0, -M_PI/2.0, -59.0*CTRL_DEG2RAD, 59.0*CTRL_DEG2RAD);
    linkList[3]=new iDynLink(0.27017604,	  -30.535917e-3,  2.5211768e-3, -0.23571261e-3, 	     100.46346e-6,   -0.17765781e-6,       0.44914333e-6, 45.425961e-6, -0.12682862e-6, 1.0145446e+02,		  0.033,       0.0,  M_PI/2.0,  M_PI/2.0, -40.0*CTRL_DEG2RAD, 30.0*CTRL_DEG2RAD);
    linkList[4]=new iDynLink(0.27230552,				0.0,  4.3752947e-3,   5.4544215e-3, 	     142.82339e-6, -0.0059261471e-6,    -0.0022006663e-6, 82.884917e-6,  -9.1321119e-6,  87.620338e-6,          0.0,     0.001, -M_PI/2.0, -M_PI/2.0, -70.0*CTRL_DEG2RAD, 60.0*CTRL_DEG2RAD);
    linkList[5]=new iDynLink(0,							  0,		     0,		         0,					    0,				  0,				   0,			 0,				 0,				0,		 0.0225,    0.1005, -M_PI/2.0,  M_PI/2.0, -55.0*CTRL_DEG2RAD, 55.0*CTRL_DEG2RAD);

    // virtual links that describe T_nls (see http://eris.liralab.it/wiki/ICubInertiaSensorKinematics)
    linkList[6]=new iDynLink(1.3368659,		  -11.811104e-3, -5.7800518e-3,  -11.685197e-3,			3412.8918e-06,  66.297315e-6, -153.07583e-6, 4693.0882e-6,  8.0646052e-6, 4153.4285e-6, 0.0,    0.0066,  M_PI/2.0,       0.0,                0.0,               0.0);

    for(unsigned int i=0; i<linkList.size(); i++)
        *this << *linkList[i];

    // block virtual links
    blockLink(6,0.0);

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

////////////////////////////////////////
//		ICUB INERTIAL SENSOR DYN            
////////////////////////////////////////

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubNeckInertialDyn::iCubNeckInertialDyn(const ChainComputationMode _mode)
{
    allocate("right");
	setIterMode(_mode);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iCubNeckInertialDyn::iCubNeckInertialDyn(const iCubInertialSensorDyn &sensor)
{
    clone(sensor);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void iCubNeckInertialDyn::allocate(const string &_type)
{
    iDynLimb::allocate(_type);

    H0.eye();
    /*H0(0,1)=-1;
    H0(1,2)=-1;
    H0(2,0)=1;
    H0(3,3)=1;*/

    linkList.resize(4);

	//create iDynLink from parameters calling
	//linkList[i] = new iDynLink(mass,HC,I,A,D,alfa,offset,min,max);

    // links of torso and neck
    linkList[0]=new iDynLink(0.27017604,	  -30.535917e-3,  2.5211768e-3, -0.23571261e-3, 	     100.46346e-6,   -0.17765781e-6,       0.44914333e-6, 45.425961e-6, -0.12682862e-6, 1.0145446e+02,		  0.033,       0.0,  M_PI/2.0,  M_PI/2.0, -40.0*CTRL_DEG2RAD, 30.0*CTRL_DEG2RAD);
    linkList[1]=new iDynLink(0.27230552,				0.0,  4.3752947e-3,   5.4544215e-3, 	     142.82339e-6, -0.0059261471e-6,    -0.0022006663e-6, 82.884917e-6,  -9.1321119e-6,  87.620338e-6,          0.0,     0.001, -M_PI/2.0, -M_PI/2.0, -70.0*CTRL_DEG2RAD, 60.0*CTRL_DEG2RAD);
    linkList[2]=new iDynLink(0,							  0,		     0,		         0,					    0,				  0,				   0,			 0,				 0,				0,		 0.0225,    0.1005, -M_PI/2.0,  M_PI/2.0, -55.0*CTRL_DEG2RAD, 55.0*CTRL_DEG2RAD);

    // virtual links that describe T_nls (see http://eris.liralab.it/wiki/ICubInertiaSensorKinematics)
    linkList[3]=new iDynLink(1.3368659,		  -11.811104e-3, -5.7800518e-3,  -11.685197e-3,			3412.8918e-06,  66.297315e-6, -153.07583e-6, 4693.0882e-6,  8.0646052e-6, 4153.4285e-6, 0.0,    0.0066,  M_PI/2.0,       0.0,                0.0,               0.0);

    for(unsigned int i=0; i<linkList.size(); i++)
        *this << *linkList[i];

    // block virtual links
    blockLink(3,0.0);

}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~




