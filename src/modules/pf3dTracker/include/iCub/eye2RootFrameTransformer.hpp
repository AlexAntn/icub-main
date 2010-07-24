
#include <yarp/os/RFModule.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/ConstString.h>
#include <yarp/sig/Vector.h>
#include <yarp/sig/Matrix.h>

#include <iCub/iKin/iKinFwd.h>


class eye2RootFrameTransformer : public yarp::os::RFModule
{
protected:
    yarp::os::BufferedPort<yarp::os::Bottle> _inputHeadPort;
    yarp::os::BufferedPort<yarp::os::Bottle> _inputTorsoPort;
    yarp::os::BufferedPort<yarp::os::Bottle> _inputTargetPosPort;
    yarp::os::BufferedPort<yarp::os::Bottle> _outputTargetPosPort;
    
    bool receivedHead;
    bool receivedTorso;
    bool receivedTargetPos;
    bool isLeftEye;
    
	iCub::iKin::iCubEye   *eye;
    iCub::iKin::iKinChain  chainEye;
    
    yarp::sig::Vector v;
    yarp::sig::Matrix transformation;
    yarp::sig::Vector eyeTargetPos;
    yarp::sig::Vector rootTargetPos;
    
    double head0,head1,head2,head3,head4,head5;
    double torso0,torso1,torso2;
    double targetPosInX,targetPosInY,targetPosInZ,targetPosInGood;

public:  
    virtual bool configure(yarp::os::ResourceFinder &rf);
    virtual bool close();
    virtual bool updateModule();
    virtual double getPeriod();
};


