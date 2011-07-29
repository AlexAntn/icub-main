/*
 * Copyright (C) 2007-2010 RobotCub Consortium, European Commission FP6 Project IST-004370
 * author:  Arjan Gijsberts
 * email:   arjan.gijsberts@iit.it
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

#ifndef LM_IMACHINELEARNERMODULE__
#define LM_IMACHINELEARNERMODULE__

#include <yarp/os/RFModule.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/os/Bottle.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Time.h>

#include "iCub/learningMachine/DispatcherManager.h"


namespace iCub {
namespace learningmachine {

/**
 * \defgroup icub_libLM_modules Modules
 *
 * \ingroup icub_libLearningMachine
 *
 * Modules to train and predict with learning machines and to use preprocessors.
 *
 * \author Arjan Gijsberts
 */

/**
 * \ingroup icub_libLM_modules
 *
 * An abstract base module for the machine learning YARP interface. This
 * abstract class contains base functionality that is shared by
 * PredictModule, TrainModule and TransformModule.
 *
 * \see iCub::learningmachine::IMachineLearner
 * \see iCub::learningmachine::TrainModule
 * \see iCub::learningmachine::PredictModule
 * \see iCub::learningmachine::TransformerModule
 *
 * \author Arjan Gijsberts
 */

class IMachineLearnerModule : public yarp::os::RFModule {
protected:
    /**
     * An input port for commands.
     */
    yarp::os::Port cmd_in;

    /**
     * A prefix path for the ports that will be registered.
     */
    std::string portPrefix;

    /**
     * An instance of the DispatchManager to configure event listeners.
     */
    DispatcherManager dmanager;

    /**
     * A pointer to the ResourceFinder.
     */
    yarp::os::ResourceFinder* rf;

    /**
     * Copy Constructor (private and unimplemented on purpose).
     */
    IMachineLearnerModule(const IMachineLearnerModule& other);

    /**
     * Assignment operator (private and unimplemented on purpose).
     */
    IMachineLearnerModule& operator=(const IMachineLearnerModule& other);

    /**
     * Register a port with a given name.
     *
     * @param port the port object
     * @param name the name for the port
     *
     */
    void registerPort(yarp::os::Contactable& port, std::string name);

    /**
     * Registers all ports used by this module.
     */
    virtual void registerAllPorts() = 0;

    /**
     * Unregisters all ports used by this module.
     */
    virtual void unregisterAllPorts() = 0;

    /**
     * Prints the accepted command line options with an optional error message.
     */
    virtual void printOptions(std::string error = "") = 0;

    /**
     * Reads bottles from a file and sends these one by one to the respond method.
     *
     * @param fname  the filename
     * @param out  an optional Bottle to which the replies will be appended
     */
    virtual void loadCommandFile(std::string fname, yarp::os::Bottle* out = (yarp::os::Bottle*) 0);

    /**
     * Returns a reference to the cached ResourceFinder.
     *
     * @return a reference to a ResourceFinder
     * @exception std::runtime_error  if there is no cached ResourceFinder
     */
    virtual yarp::os::ResourceFinder& getResourceFinder() {
        if(this->rf == (yarp::os::ResourceFinder*) 0) {
            throw std::runtime_error("Attempt to retrieve inexistent resource finder.");
        }
        return *(this->rf);
    }

    /**
     * Finds the full path to a specified filename using the ResourceFinder.
     * This method is necessary to work around the limitation of ResourceFinder
     * to only work with registered keys instead of directly with filenames.
     *
     * @param fname  the filename
     * @return the full path
     * @exception std::runtime_error if the file cannot be located
     */
    std::string findFile(std::string fname);

    /**
     * Mutator for the locally stored ResourceFinder. The module does _not_ take
     * responsibility for the allocated memory.
     *
     * @param rf  a pointer to a ResourceFinder
     */
    virtual void setResourceFinder(yarp::os::ResourceFinder* rf) {
        this->rf = rf;
    }

public:
    /**
     * Constructor.
     *
     * @param pp the default prefix used for the ports.
     */
    IMachineLearnerModule(std::string pp) : portPrefix(pp), rf((yarp::os::ResourceFinder*) 0) { }

    /**
     * Destructor (empty).
     */
    virtual ~IMachineLearnerModule() { }

    /**
     * Close the module.
     *
     * @return true if the module was closed successfully
     */
    virtual bool close();

    /**
     * Default empty update loop with 1 second delay.
     */
    virtual bool updateModule() {
        yarp::os::Time::delay(1.);
        return true;
    }

};

} // learningmachine
} // iCub

#endif

