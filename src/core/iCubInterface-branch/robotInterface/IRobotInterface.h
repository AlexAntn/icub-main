// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

/*
 * Copyright (C) 2008 RobotCub Consortium
 * Author: Lorenzo Natale
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
 *
 */

#ifndef __IROBOTINTERFACE__
#define __IROBOTINTERFACE__

#include <yarp/os/Property.h>
#include <yarp/sig/Vector.h>
#include <string>
#include "FeatureInterface.h"
#include "FeatureInterface_hid.h"

using namespace yarp::sig;

class IRobotInterface
{
public:
    virtual ~IRobotInterface(){};
    virtual bool initialize(const std::string &file)=0;
    virtual bool initCart(const std::string &file)
        {return true;}
    virtual bool finiCart()
        {return true;}


    /**
    * Closes all robot devices.
    */
    virtual bool detachWrappers()=0;
    virtual bool closeNetworks()=0;

    /**
    * Park the robot. This function can be blocking or not depending on 
    * the value of the parameter wait.
    * @param wait if true the function blocks and returns only when parking is finished
    */
    virtual void park(bool wait=true)=0;

    virtual void abort()=0;

    // _AC_
    virtual IiCubFeatureList *getRobotFeatureList(FEAT_ID *id)=0;
    virtual IiCubFeatureList *getRobotSkinList(FEAT_ID *id)=0;
    virtual IRobotInterface *getRobot()=0;
};




#endif
