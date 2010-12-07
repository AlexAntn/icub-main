/* 
 * Copyright (C) 2010 RobotCub Consortium, European Commission FP6 Project IST-004370
 * Author: Ugo Pattacini
 * email:  ugo.pattacini@iit.it
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

#include <yarp/math/Math.h>
#include <iCub/ctrl/filters.h>

using namespace std;
using namespace yarp;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::ctrl;


/***************************************************************************/
Filter::Filter(const Vector &num, const Vector &den, const Vector &y0)
{
    b=num;
    a=den;
    
    m=b.size(); uold.resize(m-1);
    n=a.size(); yold.resize(n-1);
    
    init(y0);
}


/***************************************************************************/
void Filter::init(const Vector &y0)
{
    y=y0;
    for (int i=0; i<n-1; i++)
        yold[i]=y;

    for (int i=0; i<m-1; i++)
        uold[i].resize(y.length(),0.0);
}


/***************************************************************************/
void Filter::getCoeffs(Vector &num, Vector &den)
{
    num=b;
    den=a;
}


/***************************************************************************/
void Filter::setCoeffs(const Vector &num, const Vector &den)
{
    b=num;
    a=den;

    uold.clear();
    yold.clear();

    m=b.size(); uold.resize(m-1);
    n=a.size(); yold.resize(n-1);

    init(y);
}


/***************************************************************************/
void Filter::adjustCoeffs(const Vector &num, const Vector &den)
{
    if ((num.size()==b.size()) && (den.size()==a.size()))
    {
        b=num;
        a=den;
    }
}


/***************************************************************************/
Vector Filter::filt(const Vector &u)
{
    y=b[0]*u;
    
    for (int i=1; i<m; i++)
        y=y+b[i]*uold[i-1];
    
    for (int i=1; i<n; i++)
        y=y-a[i]*yold[i-1];
    
    y=(1.0/a[0])*y;
    
    uold.push_front(u);
    uold.pop_back();
    
    yold.push_front(y);
    yold.pop_back();
    
    return y;
}


/**********************************************************************/
Vector Filter::output()
{
    return y;
}


/**********************************************************************/
RateLimiter::RateLimiter(const Vector &rL, const Vector &rU) :
                         rateLowerLim(rL), rateUpperLim(rU)
{
    size_t nL=rateLowerLim.length();
    size_t nU=rateUpperLim.length();

    n=nU>nL ? nL : nU;
}


/**********************************************************************/
void RateLimiter::init(const Vector &u0)
{ 
    uLim=u0;
}


/**********************************************************************/
void RateLimiter::getLimits(Vector &rL, Vector &rU)
{
    rL=rateLowerLim;
    rU=rateUpperLim;
}


/**********************************************************************/
void RateLimiter::setLimits(const Vector &rL, const Vector &rU)
{
    rateLowerLim=rL;
    rateUpperLim=rU;
}


/**********************************************************************/
Vector RateLimiter::filt(const Vector &u)
{
    uD=u-uLim;
    for (size_t i=0; i<n; i++)
        if (uD[i]>rateUpperLim[i])
            uD[i]=rateUpperLim[i];
        else if (uD[i]<rateLowerLim[i])
            uD[i]=rateLowerLim[i];

    uLim=uLim+uD;

    return uLim;
}



