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

#ifndef LM_PREDICTMODULE__
#define LM_PREDICTMODULE__

#include "iCub/learningMachine/IMachineLearnerModule.h"
#include "iCub/learningMachine/MachinePortable.h"

namespace iCub {
namespace learningmachine {

/**
 * Generic abstract class for machine based processors.
 *
 * \see iCub::learningmachine::PredictProcessor
 * \see iCub::learningmachine::TrainProcessor
 *
 * \author Arjan Gijsberts
 *
 */
class IMachineProcessor {
protected:
    /**
     * A pointer to a concrete wrapper around a learning machine.
     */
    MachinePortable& machinePortable;

public:
    /**
     * Constructor.
     *
     * @param mp a pointer to a machine portable.
     */
    IMachineProcessor(MachinePortable& mp) : machinePortable(mp) { }

    /**
     * Retrieve the machine portable machine wrapper.
     *
     * @return a reference to the machine portable
     */
    virtual MachinePortable& getMachinePortable() {
        return this->machinePortable;
    }

    /**
     * Convenience function to quickly retrieve the machine that is wrapped in
     * the portable machine wrapper.
     *
     * @return a pointer to the actual machine
     */
    virtual IMachineLearner& getMachine() {
        return this->getMachinePortable().getWrapped();
    }
};



/**
 * Reply processor helper class for predictions.
 *
 * \see iCub::learningmachine::PredictModule
 * \see iCub::learningmachine::IMachineProcessor
 *
 * \author Arjan Gijsberts
 *
 */
class PredictProcessor : public IMachineProcessor, public yarp::os::PortReader {
public:
    /**
     * Constructor.
     *
     * @param mp a reference to a machine portable.
     */
    PredictProcessor(MachinePortable& mp) : IMachineProcessor(mp) { }

    /*
     * Inherited from PortReader.
     */
    virtual bool read(yarp::os::ConnectionReader& connection);
};


/**
 * \ingroup icub_libLM_modules
 *
 * A module for predictions.
 * The module can contain any iCub::learningmachine::IMachineLearner.
 * This module can be used in a combined system with a TrainModule.
 *
 * \see iCub::learningmachine::IMachineLearner
 * \see iCub::learningmachine::IMachineLearnerModule
 * \see iCub::learningmachine::TrainModule
 *
 * \author Arjan Gijsberts, Francesco Orabona, Paul Fitzpatrick
 *
 */
class PredictModule : public IMachineLearnerModule {
protected:

    /**
     * Buffered port for the incoming samples and corresponding replies.
     */
    yarp::os::BufferedPort<yarp::sig::Vector> predict_inout;

    /**
     * A concrete wrapper around a learning machine.
     */
    MachinePortable machinePortable;

    /**
     * The processor handling prediction requests.
     */
    PredictProcessor predictProcessor;

    /**
     * Incoming port for the models from the train module.
     */
    yarp::os::Port model_in;

    /**
     * Copy constructor (private and unimplemented on purpose).
     */
    PredictModule(const PredictModule& other);

    /**
     * Assignment operator (private and unimplemented on purpose).
     */
    PredictModule& operator=(const PredictModule& other);

    /*
     * Inherited from IMachineLearnerModule.
     */
    void registerAllPorts();

    /*
     * Inherited from IMachineLearnerModule.
     */
    void unregisterAllPorts();

    /*
     * Inherited from IMachineLearnerModule.
     */
    void printOptions(std::string error = "");

public:
    /**
     * Constructor.
     *
     * @param pp the default prefix used for the ports.
     */
    PredictModule(std::string pp = "/lm/predict")
      : IMachineLearnerModule(pp), machinePortable((IMachineLearner*) 0),
        predictProcessor(machinePortable) { }

    /**
     * Destructor (empty).
     */
    virtual ~PredictModule() { }

    /*
     * Inherited from IMachineLearnerModule.
     */
    virtual bool configure(yarp::os::ResourceFinder& opt);

    /*
     * Inherited from IMachineLearnerModule.
     */
    virtual bool interruptModule();

    /*
     * Inherited from IMachineLearnerModule.
     */
    virtual bool respond(const yarp::os::Bottle& cmd, yarp::os::Bottle& reply);

    /*
     * Inherited from IMachineLearnerModule.
     */
    virtual bool close() {
        IMachineLearnerModule::close();
        if(this->getMachinePortable().hasWrapped()) {
            return this->getMachine().close();
        } else {
            return true;
        }
    }

    /**
     * Retrieve the machine that is wrapped in the portable machine wrapper.
     *
     * @return a reference to the actual machine
     */
    virtual IMachineLearner& getMachine() {
        return this->getMachinePortable().getWrapped();
    }

    /**
     * Retrieve the machine portable.
     *
     * @return a reference to the machine portable
     */
    virtual MachinePortable& getMachinePortable() {
        return this->machinePortable;
    }

};

} // learningmachine
} // iCub

#endif
