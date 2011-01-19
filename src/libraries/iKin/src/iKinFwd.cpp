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

#include <stdio.h>

#include <iCub/iKin/iKinFwd.h>

using namespace std;
using namespace yarp;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace yarp::math;
using namespace iCub::ctrl;
using namespace iCub::iKin;


/************************************************************************/
void iCub::iKin::notImplemented(const unsigned int verbose)
{
    if (verbose)
        fprintf(stderr,"iKin: not implemented\n");
}


/************************************************************************/
iKinLink::iKinLink(double _A, double _D, double _Alpha, double _Offset,
                   double _Min, double _Max)
{
    A     =_A;
    D     =_D;
    Alpha =_Alpha;
    Offset=_Offset;

    Min=_Min;
    Max=_Max;
    Ang=Min;

    c_alpha=cos(Alpha);
    s_alpha=sin(Alpha);

    blocked    =false;
    cumulative =false;
    constrained=true;
    verbose    =0;

    H.resize(4,4);
    H.zero();
    DnH =H;
    cumH=H;
    cumH.eye();

    H(2,1)=s_alpha;
    H(2,2)=c_alpha;
    H(2,3)=D;
    H(3,3)=1.0;
}


/************************************************************************/
void iKinLink::clone(const iKinLink &l)
{
    A     =l.A;
    D     =l.D;
    Alpha =l.Alpha;
    Offset=l.Offset;

    c_alpha=l.c_alpha;
    s_alpha=l.s_alpha;

    Ang=l.Ang;
    Min=l.Min;
    Max=l.Max;

    blocked    =l.blocked;
    cumulative =l.cumulative;
    constrained=l.constrained;
    verbose    =l.verbose;

    H   =l.H;
    cumH=l.cumH;
    DnH =l.DnH;
}


/************************************************************************/
iKinLink::iKinLink(const iKinLink &l)
{
    clone(l);
}


/************************************************************************/
iKinLink &iKinLink::operator=(const iKinLink &l)
{
    clone(l);

    return *this;
}


/************************************************************************/
void iKinLink::setMin(const double _Min)
{
    Min=_Min;

    if (Ang<Min)
        Ang=Min;
}


/************************************************************************/
void iKinLink::setMax(const double _Max)
{
    Max=_Max;

    if (Ang>Max)
        Ang=Max;
}


/************************************************************************/
void iKinLink::setD(const double _D)
{
    H(2,3)=D=_D;
}


/************************************************************************/
void iKinLink::setAlpha(const double _Alpha)
{
    Alpha=_Alpha;

    H(2,2)=c_alpha=cos(Alpha);
    H(2,1)=s_alpha=sin(Alpha);
}


/************************************************************************/
double iKinLink::setAng(double _Ang)
{
    if (!blocked)
    {
        if (constrained)
            Ang=(_Ang<Min) ? Min : ((_Ang>Max) ? Max : _Ang);
        else
            Ang=_Ang;
    }
    else if (verbose)
        fprintf(stderr,"Attempt to set joint angle to %g while blocked\n",_Ang);

    return Ang;
}


/************************************************************************/
Matrix iKinLink::getH(bool c_override)
{
    double theta=Ang+Offset;
    double c_theta=cos(theta);
    double s_theta=sin(theta);

    H(0,0)=c_theta;
    H(0,1)=-s_theta*c_alpha;
    H(0,2)=s_theta*s_alpha;
    H(0,3)=c_theta*A;

    H(1,0)=s_theta;
    H(1,1)=c_theta*c_alpha;
    H(1,2)=-c_theta*s_alpha;
    H(1,3)=s_theta*A;

    if (cumulative && !c_override)
        return cumH*H;
    else
        return H;
}


/************************************************************************/
Matrix iKinLink::getH(double _Ang, bool c_override)
{
    setAng(_Ang);

    return getH(c_override);
}


/************************************************************************/
Matrix iKinLink::getDnH(unsigned int n, bool c_override)
{
    if (!n)
        return getH(c_override);
    else
    {
        double theta=Ang+Offset;
        double c_theta=cos(theta);
        double s_theta=sin(theta);

        int    C=(n>>1)&1 ? -1 : 1;

        if (n&1)
        {
            DnH(0,0)=-C*s_theta;
            DnH(0,1)=-C*c_theta*c_alpha;
            DnH(0,2)=C*c_theta*s_alpha;
            DnH(0,3)=-C*s_theta*A;
    
            DnH(1,0)=C*c_theta;
            DnH(1,1)=-C*s_theta*c_alpha;
            DnH(1,2)=C*s_theta*s_alpha;
            DnH(1,3)=C*c_theta*A;
        }
        else
        {
            DnH(0,0)=C*c_theta;
            DnH(0,1)=-C*s_theta*c_alpha;
            DnH(0,2)=C*s_theta*s_alpha;
            DnH(0,3)=C*c_theta*A;

            DnH(1,0)=C*s_theta;
            DnH(1,1)=C*c_theta*c_alpha;
            DnH(1,2)=-C*c_theta*s_alpha;
            DnH(1,3)=C*s_theta*A;
        }

        if (cumulative && !c_override)
            return cumH*DnH;
        else
            return DnH;
    }
}


/************************************************************************/
void iKinLink::addCumH(const Matrix &_cumH)
{
    cumulative=true;
    cumH=_cumH;
}


/************************************************************************/
iKinChain::iKinChain()
{
    N=DOF=verbose=0;
    hess_DH=NULL;

    H0=eye(4,4);
}


/************************************************************************/
iKinChain::iKinChain(const Matrix &_H0)
{
    N=DOF=verbose=0;
    hess_DH=NULL;

    setH0(_H0);
}


/************************************************************************/
void iKinChain::clone(const iKinChain &c)
{
    N      =c.N;
    DOF    =c.DOF;
    H0     =c.H0;
    curr_q =c.curr_q;    
    verbose=c.verbose;

    allList.assign(c.allList.begin(),c.allList.end());
    quickList.assign(c.quickList.begin(),c.quickList.end());
    hash.assign(c.hash.begin(),c.hash.end());
    hash_dof.assign(c.hash_dof.begin(),c.hash_dof.end());

    hess_DH=NULL;
}


/************************************************************************/
iKinChain::iKinChain(const iKinChain &c)
{
    clone(c);
}


/************************************************************************/
iKinChain &iKinChain::operator=(const iKinChain &c)
{
    clone(c);

    return *this;
}


/************************************************************************/
bool iKinChain::addLink(const unsigned int i, iKinLink &l)
{
    if (i<=N)
    {
        allList.insert(allList.begin()+i,&l);
        N=allList.size();

        build();

        return true;
    }
    else
    {
        if (verbose)
            fprintf(stderr,"addLink() failed due to out of range index: %d>%d\n",i,N);

        return false;
    }
}


/************************************************************************/
bool iKinChain::rmLink(const unsigned int i)
{
    if (i<N)
    {
        allList.erase(allList.begin()+i);
        N=allList.size();

        build();

        return true;
    }
    else
    {
        if (verbose)
            fprintf(stderr,"rmLink() failed due to out of range index: %d>=%d\n",i,N);

        return false;
    }
}


/************************************************************************/
void iKinChain::pushLink(iKinLink &l)
{
    allList.push_back(&l);
    N=allList.size();

    build();
}


/************************************************************************/
void iKinChain::clear()
{
    allList.clear();
    quickList.clear();
    hash.clear();
    hash_dof.clear();

    N=DOF=0;
}


/************************************************************************/
iKinChain &iKinChain::operator<<(iKinLink &l)
{
    pushLink(l);

    return *this;
}


/************************************************************************/
void iKinChain::popLink()
{
    allList.pop_back();
    N=allList.size();

    build();
}


/************************************************************************/
iKinChain &iKinChain::operator--(int)
{
    popLink();

    return *this;
}


/************************************************************************/
bool iKinChain::blockLink(const unsigned int i, double Ang)
{
    if (i<N)
    {
        allList[i]->block(Ang);
        build();

        return true;
    }
    else
    {
        if (verbose)
            fprintf(stderr,"blockLink() failed due to out of range index: %d>=%d\n",i,N);

        return false;
    }
}


/************************************************************************/
bool iKinChain::setBlockingValue(const unsigned int i, double Ang)
{
    if (i<N)
    {
        if (allList[i]->isBlocked() && (Ang!=allList[i]->getAng()))
        {
            allList[i]->blocked=false; // remove the block temporarly
            allList[i]->block(Ang);    // update the blocked link

            // update the cumulative link which follows in the chain
            if (i<N-1)
            {
                Matrix H=eye(4,4);
                int j;

                for (j=i-1; j>=0; j--)
                    if (!allList[j]->isBlocked())
                        break;
                
                for (++j; j<=(int)i; j++)
                    H=H*allList[j]->getH(true);
    
                for (; j<(int)N && !allList[j]->isCumulative(); j++)
                    H=H*allList[j]->getH(true);
    
                allList[j]->addCumH(H);
            } 

            return true;
        }
        else
        {
            if (verbose)
                fprintf(stderr,"setBlockingValue() failed since the %dth link was not already blocked\n",i);

            return false;
        }
    }
    else
    {
        if (verbose)
            fprintf(stderr,"setBlockingValue() failed due to out of range index: %d>=%d\n",i,N);

        return false;
    }
}


/************************************************************************/
bool iKinChain::releaseLink(const unsigned int i)
{
    if (i<N)
    {
        allList[i]->release();
        build();

        return true;
    }
    else
    {    
        if (verbose)
            fprintf(stderr,"releaseLink() failed due to out of range index: %d>=%d\n",i,N);

        return false;
    }
}


/************************************************************************/
bool iKinChain::isLinkBlocked(const unsigned int i)
{
    if (i<N)
        return allList[i]->isBlocked();
    else
    {    
        if (verbose)
            fprintf(stderr,"isLinkBlocked() failed due to out of range index: %d>=%d\n",i,N);

        return false;
    }
}


/************************************************************************/
void iKinChain::setAllConstraints(bool _constrained)
{
    for (unsigned int i=0; i<N; i++)
        allList[i]->setConstraint(_constrained);
}


/************************************************************************/
void iKinChain::setAllLinkVerbosity(unsigned int _verbose)
{
    for (unsigned int i=0; i<N; i++)
        allList[i]->setVerbosity(_verbose);
}


/************************************************************************/
void iKinChain::build()
{
    quickList.clear();
    hash.clear();
    hash_dof.clear();
    DOF=0;

    Matrix H=eye(4,4);
    bool cumulOn=false;

    for (unsigned int i=0; i<N; i++)
    {
        allList[i]->rmCumH();

        if (allList[i]->isBlocked())
        {
            if (i==N-1)
            {    
                allList[i]->addCumH(H);
                quickList.push_back(allList[i]);
            }
            else
            {
                H=H*allList[i]->getH();
                cumulOn=true;
            }
        }
        else
        {
            if (cumulOn)
                allList[i]->addCumH(H);

            DOF++;
            quickList.push_back(allList[i]);
            hash_dof.push_back(quickList.size()-1);
            hash.push_back(i);

            H.eye();
            cumulOn=false;
        }
    }

    if (DOF>0)
        curr_q.resize(DOF,0);
}


/************************************************************************/
void iKinChain::setH0(const Matrix &_H0)
{
    if ((_H0.rows()==4) && (_H0.cols()==4))
        H0=_H0;
    else
    {
        H0=eye(4,4);

        if (verbose)
            fprintf(stderr,"Attempt to create a chain with wrong matrix H0 (not 4x4)\n");
    }
}


/************************************************************************/
Vector iKinChain::setAng(const Vector &q)
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"setAng() failed since DOF==0\n");

        return Vector(0);
    }

    int sz=(q.length()>(int)DOF)?DOF:q.length();
    for (int i=0; i<sz; i++)
        curr_q[i]=quickList[hash_dof[i]]->setAng(q[i]);

    return curr_q;
}


/************************************************************************/
Vector iKinChain::getAng()
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"getAng() failed since DOF==0\n");

        return Vector(0);
    }

    for (unsigned int i=0; i<DOF; i++)
        curr_q[i]=quickList[hash_dof[i]]->getAng();

    return curr_q;
}


/************************************************************************/
double iKinChain::setAng(const unsigned int i, double _Ang)
{
    double res=0.0;

    if (i<N)
        res=allList[i]->setAng(_Ang);
    else if (verbose)
        fprintf(stderr,"setAng() failed due to out of range index: %d>=%d\n",i,N);

    return res;
}


/************************************************************************/
double iKinChain::getAng(const unsigned int i)
{
    double res=0.0;

    if (i<N)
        res=allList[i]->getAng();
    else if (verbose)
        fprintf(stderr,"getAng() failed due to out of range index: %d>=%d\n",i,N);

    return res;
}


/************************************************************************/
Vector iKinChain::RotAng(const Matrix &R)
{
    Vector r(3);

    // Euler Angles as XYZ (see dcm2angle.m)
    r[0]=atan2(-R(2,1),R(2,2));
    r[1]=asin(R(2,0));
    r[2]=atan2(-R(1,0),R(0,0));

    return r;
}


/************************************************************************/
Vector iKinChain::dRotAng(const Matrix &R, const Matrix &dR)
{
    Vector dr(3);

    dr[0]=(R(2,1)*dR(2,2) - R(2,2)*dR(2,1)) / (R(2,1)*R(2,1) + R(2,2)*R(2,2));
    dr[1]=dR(2,0)/sqrt(fabs(1-R(2,0)*R(2,0)));
    dr[2]=(R(1,0)*dR(0,0) - R(0,0)*dR(1,0)) / (R(1,0)*R(1,0) + R(0,0)*R(0,0));

    return dr;
}


/************************************************************************/
Vector iKinChain::d2RotAng(const Matrix &R, const Matrix &dRi,
                           const Matrix &dRj, const Matrix &d2R)
{
    Vector d2r(3);

    double y,yi,yj,yij,x,xi,xj,xij;
    double tmp1,tmp2;

    y  =-R(2,1);
    yi =-dRi(2,1);
    yj =-dRj(2,1);
    yij=-d2R(2,1);
    x  = R(2,2);
    xi = dRi(2,2);
    xj = dRj(2,2);
    xij= d2R(2,2);

    tmp1  =x*x+y*y;
    d2r[0]=((xj*yi+x*yij-xij*y-xi*yj)*tmp1 - 2.0*(x*yi-xi*y)*(x*xj+y*yj)) / (tmp1*tmp1);

    x  =R(2,0);
    xi =dRi(2,0);
    xj =dRj(2,0);
    xij=d2R(2,0);

    tmp1  =1-x*x;
    tmp2  =sqrt(fabs(tmp1));
    d2r[1]=(xij-(x*xi*xj)/tmp1) / (tmp1*tmp2);

    y  =-R(1,0);
    yi =-dRi(1,0);
    yj =-dRj(1,0);
    yij=-d2R(1,0);
    x  = R(0,0);
    xi = dRi(0,0);
    xj = dRj(0,0);
    xij= d2R(0,0);

    tmp1  =x*x+y*y;
    d2r[2]=((xj*yi+x*yij-xij*y-xi*yj)*tmp1 - 2.0*(x*yi-xi*y)*(x*xj+y*yj)) / (tmp1*tmp1);

    return d2r;
}


/************************************************************************/
Matrix iKinChain::getH(const unsigned int i, const bool allLink)
{
    Matrix H=H0;
    unsigned int _i,n;
    deque<iKinLink*> *l;
    bool c_override;

    if (allLink)
    {
        n=N;
        l=&allList;
        c_override=true;

        _i=i;
    }
    else
    {
        n=DOF;
        l=&quickList;
        c_override=false;

        if (i==DOF)
            _i=quickList.size();
        else
            _i=i;
    }

    if (i<n)
        for (unsigned int j=0; j<=_i; j++)
            H=H*((*l)[j]->getH(c_override));
    else if (verbose)
        fprintf(stderr,"getH() failed due to out of range index: %d>=%d\n",i,n);

    return H;
}


/************************************************************************/
Matrix iKinChain::getH()
{
    // may be different from DOF since one blocked link may lie
    // at the end of the chain.
    size_t n=quickList.size();
    Matrix H=H0;

    for (unsigned int i=0; i<n; i++)
        H=H*quickList[i]->getH();

    return H;
}


/************************************************************************/
Matrix iKinChain::getH(const Vector &q)
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"getH() failed since DOF==0\n");
    
        return Matrix(0,0);
    }

    setAng(q);
    return getH();
}


/************************************************************************/
Vector iKinChain::Pose(const unsigned int i, const bool axisRep)
{
    Matrix H=getH(i,true);
    Vector v;

    if (i<N)
    {
        if (axisRep)
        {
            v.resize(7);
            Vector r=dcm2axis(H,verbose);
            v[0]=H(0,3);
            v[1]=H(1,3);
            v[2]=H(2,3);
            v[3]=r[0];
            v[4]=r[1];
            v[5]=r[2];
            v[6]=r[3];
        }
        else
        {
            v.resize(6);
            Vector r=RotAng(H);
            v[0]=H(0,3);
            v[1]=H(1,3);
            v[2]=H(2,3);
            v[3]=r[0];
            v[4]=r[1];
            v[5]=r[2];
        }
    }
    else if (verbose)
        fprintf(stderr,"Pose() failed due to out of range index: %d>=%d\n",i,N);

    return v;
}


/************************************************************************/
Vector iKinChain::EndEffPose(const bool axisRep)
{
    Matrix H=getH();
    Vector v;

    if (axisRep)
    {
        v.resize(7);
        Vector r=dcm2axis(H,verbose);
        v[0]=H(0,3);
        v[1]=H(1,3);
        v[2]=H(2,3);
        v[3]=r[0];
        v[4]=r[1];
        v[5]=r[2];
        v[6]=r[3];
    }
    else
    {
        v.resize(6);
        Vector r=RotAng(H);
        v[0]=H(0,3);
        v[1]=H(1,3);
        v[2]=H(2,3);
        v[3]=r[0];
        v[4]=r[1];
        v[5]=r[2];
    }

    return v;
}


/************************************************************************/
Vector iKinChain::EndEffPose(const Vector &q, const bool axisRep)
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"EndEffPose() failed since DOF==0\n");
    
        return Vector(0);
    }

    setAng(q);
    return EndEffPose(axisRep);
}


/************************************************************************/
Matrix iKinChain::AnaJacobian(const unsigned int i, unsigned int col)
{
    if (i>=N)
    {
        if (verbose)
            fprintf(stderr,"AnaJacobian() failed due to out of range index: %d>=%d\n",i,N);

        return Matrix(0,0);
    }

    col=col>3 ? 3 : col;

    Matrix J(6,i);
    Matrix H,dH,_H;
    Vector dr;

    for (unsigned int j=0; j<i; j++)
    {
        H=dH=H0;

        for (unsigned int k=0; k<i; k++)
        {
            _H=allList[k]->getH();
            H=H*_H;

            if (j==k)
                dH=dH*allList[k]->getDnH();
            else
                dH=dH*_H;
        }

        dr=dRotAng(H,dH);

        J(0,j)=dH(0,col);
        J(1,j)=dH(1,col);
        J(2,j)=dH(2,col);
        J(3,j)=dr[0];
        J(4,j)=dr[1];
        J(5,j)=dr[2];
    }

    return J;
}


/************************************************************************/
Matrix iKinChain::AnaJacobian(unsigned int col)
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"AnaJacobian() failed since DOF==0\n");

        return Matrix(0,0);
    }

    col=col>3 ? 3 : col;

    // may be different from DOF since one blocked link may lie
    // at the end of the chain.
    size_t n=quickList.size();
    Matrix J(6,DOF);
    Matrix H,dH,_H;
    Vector dr;

    for (unsigned int i=0; i<DOF; i++)
    {
        H=dH=H0;

        for (unsigned int j=0; j<n; j++)
        {
            _H=quickList[j]->getH();
            H=H*_H;

            if (hash_dof[i]==j)
                dH=dH*quickList[j]->getDnH();
            else
                dH=dH*_H;
        }

        dr=dRotAng(H,dH);

        J(0,i)=dH(0,col);
        J(1,i)=dH(1,col);
        J(2,i)=dH(2,col);
        J(3,i)=dr[0];
        J(4,i)=dr[1];
        J(5,i)=dr[2];
    }

    return J;
}


/************************************************************************/
Matrix iKinChain::AnaJacobian(const Vector &q, unsigned int col)
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"AnaJacobian() failed since DOF==0\n");
    
        return Matrix(0,0);
    }

    setAng(q);
    return AnaJacobian(col);
}


/************************************************************************/
Matrix iKinChain::GeoJacobian(const unsigned int i)
{
    if (i>=N)
    {
        if (verbose)
            fprintf(stderr,"GeoJacobian() failed due to out of range index: %d>=%d\n",i,N);

        return Matrix(0,0);
    }

    Matrix J(6,i+1);
    Matrix Pn,Z;
    Vector w;

    deque<Matrix> intH;
    intH.push_back(H0);

    for (unsigned int j=0; j<=i; j++)
        intH.push_back(intH[j]*allList[j]->getH(true));

    Pn=intH[i+1];

    for (unsigned int j=0; j<=i; j++)
    {
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


/************************************************************************/
Matrix iKinChain::GeoJacobian()
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"GeoJacobian() failed since DOF==0\n");

        return Matrix(0,0);
    }

    Matrix J(6,DOF);
    Matrix Pn,Z;
    Vector w;

    deque<Matrix> intH;
    intH.push_back(H0);

    for (unsigned int i=0; i<N; i++)
        intH.push_back(intH[i]*allList[i]->getH(true));

    Pn=intH[N];

    for (unsigned int i=0; i<DOF; i++)
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


/************************************************************************/
Matrix iKinChain::GeoJacobian(const Vector &q)
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"GeoJacobian() failed since DOF==0\n");
    
        return Matrix(0,0);
    }

    setAng(q);
    return GeoJacobian();
}


/************************************************************************/
Vector iKinChain::Hessian_ij(const unsigned int i, const unsigned int j)
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"Hessian_ij() failed since DOF==0\n");

        return Vector(0);
    }

    if ((i>=DOF) || (j>=DOF))
    {
        if (verbose)
            fprintf(stderr,"Hessian_ij() failed due to out of range index\n");

        return Vector(0);
    }

    deque<Matrix> intH;
    deque<Matrix> intDH;
    intH.push_back(H0);
    intDH.push_back(H0);

    for (unsigned int k=0; k<N; k++)
    {
        intH.push_back(intH[k]*allList[k]->getH(true));

        if (k==hash[i])
            intDH.push_back(intDH[k]*allList[k]->getDnH(1,true));
        else
            intDH.push_back(intDH[k]*allList[k]->getH(true));
    }

    unsigned int k=hash[j];

    Matrix DZ=intDH[k];

    Vector Dw=Dcross(intH[k],DZ,2,intH[N]-intH[k],intDH[N]-DZ,3,verbose);
    Vector h(6);

    h[0]=Dw[0];
    h[1]=Dw[1];
    h[2]=Dw[2];
    h[3]=DZ(0,2);
    h[4]=DZ(1,2);
    h[5]=DZ(2,2);

    return h;
}


/************************************************************************/
void iKinChain::prepareForHessian()
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"prepareForHessian() failed since DOF==0\n");

        return;
    }

    if (hess_DH!=NULL)
        delete[] hess_DH;

    hess_H.clear();
    hess_DH=new deque<Matrix>[DOF];

    hess_H.push_back(H0);

    for (unsigned int i=0; i<DOF; i++)
        hess_DH[i].push_back(H0);

    for (unsigned int k=0; k<N; k++)
    {
        hess_H.push_back(hess_H[k]*allList[k]->getH(true));

        for (unsigned int i=0; i<DOF; i++)
            if (k==hash[i])
                hess_DH[i].push_back(hess_DH[i][k]*allList[k]->getDnH(1,true));
            else
                hess_DH[i].push_back(hess_DH[i][k]*allList[k]->getH(true));
    }
}


/************************************************************************/
Vector iKinChain::fastHessian_ij(const unsigned int i, const unsigned int j)
{
    if (DOF==0)
    {
        if (verbose)
            fprintf(stderr,"fastHessian_ij() failed since DOF==0\n");

        return Vector(0);
    }

    if ((i>=DOF) || (j>=DOF))
    {
        if (verbose)
            fprintf(stderr,"fastHessian_ij() failed due to out of range index\n");

        return Vector(0);
    }

    unsigned int k=hash[j];

    Matrix DZ=hess_DH[i][k];

    Vector Dw=Dcross(hess_H[k],DZ,2,hess_H[N]-hess_H[k],hess_DH[i][N]-hess_DH[i][k],3,verbose);
    Vector h(6);

    h[0]=Dw[0];
    h[1]=Dw[1];
    h[2]=Dw[2];
    h[3]=DZ(0,2);
    h[4]=DZ(1,2);
    h[5]=DZ(2,2);

    return h;
}


/************************************************************************/
iKinChain::~iKinChain()
{
	dispose();
}


/************************************************************************/
void iKinChain::dispose()
{
    allList.clear();
    quickList.clear();

    if (hess_DH!=NULL)
    {
        delete[] hess_DH;
        hess_DH=NULL;
    }
}


/************************************************************************/
iKinLimb::iKinLimb()
{
    allocate("right");
}


/************************************************************************/
iKinLimb::iKinLimb(const string &_type)
{
    allocate(_type);
}


/************************************************************************/
iKinLimb::iKinLimb(const iKinLimb &limb)
{
    clone(limb);
}


/************************************************************************/
iKinLimb::iKinLimb(const Property &option)
{
    fromLinksProperties(option);
}


/************************************************************************/
bool iKinLimb::fromLinksProperties(const Property &option)
{
    Property &opt=const_cast<Property&>(option);

    dispose();    

    type=opt.check("type",Value("right")).asString().c_str();
    if (type!="right" && type!="left")
    {
        fprintf(stderr,"Error: invalid handedness type specified!\n");
        return false;
    }

    if (Bottle *bH0=opt.find("H0").asList())
    {
        int i=0;
        int j=0;

        H0.zero();

        for (int cnt=0; (cnt<bH0->size()) && (cnt<H0.rows()*H0.cols()); cnt++)
        {    
            H0(i,j)=bH0->get(cnt).asDouble();

            if (++j>=H0.cols())
            {
                i++;
                j=0;
            }
        }
    }

    int numLinks=opt.check("numLinks",Value(0)).asInt();
    if (numLinks==0)
    {
        fprintf(stderr,"Error: invalid number of links specified!\n");

        type="right";
        H0.eye();

        return false;
    }

    linkList.resize(numLinks,NULL);

    for (int i=0; i<numLinks; i++)
    {
        char link[255];
        sprintf(link,"link_%d",i);

        Bottle &bLink=opt.findGroup(link);
        if (bLink.isNull())
        {
            fprintf(stderr,"Error: %s is missing!\n",link);

            type="right";
            H0.eye();
            dispose();

            return false;
        }

        double A=bLink.check("A",Value(0.0)).asDouble();
        double D=bLink.check("D",Value(0.0)).asDouble();
        double alpha=CTRL_DEG2RAD*bLink.check("alpha",Value(0.0)).asDouble();
        double offset=CTRL_DEG2RAD*bLink.check("offset",Value(0.0)).asDouble();
        double min=CTRL_DEG2RAD*bLink.check("min",Value(0.0)).asDouble();
        double max=CTRL_DEG2RAD*bLink.check("max",Value(0.0)).asDouble();

        linkList[i]=new iKinLink(A,D,alpha,offset,min,max);

        *this<<*linkList[i];

        if (bLink.check("blocked"))
            blockLink(i,CTRL_DEG2RAD*bLink.find("blocked").asDouble());
    }

    return configured=true;
}


/************************************************************************/
iKinLimb &iKinLimb::operator=(const iKinLimb &limb)
{
    dispose();	
    clone(limb);

    return *this;
}


/************************************************************************/
iKinLimb::~iKinLimb()
{
    dispose();
}


/************************************************************************/
void iKinLimb::allocate(const string &_type)
{
    type=_type;

    if (type!="right" && type!="left")
        type="right";

    configured=true;
}


/************************************************************************/
void iKinLimb::clone(const iKinLimb &limb)
{
    type=limb.type;
    H0=limb.H0;

    if (unsigned int n=limb.linkList.size())
    {
        linkList.resize(n);

        for (unsigned int i=0; i<n; i++)
        {
            linkList[i]=new iKinLink(*limb.linkList[i]);
            *this<<*linkList[i];
        }
    }

    configured=limb.configured;
}


/************************************************************************/
void iKinLimb::dispose()
{
    if (unsigned int n=linkList.size())
    {
        for (unsigned int i=0; i<n; i++)
            if (linkList[i]!=NULL)
                delete linkList[i];

        linkList.clear();
    }

    iKinChain::dispose();

    configured=false;
}


/************************************************************************/
iCubArm::iCubArm()
{
    allocate("right");
}


/************************************************************************/
iCubArm::iCubArm(const string &_type)
{
    allocate(_type);
}


/************************************************************************/
iCubArm::iCubArm(const iCubArm &arm)
{
    clone(arm);
}


/************************************************************************/
void iCubArm::allocate(const string &_type)
{
    iKinLimb::allocate(_type);

    H0.zero();
    H0(0,1)=-1;
    H0(1,2)=-1;
    H0(2,0)=1;
    H0(3,3)=1;

    linkList.resize(10);

    if (type=="right")
    {
        linkList[0]=new iKinLink(     0.032,      0.0,  M_PI/2.0,                 0.0, -22.0*CTRL_DEG2RAD,  84.0*CTRL_DEG2RAD);
        linkList[1]=new iKinLink(       0.0,  -0.0055,  M_PI/2.0,           -M_PI/2.0, -39.0*CTRL_DEG2RAD,  39.0*CTRL_DEG2RAD);
        linkList[2]=new iKinLink(-0.0233647,  -0.1433,  M_PI/2.0, -105.0*CTRL_DEG2RAD, -59.0*CTRL_DEG2RAD,  59.0*CTRL_DEG2RAD);
        linkList[3]=new iKinLink(       0.0, -0.10774,  M_PI/2.0,           -M_PI/2.0, -95.5*CTRL_DEG2RAD,   5.0*CTRL_DEG2RAD);
        linkList[4]=new iKinLink(       0.0,      0.0, -M_PI/2.0,           -M_PI/2.0,                0.0, 160.8*CTRL_DEG2RAD);
        linkList[5]=new iKinLink(       0.0, -0.15228, -M_PI/2.0, -105.0*CTRL_DEG2RAD, -37.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[6]=new iKinLink(     0.015,      0.0,  M_PI/2.0,                 0.0,   5.5*CTRL_DEG2RAD, 106.0*CTRL_DEG2RAD);
        linkList[7]=new iKinLink(       0.0,  -0.1373,  M_PI/2.0,           -M_PI/2.0, -90.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[8]=new iKinLink(       0.0,      0.0,  M_PI/2.0,            M_PI/2.0, -90.0*CTRL_DEG2RAD,   0.0*CTRL_DEG2RAD);
        linkList[9]=new iKinLink(    0.0625,    0.016,       0.0,                M_PI, -20.0*CTRL_DEG2RAD,  40.0*CTRL_DEG2RAD);
    }
    else
    {
        linkList[0]=new iKinLink(     0.032,      0.0,  M_PI/2.0,                 0.0, -22.0*CTRL_DEG2RAD,  84.0*CTRL_DEG2RAD);
        linkList[1]=new iKinLink(       0.0,  -0.0055,  M_PI/2.0,           -M_PI/2.0, -39.0*CTRL_DEG2RAD,  39.0*CTRL_DEG2RAD);
        linkList[2]=new iKinLink( 0.0233647,  -0.1433, -M_PI/2.0,  105.0*CTRL_DEG2RAD, -59.0*CTRL_DEG2RAD,  59.0*CTRL_DEG2RAD);
        linkList[3]=new iKinLink(       0.0,  0.10774, -M_PI/2.0,            M_PI/2.0, -95.5*CTRL_DEG2RAD,   5.0*CTRL_DEG2RAD);
        linkList[4]=new iKinLink(       0.0,      0.0,  M_PI/2.0,           -M_PI/2.0,                0.0, 160.8*CTRL_DEG2RAD);
        linkList[5]=new iKinLink(       0.0,  0.15228, -M_PI/2.0,   75.0*CTRL_DEG2RAD, -37.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[6]=new iKinLink(    -0.015,      0.0,  M_PI/2.0,                 0.0,   5.5*CTRL_DEG2RAD, 106.0*CTRL_DEG2RAD);
        linkList[7]=new iKinLink(       0.0,   0.1373,  M_PI/2.0,           -M_PI/2.0, -90.0*CTRL_DEG2RAD,  90.0*CTRL_DEG2RAD);
        linkList[8]=new iKinLink(       0.0,      0.0,  M_PI/2.0,            M_PI/2.0, -90.0*CTRL_DEG2RAD,   0.0*CTRL_DEG2RAD);
        linkList[9]=new iKinLink(    0.0625,   -0.016,       0.0,                 0.0, -20.0*CTRL_DEG2RAD,  40.0*CTRL_DEG2RAD);
    }

    for (unsigned int i=0; i<linkList.size(); i++)
        *this<<*linkList[i];

    blockLink(0,0.0);
    blockLink(1,0.0);
    blockLink(2,0.0);
}


/************************************************************************/
bool iCubArm::alignJointsBounds(const deque<IControlLimits*> &lim)
{
    if (lim.size()<2)
        return false;

    IControlLimits &limTorso=*lim[0];
    IControlLimits &limArm  =*lim[1];

    unsigned int iTorso;
    unsigned int iArm;
    double min, max;

    for (iTorso=0; iTorso<3; iTorso++)
    {   
        if (!limTorso.getLimits(iTorso,&min,&max))
            return false;

        (*this)[2-iTorso].setMin(CTRL_DEG2RAD*min);
        (*this)[2-iTorso].setMax(CTRL_DEG2RAD*max);
    }

    for (iArm=0; iArm<getN()-iTorso; iArm++)
    {   
        if (!limArm.getLimits(iArm,&min,&max))
            return false;

        (*this)[iTorso+iArm].setMin(CTRL_DEG2RAD*min);
        (*this)[iTorso+iArm].setMax(CTRL_DEG2RAD*max);
    }

    return true;
}


/************************************************************************/
iCubLeg::iCubLeg()
{
    allocate("right");
}


/************************************************************************/
iCubLeg::iCubLeg(const string &_type)
{
    allocate(_type);
}


/************************************************************************/
iCubLeg::iCubLeg(const iCubLeg &leg)
{
    clone(leg);
}


/************************************************************************/
void iCubLeg::allocate(const string &_type)
{
    iKinLimb::allocate(_type);

    H0.zero();
    H0(0,0)=1;
    H0(1,2)=1;    
    H0(2,1)=-1;
    H0(2,3)=-0.1199;
    H0(3,3)=1;

    linkList.resize(6);

    if (type=="right")
    {
        H0(1,3)=0.0681;

        linkList[0]=new iKinLink(   0.0,     0.0,  M_PI/2.0,  M_PI/2.0,  -44.0*CTRL_DEG2RAD, 132.0*CTRL_DEG2RAD);
        linkList[1]=new iKinLink(   0.0,     0.0,  M_PI/2.0,  M_PI/2.0,  -17.0*CTRL_DEG2RAD, 119.0*CTRL_DEG2RAD);
        linkList[2]=new iKinLink(   0.0,  0.2236, -M_PI/2.0, -M_PI/2.0,  -79.0*CTRL_DEG2RAD,  79.0*CTRL_DEG2RAD);
        linkList[3]=new iKinLink(-0.213,     0.0,      M_PI,  M_PI/2.0, -125.0*CTRL_DEG2RAD,  23.0*CTRL_DEG2RAD);
        linkList[4]=new iKinLink(   0.0,     0.0,  M_PI/2.0,       0.0,  -42.0*CTRL_DEG2RAD,  21.0*CTRL_DEG2RAD);
        linkList[5]=new iKinLink(-0.041,     0.0,      M_PI,       0.0,  -24.0*CTRL_DEG2RAD,  24.0*CTRL_DEG2RAD);
    }
    else
    {
        H0(1,3)=-0.0681;

        linkList[0]=new iKinLink(   0.0,     0.0, -M_PI/2.0,  M_PI/2.0,  -44.0*CTRL_DEG2RAD, 132.0*CTRL_DEG2RAD);
        linkList[1]=new iKinLink(   0.0,     0.0, -M_PI/2.0,  M_PI/2.0,  -17.0*CTRL_DEG2RAD, 119.0*CTRL_DEG2RAD);
        linkList[2]=new iKinLink(   0.0, -0.2236,  M_PI/2.0, -M_PI/2.0,  -79.0*CTRL_DEG2RAD,  79.0*CTRL_DEG2RAD);
        linkList[3]=new iKinLink(-0.213,     0.0,      M_PI,  M_PI/2.0, -125.0*CTRL_DEG2RAD,  23.0*CTRL_DEG2RAD);
        linkList[4]=new iKinLink(   0.0,     0.0, -M_PI/2.0,       0.0,  -42.0*CTRL_DEG2RAD,  21.0*CTRL_DEG2RAD);
        linkList[5]=new iKinLink(-0.041,     0.0,       0.0,       0.0,  -24.0*CTRL_DEG2RAD,  24.0*CTRL_DEG2RAD);
    }

    for (unsigned int i=0; i<linkList.size(); i++)
        *this<<*linkList[i];
}


/************************************************************************/
bool iCubLeg::alignJointsBounds(const deque<IControlLimits*> &lim)
{
    if (lim.size()<1)
        return false;

    IControlLimits &limLeg=*lim[0];

    unsigned int iLeg;
    double min, max;

    for (iLeg=0; iLeg<getN(); iLeg++)
    {   
        if (!limLeg.getLimits(iLeg,&min,&max))
            return false;

        (*this)[iLeg].setMin(CTRL_DEG2RAD*min);
        (*this)[iLeg].setMax(CTRL_DEG2RAD*max);
    }

    return true;
}


/************************************************************************/
iCubEye::iCubEye()
{
    allocate("right");
}


/************************************************************************/
iCubEye::iCubEye(const string &_type)
{
    allocate(_type);
}


/************************************************************************/
iCubEye::iCubEye(const iCubEye &eye)
{
    clone(eye);
}


/************************************************************************/
void iCubEye::allocate(const string &_type)
{
    iKinLimb::allocate(_type);

    H0.zero();
    H0(0,1)=-1;
    H0(1,2)=-1;
    H0(2,0)=1;
    H0(3,3)=1;

    linkList.resize(8);

    if (type=="right")
    {
        linkList[0]=new iKinLink(   0.032,    0.0,  M_PI/2.0,       0.0, -22.0*CTRL_DEG2RAD, 84.0*CTRL_DEG2RAD);
        linkList[1]=new iKinLink(     0.0,-0.0055,  M_PI/2.0, -M_PI/2.0, -39.0*CTRL_DEG2RAD, 39.0*CTRL_DEG2RAD);
        linkList[2]=new iKinLink( 0.00231,-0.1933, -M_PI/2.0, -M_PI/2.0, -59.0*CTRL_DEG2RAD, 59.0*CTRL_DEG2RAD);
        linkList[3]=new iKinLink(   0.033,    0.0,  M_PI/2.0,  M_PI/2.0, -40.0*CTRL_DEG2RAD, 30.0*CTRL_DEG2RAD);
        linkList[4]=new iKinLink(     0.0,  0.001, -M_PI/2.0, -M_PI/2.0, -70.0*CTRL_DEG2RAD, 60.0*CTRL_DEG2RAD);
        linkList[5]=new iKinLink(  -0.054, 0.0825, -M_PI/2.0,  M_PI/2.0, -55.0*CTRL_DEG2RAD, 55.0*CTRL_DEG2RAD);
        linkList[6]=new iKinLink(     0.0,  0.034, -M_PI/2.0,       0.0, -35.0*CTRL_DEG2RAD, 15.0*CTRL_DEG2RAD);
        linkList[7]=new iKinLink(     0.0,    0.0,  M_PI/2.0, -M_PI/2.0, -50.0*CTRL_DEG2RAD, 50.0*CTRL_DEG2RAD);
    }
    else
    {
        linkList[0]=new iKinLink(   0.032,    0.0,  M_PI/2.0,       0.0, -22.0*CTRL_DEG2RAD, 84.0*CTRL_DEG2RAD);
        linkList[1]=new iKinLink(     0.0,-0.0055,  M_PI/2.0, -M_PI/2.0, -39.0*CTRL_DEG2RAD, 39.0*CTRL_DEG2RAD);
        linkList[2]=new iKinLink( 0.00231,-0.1933, -M_PI/2.0, -M_PI/2.0, -59.0*CTRL_DEG2RAD, 59.0*CTRL_DEG2RAD);
        linkList[3]=new iKinLink(   0.033,    0.0,  M_PI/2.0,  M_PI/2.0, -40.0*CTRL_DEG2RAD, 30.0*CTRL_DEG2RAD);
        linkList[4]=new iKinLink(     0.0,  0.001, -M_PI/2.0, -M_PI/2.0, -70.0*CTRL_DEG2RAD, 60.0*CTRL_DEG2RAD);
        linkList[5]=new iKinLink(  -0.054, 0.0825, -M_PI/2.0,  M_PI/2.0, -55.0*CTRL_DEG2RAD, 55.0*CTRL_DEG2RAD);
        linkList[6]=new iKinLink(     0.0, -0.034, -M_PI/2.0,       0.0, -35.0*CTRL_DEG2RAD, 15.0*CTRL_DEG2RAD);
        linkList[7]=new iKinLink(     0.0,    0.0,  M_PI/2.0, -M_PI/2.0, -50.0*CTRL_DEG2RAD, 50.0*CTRL_DEG2RAD);
    }

    for (unsigned int i=0; i<linkList.size(); i++)
        *this<<*linkList[i];

    blockLink(0,0.0);
    blockLink(1,0.0);
    blockLink(2,0.0);
}


/************************************************************************/
bool iCubEye::alignJointsBounds(const deque<IControlLimits*> &lim)
{
    if (lim.size()<2)
        return false;

    IControlLimits &limTorso=*lim[0];
    IControlLimits &limHead =*lim[1];

    unsigned int iTorso;
    unsigned int iHead;
    double min, max;

    for (iTorso=0; iTorso<3; iTorso++)
    {   
        if (!limTorso.getLimits(iTorso,&min,&max))
            return false;

        (*this)[2-iTorso].setMin(CTRL_DEG2RAD*min);
        (*this)[2-iTorso].setMax(CTRL_DEG2RAD*max);
    }

    for (iHead=0; iHead<getN()-iTorso; iHead++)
    {   
        if (!limHead.getLimits(iHead,&min,&max))
            return false;

        (*this)[iTorso+iHead].setMin(CTRL_DEG2RAD*min);
        (*this)[iTorso+iHead].setMax(CTRL_DEG2RAD*max);
    }

    return true;
}


/************************************************************************/
iCubEyeNeckRef::iCubEyeNeckRef()
{
    allocate("right");
}


/************************************************************************/
iCubEyeNeckRef::iCubEyeNeckRef(const string &_type)
{
    allocate(_type);
}


/************************************************************************/
iCubEyeNeckRef::iCubEyeNeckRef(const iCubEyeNeckRef &eye)
{
    clone(eye);
}


/************************************************************************/
void iCubEyeNeckRef::allocate(const string &_type)
{
    rmLink(0);
    rmLink(0);
    rmLink(0);

    delete linkList[0];
    delete linkList[1];
    delete linkList[2];

    linkList.erase(linkList.begin(),linkList.begin()+2);
}


/************************************************************************/
iCubInertialSensor::iCubInertialSensor()
{
    allocate("right");
}


/************************************************************************/
iCubInertialSensor::iCubInertialSensor(const iCubInertialSensor &sensor)
{
    clone(sensor);
}


/************************************************************************/
void iCubInertialSensor::allocate(const string &_type)
{
    iKinLimb::allocate(_type);

    H0.zero();
    H0(0,1)=-1;
    H0(1,2)=-1;
    H0(2,0)=1;
    H0(3,3)=1;

    linkList.resize(7);

    // links of torso and neck
    linkList[0]=new iKinLink(   0.032,       0.0,  M_PI/2.0,       0.0, -22.0*CTRL_DEG2RAD, 84.0*CTRL_DEG2RAD);
    linkList[1]=new iKinLink(     0.0,   -0.0055,  M_PI/2.0, -M_PI/2.0, -39.0*CTRL_DEG2RAD, 39.0*CTRL_DEG2RAD);
    linkList[2]=new iKinLink( 0.00231,   -0.1933, -M_PI/2.0, -M_PI/2.0, -59.0*CTRL_DEG2RAD, 59.0*CTRL_DEG2RAD);
    linkList[3]=new iKinLink(   0.033,       0.0,  M_PI/2.0,  M_PI/2.0, -40.0*CTRL_DEG2RAD, 30.0*CTRL_DEG2RAD);
    linkList[4]=new iKinLink(     0.0,     0.001, -M_PI/2.0, -M_PI/2.0, -70.0*CTRL_DEG2RAD, 60.0*CTRL_DEG2RAD);
    linkList[5]=new iKinLink(  0.0225,    0.1005, -M_PI/2.0,  M_PI/2.0, -55.0*CTRL_DEG2RAD, 55.0*CTRL_DEG2RAD);

    // virtual link that describes T_nls (see http://eris.liralab.it/wiki/ICubInertiaSensorKinematics )
    linkList[6]=new iKinLink(     0.0,    0.0066,  M_PI/2.0,       0.0,                0.0,               0.0);

    for (unsigned int i=0; i<linkList.size(); i++)
        *this<<*linkList[i];

    // block virtual links
    blockLink(6,0.0);
}


/************************************************************************/
bool iCubInertialSensor::alignJointsBounds(const deque<IControlLimits*> &lim)
{
    if (lim.size()<2)
        return false;

    IControlLimits &limTorso=*lim[0];
    IControlLimits &limHead =*lim[1];

    unsigned int iTorso;
    unsigned int iHead;
    double min, max;

    for (iTorso=0; iTorso<3; iTorso++)
    {   
        if (!limTorso.getLimits(iTorso,&min,&max))
            return false;

        (*this)[2-iTorso].setMin(CTRL_DEG2RAD*min);
        (*this)[2-iTorso].setMax(CTRL_DEG2RAD*max);
    }

    // only the neck
    for (iHead=0; iHead<3; iHead++)
    {   
        if (!limHead.getLimits(iHead,&min,&max))
            return false;

        (*this)[iTorso+iHead].setMin(CTRL_DEG2RAD*min);
        (*this)[iTorso+iHead].setMax(CTRL_DEG2RAD*max);
    }

    return true;
}



