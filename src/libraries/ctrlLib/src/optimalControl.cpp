/* 
 * Copyright (C) 2010 RobotCub Consortium, European Commission FP6 Project IST-004370
 * Author: Serena Ivaldi
 * email:   serena.ivaldi@iit.it
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

#include <stdio.h>

#include <gsl/gsl_math.h>

#include <yarp/math/Math.h>
#include <yarp/math/SVD.h>
#include <iCub/ctrl/optimalControl.h>

using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::ctrl;


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Riccati::Riccati(const Matrix &_A, const Matrix &_B, const Matrix &_V,
                 const Matrix &_P, const Matrix &_VN, bool verb) 
{
    A = _A; At = A.transposed();
    B = _B; Bt = B.transposed();
    V = _V;
    P = _P;
    VN = _VN;

    Ti = new Matrix[1]; Ti[0].resize(1,1); Ti[0].zero();
    Li = new Matrix[1]; Li[0].resize(1,1); Li[0].zero();

    n=A.rows();
    m=B.rows();
    N=-1;

    verbose=verb;
    if(verbose) fprintf(stderr,"Riccati: problem defined, unsolved.\n");
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Riccati::setVerbose(bool verb)
{
    verbose=verb;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix Riccati::L(int step)
{
    if(N<0) 
    {
        if(verbose) fprintf(stderr,"Riccati: warning. DARE has not been solved yet.\n");
        return Li[0];
    }
    if(step>=0 && step>=N)
    {
        if(verbose) fprintf(stderr,"Riccati: warning. Index for gain matrix out of bound.\n");
        return Li[0];
    }
    return Li[step];
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Matrix Riccati::T(int step)
{
    if(N<0) 
    {
        if(verbose) fprintf(stderr,"Riccati: error. DARE has not been solved yet.\n");
        return Ti[0];
    }
    if(step>=0 && step>N)
    {
        if(verbose) fprintf(stderr,"Riccati: error. Index for DARE matrix out of bound.\n");
        return Ti[0];
    }
    return Ti[step];
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Riccati::setProblemData(const Matrix &_A, const Matrix &_B, const Matrix &_V,
                             const Matrix &_P, const Matrix &_VN)
{
    A = _A; At = A.transposed();
    B = _B; Bt = B.transposed();
    V = _V;
    P = _P;
    VN = _VN;

    n=A.rows();
    m=B.rows();
    N=-1;

    if(verbose) fprintf(stderr,"Riccati: problem defined, unsolved.\n");
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Riccati::solveRiccati(int steps)
{
    int i;
    N = steps;
    delete [] Ti;
    delete [] Li;
    Ti = new Matrix[steps+1];
    Li = new Matrix[steps];
    for(i=0; i<=steps; i++)
        Ti[i].resize(VN.rows(),VN.cols());		
    //init TN=VN
    Ti[steps]=VN;
    //compute backward all Ti
    for(i=steps-1; i>=0; i--)
    {
        lastT=Ti[i+1]; 
        //Ti = V + A' * (Ti+1 - Ti+1 * B * (P + B' * Ti+1 * B)^-1 * B' * Ti+1 )* A;
        Ti[i] = V + At *(lastT - lastT * B* pinv(P+Bt*lastT*B)*Bt*lastT )* A; 
    }
    //compute forward all Li
    for(i=0;i<steps; i++)
    {
        //Li = (P + B' * Ti+1 * B)^-1 * B' * Ti+1 * A
        Li[i] = pinv(P + Bt*Ti[i+1]*B) *Bt * Ti[i+1] * A;	
    }
    if(verbose) fprintf(stderr,"Riccati: DARE solved, matrices Li and Ti computed and stored.\n");
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Vector Riccati::doLQcontrol(int step, const Vector &x)
{
    if(N<0) 
    {
        if(verbose) fprintf(stderr,"Riccati: error. DARE has not been solved yet.\n");
        Vector ret(1,0.0);
        return ret;
    }
    if(step>=0 && step>N)
    {
        if(verbose) fprintf(stderr,"Riccati: error. Index for DARE matrix out of bound.\n");
        Vector ret(1,0.0);
        return ret;
    }
    return (Li[step] * (-1.0*x));
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Riccati::doLQcontrol(int step, const Vector &x, Vector &ret)
{
    if(N<0) 
    {
        if(verbose) fprintf(stderr,"Riccati: error. DARE has not been solved yet.\n");
        ret.zero();
    }
    else if(step>=0 && step>N)
    {
        if(verbose) fprintf(stderr,"Riccati: error. Index for DARE matrix out of bound.\n");
        ret.zero();
    }
    else
    {
        ret.resize(m);
        ret = Li[step] * (-1.0*x);
    }
}


