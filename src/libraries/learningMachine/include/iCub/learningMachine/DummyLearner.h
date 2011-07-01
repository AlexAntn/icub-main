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


#ifndef LM_DUMMYLEARNER__
#define LM_DUMMYLEARNER__

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <yarp/os/Bottle.h>

#include "iCub/learningMachine/IFixedSizeLearner.h"


namespace iCub {
namespace learningmachine {

std::string printVector(const Vector& v);
/**
 * \ingroup icub_libLM_learning_machines
 *
 * This dummy machine learner demonstrates how the IMachineLearner interface can
 * be used in practice. The functionality, however, is rather useless.
 *
 * \see iCub::learningmachine::IMachineLearner
 *
 * \author Arjan Gijsberts
 *
 */
class DummyLearner : public IFixedSizeLearner {
private:
    /**
     * Number of samples during last training routine.
     */
    int sampleCount;

    /**
     * Number of training routines performed in total.
     */
    int trainCount;

    /**
     * The stored inputs.
     */
    std::vector<Vector> inputs;

    /**
     * The stored outputs.
     */
    std::vector<Vector> outputs;


public:
    /**
     * Constructor.
     */
    DummyLearner(unsigned int dom = 1, unsigned int cod = 1) : sampleCount(0), trainCount(0) {
        this->setName("Dummy");
        inputs.resize(0);
        outputs.resize(0);
        this->setDomainSize(dom);
        this->setCoDomainSize(cod);
    }

    /**
     * Destructor (empty).
     */
    virtual ~DummyLearner() { }

    /*
     * Inherited from IMachineLearner.
     */
    virtual void feedSample(const Vector& input, const Vector& output) {
        // call parent method to let it do some validation and processing with the transformers for us
        this->IFixedSizeLearner::feedSample(input, output);

        std::cout << "Received a sample: " << printVector(input) << " => " << printVector(output) << std::endl;

        this->inputs.push_back(input);
        this->outputs.push_back(input);
    }

    /*
     * Inherited from IMachineLearner.
     */
    virtual void train() {
        this->sampleCount = this->inputs.size();
        this->trainCount++;
    }

    /*
     * Inherited from IMachineLearner.
     */
    Vector predict(const Vector& input) {
        this->checkDomainSize(input);
        std::cout << "Received a prediction sample: " << printVector(input) << " => ";
        Vector output = input;
        output.resize(this->getCoDomainSize());
        for(int i = 0; i < output.size(); i++)
            output[i] += this->sampleCount;
        std::cout << "(" << printVector(output) << ")" << std::endl;
        return output;
    }

    /*
     * Inherited from IMachineLearner.
     */
    void reset() {
        this->sampleCount = 0;
        this->trainCount = 0;
        this->inputs.clear();
        this->outputs.clear();
    }

    /*
     * Inherited from IMachineLearner.
     */
    DummyLearner* clone() {
        return new DummyLearner(*this);
    }

    /*
     * Inherited from IMachineLearner.
     */
    std::string getInfo() {
        std::ostringstream buffer;
        buffer << this->IFixedSizeLearner::getInfo();
        buffer << "Training Samples: " << this->sampleCount << std::endl;
        buffer << "Training Iterations: " << this->trainCount << std::endl;
        buffer << "Collected Samples: " << this->inputs.size() << std::endl;
        return buffer.str();
    }

    /*
     * Inherited from IMachineLearner.
     */
    virtual void writeBottle(Bottle& bot) {
        // make sure to call the superclass's method
        bot.addInt(this->sampleCount);
        bot.addInt(this->trainCount);
        this->IFixedSizeLearner::writeBottle(bot);
    }

    /*
     * Inherited from IMachineLearner.
     */
    virtual void readBottle(Bottle& bot) {
        // make sure to call the superclass's method
        this->IFixedSizeLearner::readBottle(bot);
        this->trainCount = bot.pop().asInt();
        this->sampleCount = bot.pop().asInt();
    }

};

} // learningmachine
} // iCub
#endif
