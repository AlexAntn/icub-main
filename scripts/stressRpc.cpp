#include <stdio.h>

#include <yarp/os/Network.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/os/Property.h>
#include <yarp/dev/ControlBoardInterfaces.h>
#include <yarp/os/Time.h>
#include <yarp/os/Random.h>

#include <yarp/sig/Vector.h>

using namespace yarp::dev;
using namespace yarp::os;
using namespace yarp::sig;

#include <string>
#include <sstream>

using namespace std;

int main(int argc, char **argv)
{
    Network yarp;

    printf("Going to stress rpc connections to the robot\n");

    Property parameters;
    parameters.fromCommand(argc, argv);

    ConstString part=parameters.find("part").asString();

    PolyDriver dd;
    Property p;

    Random::seed((unsigned int)Time::now());
    int random=(int) Random::uniform(0, 32768);
    
    string remote="/icub/";
    remote+=part;
    string local="/icub/stress/";

    stringstream lStream; 
    lStream << random;
    local += lStream.str();

    p.put("device", "remote_controlboard");
    p.put("local", local.c_str());
    p.put("remote", remote.c_str());
    p.put("carrier", "udp");
    dd.open(p);

    IEncoders *ienc;
    IPositionControl *ipos;
    IAmplifierControl *iamp;
    IPidControl *ipid;
    IControlLimits *ilim;
    IControlCalibration2 *ical;

    dd.view(ienc);
    dd.view(ipos);
    dd.view(iamp);
    dd.view(ipid);
    dd.view(ilim);
    dd.view(ical);
    
    int c=100;
    int nj;
    Vector encoders;
    ienc->getAxes(&nj);
    encoders.resize(nj);

    int count=0;
    while(true)
        {
            count++;
            double v;
            int jj=0;
            
            for(jj=0; jj<nj; jj++)
                {
                    ienc->getEncoder(jj, encoders.data()+jj);
            //            iamp->enableAmp(jj);
            //            ilim->setLimits(jj, 0, 0);
            //            double max;
            //            double min;
            //            ilim->getLimits(jj, &min, &max);
                    Pid pid;
                    ipid->getPid(jj, &pid);
                }

            fprintf(stderr, "%u\n", count);

            Time::delay(0.1);
        }
    
    return 0;
}
