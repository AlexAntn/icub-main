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

/**
 * \defgroup servercartesiancontroller servercartesiancontroller
 * @ingroup icub_hardware_modules 
 *  
 * Implements the server part of the <a 
 * href="http://eris.liralab.it/yarpdoc/dd/de6/classyarp_1_1dev_1_1ICartesianControl.html">Cartesian 
 * Interface</a>. 
 *  
 * @note Please read carefully the \ref icub_cartesian_interface
 *       "Cartesian Interface" documentation.
 *
 * Copyright (C) 2010 RobotCub Consortium.
 *
 * Author: Ugo Pattacini
 *
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 * This file can be edited at 
 * src/modules/cartesianController/ServerCartesianController.h 
 */

#ifndef __SERVERCARTESIANCONTROLLER_H__
#define __SERVERCARTESIANCONTROLLER_H__

#include <yarp/os/Property.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/Semaphore.h>
#include <yarp/os/Event.h>
#include <yarp/os/Stamp.h>
#include <yarp/sig/Vector.h>

#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/Wrapper.h>
#include <yarp/dev/CartesianControl.h>

#include <iCub/ctrl/pids.h>
#include <iCub/iKin/iKinHlp.h>
#include <iCub/iKin/iKinFwd.h>
#include <iCub/iKin/iKinInv.h>

#include <deque>
#include <map>

#include "SmithPredictor.h"


class ServerCartesianController;


struct DriverDescriptor
{
    yarp::os::ConstString key;
    bool jointsDirectOrder;

    yarp::sig::Vector minAbsVels;
    bool useDefaultMinAbsVel;
};


class CartesianCtrlRpcProcessor : public yarp::os::PortReader
{
protected:
    ServerCartesianController *ctrl;

    bool read(yarp::os::ConnectionReader &connection);

public:
    CartesianCtrlRpcProcessor(ServerCartesianController *_ctrl);
};


class CartesianCtrlCommandPort : public yarp::os::BufferedPort<yarp::os::Bottle>
{
protected:
    ServerCartesianController *ctrl;

    void onRead(yarp::os::Bottle &command);

public:
    CartesianCtrlCommandPort(ServerCartesianController *_ctrl);
};


class ServerCartesianController : public    yarp::dev::DeviceDriver,
                                  public    yarp::dev::IMultipleWrapper,
                                  public    yarp::dev::ICartesianControl,
                                  public    yarp::os::RateThread,
                                  protected iCub::iKin::CartesianHelper
{
protected:
    yarp::dev::PolyDriverList drivers;

    bool attached;
    bool connected;
    bool closed;
    bool trackingMode;
    bool executingTraj;
    bool taskVelModeOn;
    bool motionDone;
    bool encTimedEnabled;

    yarp::os::ConstString ctrlName;
    yarp::os::ConstString slvName;
    yarp::os::ConstString kinPart;
    yarp::os::ConstString kinType;
    int numDrv;

    iCub::iKin::iKinLimb            *limb;
    iCub::iKin::iKinChain           *chain;
    iCub::iKin::MultiRefMinJerkCtrl *ctrl;
    iCub::ctrl::Integrator          *taskRefVelTargetGen;

    yarp::os::Property plantModelProperties;
    SmithPredictor     smithPredictor;
    
    std::deque<DriverDescriptor>             lDsc;
    std::deque<yarp::dev::IControlLimits*>   lLim;
    std::deque<yarp::dev::IEncoders*>        lEnc;
    std::deque<yarp::dev::IEncodersTimed*>   lEnt;
    std::deque<yarp::dev::IVelocityControl*> lVel;
    std::deque<int>                          lJnt;
    std::deque<int*>                         lRmp;

    unsigned int connectCnt;
    unsigned int ctrlPose;
    int          maxPartJoints;
    double       targetTol;
    double       trajTime;
    int          taskRefVelPeriodFactor;
    int          taskRefVelPeriodCnt;

    double       txToken;
    double       rxToken;
    double       txTokenLatchedStopControl;
    double       txTokenLatchedGoToRpc;
    bool         skipSlvRes;
    bool         syncEventEnabled;

    yarp::os::Semaphore mutex;
    yarp::os::Event     syncEvent;
    yarp::os::Stamp     txInfo;
    yarp::os::Stamp     poseInfo;

    yarp::sig::Vector xdes;
    yarp::sig::Vector qdes;
    yarp::sig::Vector xdot_set;
    yarp::sig::Vector velCmd;
    yarp::sig::Vector fb;

    yarp::os::BufferedPort<yarp::os::Bottle>   portSlvIn;
    yarp::os::BufferedPort<yarp::os::Bottle>   portSlvOut;
    yarp::os::Port                             portSlvRpc;

    yarp::os::BufferedPort<yarp::sig::Vector>  portState;
    yarp::os::Port                             portEvent;
    yarp::os::Port                             portRpc;

    CartesianCtrlCommandPort                  *portCmd;
    CartesianCtrlRpcProcessor                 *rpcProcessor;

    struct Context
    {
        yarp::sig::Vector dof;
        yarp::sig::Vector restPos;
        yarp::sig::Vector restWeights;
        yarp::sig::Matrix limits;
        double            trajTime;
        double            tol;
        bool              mode;
    };

    int contextIdCnt;
    std::map<int,Context> contextMap;
    std::map<std::string,yarp::dev::CartesianEvent*> eventsMap;

    void   init();
    void   openPorts();
    void   closePorts();
    bool   respond(const yarp::os::Bottle &command, yarp::os::Bottle &reply);
    void   stopLimbVel();
    void   alignJointsBounds();
    double getFeedback(yarp::sig::Vector &_fb);
    void   newController();
    bool   getNewTarget();
    void   sendVelocity(const yarp::sig::Vector &v);
    bool   goTo(unsigned int _ctrlPose, const yarp::sig::Vector &xd, const double t, const bool latchToken=false);
    bool   deleteContexts(yarp::os::Bottle *contextIdList);
    void   notifyEvent(const std::string &event);

    bool   threadInit();
    void   afterStart(bool s);
    void   run();
    void   threadRelease();

    friend class CartesianCtrlRpcProcessor;
    friend class CartesianCtrlCommandPort;

    bool stopControlNoMutex();
    bool setTrajTimeNoMutex(const double t);
    bool setInTargetTolNoMutex(const double tol);

public:
    ServerCartesianController();
    ServerCartesianController(yarp::os::Searchable &config);

    bool open(yarp::os::Searchable &config);
    bool close();

    bool pingSolver();
    bool connectToSolver();

    bool attachAll(const yarp::dev::PolyDriverList &p);
    bool detachAll();

    bool setTrackingMode(const bool f);
    bool getTrackingMode(bool *f);
    bool getPose(yarp::sig::Vector &x, yarp::sig::Vector &o, yarp::os::Stamp *stamp=NULL);
    bool getPose(const int axis, yarp::sig::Vector &x, yarp::sig::Vector &o, yarp::os::Stamp *stamp=NULL);
    bool goToPose(const yarp::sig::Vector &xd, const yarp::sig::Vector &od, const double t=0.0);
    bool goToPosition(const yarp::sig::Vector &xd, const double t=0.0);
    bool goToPoseSync(const yarp::sig::Vector &xd, const yarp::sig::Vector &od, const double t=0.0);
    bool goToPositionSync(const yarp::sig::Vector &xd, const double t=0.0);
    bool getDesired(yarp::sig::Vector &xdhat, yarp::sig::Vector &odhat, yarp::sig::Vector &qdhat);
    bool askForPose(const yarp::sig::Vector &xd, const yarp::sig::Vector &od, yarp::sig::Vector &xdhat,
                    yarp::sig::Vector &odhat, yarp::sig::Vector &qdhat);
    bool askForPose(const yarp::sig::Vector &q0, const yarp::sig::Vector &xd, const yarp::sig::Vector &od,
                    yarp::sig::Vector &xdhat, yarp::sig::Vector &odhat, yarp::sig::Vector &qdhat);
    bool askForPosition(const yarp::sig::Vector &xd, yarp::sig::Vector &xdhat, yarp::sig::Vector &odhat,
                        yarp::sig::Vector &qdhat);
    bool askForPosition(const yarp::sig::Vector &q0, const yarp::sig::Vector &xd, yarp::sig::Vector &xdhat,
                        yarp::sig::Vector &odhat, yarp::sig::Vector &qdhat);
    bool getDOF(yarp::sig::Vector &curDof);
    bool setDOF(const yarp::sig::Vector &newDof, yarp::sig::Vector &curDof);
    bool getRestPos(yarp::sig::Vector &curRestPos);
    bool setRestPos(const yarp::sig::Vector &newRestPos, yarp::sig::Vector &curRestPos);
    bool getRestWeights(yarp::sig::Vector &curRestWeights);
    bool setRestWeights(const yarp::sig::Vector &newRestWeights, yarp::sig::Vector &curRestWeights);
    bool getLimits(const int axis, double *min, double *max);
    bool setLimits(const int axis, const double min, const double max);
    bool getTrajTime(double *t);
    bool setTrajTime(const double t);
    bool getInTargetTol(double *tol);
    bool setInTargetTol(const double tol);
    bool getJointsVelocities(yarp::sig::Vector &qdot);
    bool getTaskVelocities(yarp::sig::Vector &xdot, yarp::sig::Vector &odot);
    bool setTaskVelocities(const yarp::sig::Vector &xdot, const yarp::sig::Vector &odot);
    bool checkMotionDone(bool *f);
    bool waitMotionDone(const double period=0.1, const double timeout=0.0);
    bool stopControl();
    bool storeContext(int *id);
    bool restoreContext(const int id);
    bool getInfo(yarp::os::Bottle &info);
    bool registerEvent(const yarp::os::ConstString &type, yarp::dev::CartesianEvent *event);
    bool unregisterEvent(const yarp::os::ConstString &type);

    virtual ~ServerCartesianController();
};


#endif


