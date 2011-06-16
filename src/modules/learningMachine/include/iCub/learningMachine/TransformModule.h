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

#ifndef LM_TRANSFORMMODULE__
#define LM_TRANSFORMMODULE__

#include <yarp/os/PortablePair.h>

#include "iCub/learningMachine/IMachineLearnerModule.h"
#include "iCub/learningMachine/TransformerPortable.h"

using namespace yarp::os;

namespace iCub {
namespace learningmachine {

/**
 * Generic abstract class for transformer based processors.
 *
 * \see iCub::learningmachine::PredictProcessor
 * \see iCub::learningmachine::TrainProcessor
 *
 * \author Arjan Gijsberts
 *
 */
class ITransformProcessor {
protected:
    /**
     * A reference to a portable transformer.
     */
    TransformerPortable& transformerPortable;

public:
    /**
     * Constructor.
     *
     * @param tp a reference to a transformer.
     */
    ITransformProcessor(TransformerPortable& tp) : transformerPortable(tp) { }

    /**
     * Retrieve the transformer portable wrapper.
     *
     * @return a reference to the transformer portable
     */
    virtual TransformerPortable& getTransformerPortable() {
        return this->transformerPortable;
    }

    /**
     * Retrieve the wrapped transformer.
     *
     * @return a reference to the wrapped transformer.
     */
    virtual ITransformer& getTransformer() {
        return this->getTransformerPortable().getWrapped();
    }
};

/**
 * Reply processor helper class for predictions. This processor receives
 * requests for predictions, transforms the samples and relays the request to
 * the associated port. This architecture allows multiple transformers to be
 * daisy chained, as long as the chain is terminated by a PredictModule.
 *
 * \see iCub::learningmachine::PredictModule
 * \see iCub::learningmachine::ITransformProcessor
 *
 * \author Arjan Gijsberts
 *
 */
class TransformPredictProcessor : public ITransformProcessor, public PortReader {
protected:
    /**
     * The relay port.
     */
    Port& predictRelay_inout;

public:
    /**
     * Constructor.
     *
     * @param tp a reference to a transformer.
     */
    TransformPredictProcessor(TransformerPortable& tp, Port& p)
      : ITransformProcessor(tp), predictRelay_inout(p) { }

    /*
     * Inherited from PortReader.
     */
    virtual bool read(ConnectionReader& connection);

    /**
     * Accessor for the prediction output port.
     *
     * @return a reference to the output port.
     */
    virtual Port& getOutputPort() {
        return this->predictRelay_inout;
    }

};


/**
 * Port processor helper class for incoming training samples.
 *
 * \see iCub::learningmachine::TrainModule
 * \see iCub::learningmachine::IMachineProcessor
 *
 * \author Arjan Gijsberts
 *
 */
class TransformTrainProcessor
  : public ITransformProcessor, public TypedReaderCallback< PortablePair<Vector,Vector> > {
private:
    /**
     * The relay port.
     */
    BufferedPort<PortablePair<Vector,Vector> >& train_out;

public:
    /**
     * Constructor.
     *
     * @param tp a reference to a transformer.
     */
    TransformTrainProcessor(TransformerPortable& tp,
                            BufferedPort<PortablePair<Vector,Vector> >& p)
      : ITransformProcessor(tp), train_out(p) { }

    /*
     * Inherited from TypedReaderCallback.
     */
    virtual void onRead(PortablePair<Vector,Vector>& input);

    /**
     * Retrieve the training output port.
     *
     * @return a reference to the output port.
     */
    virtual BufferedPort<PortablePair<Vector,Vector> >& getOutputPort() {
        return this->train_out;
    }

};




/**
 * \ingroup icub_libLM_modules
 *
 * A module for transforming vectors. This most common use of this module will
 * be data preprocessing (e.g. standardization), although it also supports much
 * more advanced transformations of the data.
 *
 * \author Arjan Gijsberts
 *
 */

class TransformModule : public IMachineLearnerModule {
private:
    /**
     * A portable wrapper around a transformer.
     */
    TransformerPortable transformerPortable;

    /**
     * Buffered port for the incoming training samples (input and output).
     */
    BufferedPort<PortablePair<Vector,Vector> > train_in;

    /**
     * Buffered port for the outgoing training samples (input and output).
     */
    BufferedPort<PortablePair<Vector,Vector> > train_out;

    /**
     * Buffered port for the incoming prediction samples.
     */
    BufferedPort<Vector> predict_inout;

    /**
     * Buffered port for the outgoing prediction samples.
     */
    Port predictRelay_inout;

    /**
     * The processor handling incoming training samples.
     */
    TransformTrainProcessor trainProcessor;

    /**
     * The processor handling prediction requests.
     */
    TransformPredictProcessor predictProcessor;

    /**
     * Copy constructor (private and unimplemented on purpose).
     */
    TransformModule(const TransformModule& other);

    /**
     * Assignment operator (private and unimplemented on purpose).
     */
    TransformModule& operator=(const TransformModule& other);

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
    TransformModule(std::string pp = "/lm/transform")
      : IMachineLearnerModule(pp), transformerPortable((ITransformer*) 0),
        trainProcessor(transformerPortable, train_out),
        predictProcessor(transformerPortable, predictRelay_inout) {
    }

    /**
     * Destructor (empty).
     */
    virtual ~TransformModule() { }

    /*
     * Inherited from IMachineLearnerModule.
     */
    virtual bool configure(ResourceFinder& opt);

    /*
     * Inherited from IMachineLearnerModule.
     */
    virtual bool interruptModule();

    /*
     * Inherited from IMachineLearnerModule.
     */
    virtual bool respond(const Bottle& cmd, Bottle& reply);

    /**
     * Retrieve the transformer that is used in this TransformModule.
     *
     * @return a reference to the transformer
     */
    virtual ITransformer& getTransformer() {
        return this->getTransformerPortable().getWrapped();
    }

    /**
     * Retrieve the transformer portable.
     *
     * @return a reference to the transformer portable
     */
    virtual TransformerPortable& getTransformerPortable() {
        return this->transformerPortable;
    }

};

} // learningmachine
} // iCub

#endif
