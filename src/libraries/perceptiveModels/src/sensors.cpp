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

#include <yarp/os/Bottle.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/sig/Vector.h>

#include <iCub/perception/sensors.h>

using namespace std;
using namespace yarp::os;
using namespace yarp::dev;
using namespace yarp::sig;
using namespace iCub::perception;


/************************************************************************/
Sensor::Sensor()
{
    name="";
    source=NULL;
    configured=false;    
}


/************************************************************************/
bool SensorInterface::configure(void *source, const Property &options)
{
    Property &opt=const_cast<Property&>(options);
    if ((source==NULL) || !opt.check("name") ||
        !opt.check("size") || !opt.check("index"))
        return false;

    this->source=source;
    name=opt.find("name").asString().c_str();
    size=opt.find("size").asInt();
    index=opt.find("index").asInt();

    return configured=true;
}


/************************************************************************/
bool SensorInterface::getInput(Value &in) const
{
    if (!configured)
        return false;    

    Vector vect(size);
    static_cast<IEncoders*>(source)->getEncoders(vect.data());
    in=Value(vect[index]);

    return true;
}


/************************************************************************/
bool SensorPort::configure(void *source, const Property &options)
{
    Property &opt=const_cast<Property&>(options);
    if ((source==NULL) || !opt.check("name") || !opt.check("index"))
        return false;

    this->source=source;
    name=opt.find("name").asString().c_str();
    index=opt.find("index").asInt();

    return configured=true;
}


/************************************************************************/
bool SensorPort::getInput(Value &in) const
{
    if (configured)
    {
        BufferedPort<Bottle> *pPort=static_cast<BufferedPort<Bottle>*>(source);

        if (Bottle *data=pPort->read(false))
            in=Value(data->get(index).asDouble());
        else if (Bottle *data=pPort->lastRead())
            in=Value(data->get(index).asDouble());

        return true;
    }
    else
        return false;
}




