/* 
 * Copyright (C) 2011 RobotCub Consortium, European Commission FP6 Project IST-004370
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
 * @note UNDER DEVELOPMENT  
 * @defgroup PerceptiveModels perceptiveModels
 *  
 * @ingroup icub_libraries 
 *  
 * Abstract layers for dealing with perceptive models framework. 
 *  
 * @defgroup Sensors Sensors 
 *  
 * @ingroup PerceptiveModels
 *
 * @author Ugo Pattacini 
 *  
 * Copyright (C) 2011 RobotCub Consortium
 *
 * CopyPolicy: Released under the terms of the GNU GPL v2.0. 
 *
 * @section intro_sec Description
 *
 * ... 
 *  
 */ 

#ifndef __PERCEPTIVEMODELS_SENSORS_H__
#define __PERCEPTIVEMODELS_SENSORS_H__

#include <string>

#include <yarp/os/Value.h>
#include <yarp/os/Property.h>


namespace iCub
{

namespace perception
{

/** 
* @ingroup Sensors
*  
*/
class Sensor
{
protected:
    std::string name;
    bool configured;
    void *source;

public:
    Sensor();
    std::string getName() const { return name; }
    virtual bool configure(void *source, const yarp::os::Property &options) = 0;
    virtual bool getInput(yarp::os::Value &in) const = 0;
    virtual ~Sensor() { }
};


/**
* @ingroup Sensors
*  
*/
class SensorInterface : public Sensor
{
protected:
    int size;
    int index;

public:
    bool configure(void *source, const yarp::os::Property &options);
    bool getInput(yarp::os::Value &in) const;
};


/**
* @ingroup Sensors
*  
*/
class SensorPort : public Sensor
{
protected:
    int index;

public:
    bool configure(void *source, const yarp::os::Property &options);
    bool getInput(yarp::os::Value &in) const;
};


}

}

#endif


