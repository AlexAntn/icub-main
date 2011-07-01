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

#include "iCub/learningMachine/DummyLearner.h"

namespace iCub {
namespace learningmachine {

// just here for debugging, since Vector.toString() cannot be applied to const Vector :(
std::string printVector(const Vector& v) {
  std::ostringstream output;
  output << "[";
  for(int i = 0; i < v.size(); i++) {
    if(i > 0) output << ",";
    output << v[i];
  }
  output << "]";
  return output.str();
}

} // learningmachine
} // iCub
