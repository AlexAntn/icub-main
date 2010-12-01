/* 
 * Copyright (C) 2010 RobotCub Consortium, European Commission FP6 Project IST-004370
 * Authors: Carlo Ciliberto, Ugo Pattacini
 * email:   carlo.ciliberto@iit.it, ugo.pattacini@iit.it
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
@ingroup icub_module

\defgroup indepMotionDetector indepMotionDetector
 
Detects independent moving points of a grid used to sample the 
input images. The algorithm works also with moving cameras.

Copyright (C) 2010 RobotCub Consortium
 
Authors: Carlo Ciliberto and Ugo Pattacini 
 
Date: first release on the night of 03/05/2010 :)

CopyPolicy: Released under the terms of the GNU GPL v2.0.

\section intro_sec Description
The module exploits the pyramidal Lucas-Kanade algorithm to
detect independent moving points over a selectable grid of 
nodes. The algorithm is designed in such a way that it works 
also with moving cameras. 
 
\note If you're going to use this detector for your work, please
      quote it within any resulting publication.
 
\section lib_sec Libraries 
YARP libraries and OpenCV

\section parameters_sec Parameters
--name \e stemName 
- The parameter \e stemName specifies the stem name of ports 
  created by the module.
 
--coverXratio \e ratioX
- The parameter \e ratioX identifies the portion of the x-axis 
  of the image covered by the grid nodes. Example: if
  ratioX=0.75, then the central 3/4 of the x-axis will be
  covered with points.
 
--coverYratio \e ratioY
- The analogous for the y-axis image.
 
--nodesStep \e step
- The parameter \e step selects the step in pixel between two 
  consecutive grid nodes.
 
--winSize \e size
- The parameter \e size selects window size used by the 
  algorithm.
 
--recogThres \e thres
- The parameter \e thres, given in percentage, specifies the 
  error threshold that allows to discriminate between background
  and independent moving nodes as result of a matching carried
  out on the windows whose size is determined by \e winSize
  parameter. Usually very small values, such as 0.002%, have to
  be used.
 
--adjNodesThres \e min 
- This parameter allows to filter out the salt-and-pepper noise
  over the output image, by specifying the minimum number of
  adjacent nodes that must be active (i.e. that undergo the
  motion) in the neighbourhood of any single node to keep it
  active.
 
--blobMinSizeThres \e min 
- This parameter allows to filter out blobs whose nodes number 
  is lower than <min>.
 
--framesPersistence \e frames
- This parameter allows to increase the node persistence over 
  consecutive frames implementing a sort of low-pass filter. The
  value \e frames specifies the number of consecutive frames for
  which if a node gets active it is kept on.
 
--numThreads \e threads
- This parameter allows to control the maximum number of threads
  allocated by parallelized OpenCV functions (if supported). The
  default value is 0 meaning that a number of threads equal to
  the number of available cores will be used.
 
--verbosity 
- Enable the dump of log messages.
 
\section portsa_sec Ports Accessed
None.

\section portsc_sec Ports Created
- <i> /<stemName>/img:i </i> accepts the incoming images. 
 
- <i> /<stemName>/img:o </i> outputs the input images with the 
  grid layer on top. This port propagates the time-stamp carried
  by the input image.
 
- <i> /<stemName>/nodes:o </i> outputs the x-y location of the 
  currently active nodes in this format: (nodesStep <val>)
  (<n0.x> <n0.y>) (<n1.x> <n1.y>) ... . This port propagates the
  time-stamp carried by the input image.
 
- <i> /<stemName>/blobs:o </i> outputs the x-y location of blobs
  centroids along with their size in this format: (<b0.cx>
  <b0.cy> <b0.size>) (<b1.cx> <b1.cy> <b1.size>) ... The output
  blobs list is sorted according to their size (decreasing
  order). This port propagates the time-stamp carried
  by the input image.
 
- <i> /<stemName>/opt:o </i> outputs monochrome images 
  containing just the grid nodes signalling independent
  movements. This port propagates the time-stamp carried
  by the input image.
 
- <i> /<stemName>/rpc </i> for RPC communication. 
 
\section rpcProto_sec RPC protocol 
The parameters <i> winSize, recogThres, adjNodesThres, 
framesPersistence, numThreads, verbosity </i> can be changed/retrieved 
through the commands set/get. Moreover the further switch \e 
inhibition can be accessed in order to enable/disable the motion 
detection at run-time. 
 
\section in_files_sec Input Data Files
None.

\section out_data_sec Output Data Files
None. 
 
\section conf_file_sec Configuration Files
None. 
 
\section tested_os_sec Tested OS
Linux and Windows.

\author Carlo Ciliberto and Ugo Pattacini
*/ 

#include <yarp/os/Network.h>
#include <yarp/os/RFModule.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Thread.h>
#include <yarp/os/Time.h>
#include <yarp/os/Stamp.h>
#include <yarp/sig/Image.h>

#include <cv.h>
#include <highgui.h>

#include <stdio.h>
#include <string>
#include <set>
#include <deque>

// check if OpenCV supports multi-threading
#if CV_MAJOR_VERSION > 0
    #define _INDEP_MULTI_THREADING_
#endif

// in BGR format
#define NODE_OFF    cvScalar(0,0,255)
#define NODE_ON     cvScalar(0,255,0)

using namespace std;
using namespace yarp;
using namespace yarp::os;
using namespace yarp::sig;


/************************************************************************/
class Blob
{
public:
    CvPoint centroid;
    int     size;

    /************************************************************************/
    Blob()
    {
        centroid.x=0;
        centroid.y=0;
        size=0;
    }
};


/************************************************************************/
class ProcessThread : public Thread
{
protected:
    ResourceFinder &rf;

    string name;
    double coverXratio;
    double coverYratio;
    int nodesStep;
    int winSize;
    double recogThres;
    double recogThresAbs;
    int adjNodesThres;
    int blobMinSizeThres;
    int framesPersistence;    
    bool verbosity;
    bool inhibition;
    int nodesNum;
    int nodesX;
    int nodesY;

#ifdef _INDEP_MULTI_THREADING_
    int numThreads;
#endif

    ImageOf<PixelMono>  imgMonoIn;
    ImageOf<PixelMono>  imgMonoPrev;
    ImageOf<PixelFloat> imgPyrPrev;
    ImageOf<PixelFloat> imgPyrCurr;

    CvPoint2D32f        *nodesPrev;
    CvPoint2D32f        *nodesCurr;
    int                 *nodesPersistence;
    char                *featuresFound;
    float               *featureErrors;

    set<int>             activeNodesIndexSet;
    deque<Blob>          blobSortedList;

    BufferedPort<ImageOf<PixelBgr> >  inPort;
    BufferedPort<ImageOf<PixelBgr> >  outPort;
    BufferedPort<ImageOf<PixelMono> > optPort;
    Port nodesPort;
    Port blobsPort;

    /************************************************************************/
    void disposeMem()
    {
        if (nodesPrev)
            delete nodesPrev;

        if (nodesCurr)
            delete nodesCurr;

        if (nodesPersistence)
            delete nodesPersistence;

        if (featuresFound)
            delete featuresFound;

        if (featureErrors)
            delete featureErrors;
    }

public:
    /************************************************************************/
    ProcessThread(ResourceFinder &_rf) : rf(_rf) { }

    /************************************************************************/
    virtual bool threadInit()
    {
        name=rf.check("name",Value("indepMotionDetector")).asString().c_str();
        coverXratio=rf.check("coverXratio",Value(0.75)).asDouble();
        coverYratio=rf.check("coverYratio",Value(0.75)).asDouble();
        nodesStep=rf.check("nodesStep",Value(6)).asInt();
        winSize=rf.check("winSize",Value(15)).asInt();
        recogThres=rf.check("recogThres",Value(0.002)).asDouble();
        adjNodesThres=rf.check("adjNodesThres",Value(4)).asInt();
        blobMinSizeThres=rf.check("blobMinSizeThres",Value(10)).asInt();
        framesPersistence=rf.check("framesPersistence",Value(3)).asInt();
        verbosity=rf.check("verbosity");
        inhibition=false;

        recogThresAbs=recogThres*((256*256*winSize*winSize)/100.0);

        // thresholding 
        if (coverXratio>1.0)
            coverXratio=1.0;
        if (coverYratio>1.0)
            coverYratio=1.0;

        // if the OpenCV version supports multi-threading,
        // set the maximum number of threads available to OpenCV
    #ifdef _INDEP_MULTI_THREADING_
        numThreads=rf.check("numThreads",Value(0)).asInt();
        cvSetNumThreads(numThreads);
        numThreads=cvGetNumThreads();
    #endif

        nodesPrev=NULL;
        nodesCurr=NULL;
        nodesPersistence=NULL;
        featuresFound=NULL;
        featureErrors=NULL;
        
        inPort.open(("/"+name+"/img:i").c_str());
        outPort.open(("/"+name+"/img:o").c_str());
        optPort.open(("/"+name+"/opt:o").c_str());
        nodesPort.open(("/"+name+"/nodes:o").c_str());
        blobsPort.open(("/"+name+"/blobs:o").c_str());

        return true;
    }

    /************************************************************************/
    void afterStart(bool s)
    {
        if (s)
        {
            fprintf(stdout,"Process started successfully\n");
            fprintf(stdout,"\n");
            fprintf(stdout,"Using ...\n");
            fprintf(stdout,"name              = %s\n",name.c_str());
            fprintf(stdout,"coverXratio       = %g\n",coverXratio);
            fprintf(stdout,"coverYratio       = %g\n",coverYratio);
            fprintf(stdout,"nodesStep         = %d\n",nodesStep);
            fprintf(stdout,"winSize           = %d\n",winSize);
            fprintf(stdout,"recogThres        = %g\n",recogThres);
            fprintf(stdout,"recogThresAbs     = %g\n",recogThresAbs);
            fprintf(stdout,"adjNodesThres     = %d\n",adjNodesThres);
            fprintf(stdout,"blobMinSizeThres  = %d\n",blobMinSizeThres);
            fprintf(stdout,"framesPersistence = %d\n",framesPersistence);
            
        #ifdef _INDEP_MULTI_THREADING_
            fprintf(stdout,"numThreads        = %d\n",numThreads);
        #else
            fprintf(stdout,"numThreads        = OpenCV version does not support multi-threading");
        #endif
            
            fprintf(stdout,"verbosity         = %s\n",verbosity?"on":"off");
            fprintf(stdout,"\n");
        }
        else
            fprintf(stdout,"Process did not start\n");
    }

    /************************************************************************/
    virtual void run()
    {
        double latch_t, dt0, dt1, dt2;

        while (!isStopping())
        {
            // acquire new image
            ImageOf<PixelBgr> *pImgBgrIn=inPort.read(true);

            // get the envelope from the image
            Stamp stamp;
            inPort.getEnvelope(stamp);

            if (isStopping() || pImgBgrIn==NULL)
                break;

            double t0=Time::now();
             
            // consistency check
            if (pImgBgrIn->width()!=imgMonoIn.width() ||
                pImgBgrIn->height()!=imgMonoIn.height())
            {    
                imgMonoIn.resize(*pImgBgrIn);
                imgMonoPrev.resize(*pImgBgrIn);

                imgPyrPrev.resize(pImgBgrIn->width()+8,pImgBgrIn->height()/3);
                imgPyrCurr.resize(pImgBgrIn->width()+8,pImgBgrIn->height()/3);

                // dispose previously allocated memory
                disposeMem();
                
                int min_x=(int)(((1.0-coverXratio)/2.0)*imgMonoIn.width());
                int min_y=(int)(((1.0-coverYratio)/2.0)*imgMonoIn.height());

                nodesX=(imgMonoIn.width()-2*min_x)/nodesStep;
                nodesY=(imgMonoIn.height()-2*min_y)/nodesStep;

                nodesNum=nodesX*nodesY;

                nodesPrev=new CvPoint2D32f[nodesNum];
                nodesCurr=new CvPoint2D32f[nodesNum];
                nodesPersistence=new int[nodesNum];

                featuresFound=new char[nodesNum];
                featureErrors=new float[nodesNum];

                memset(nodesPersistence,0,nodesNum*sizeof(int));
                
                // populate grid
                int cnt=0;
                for (int y=min_y; y<=(imgMonoIn.height()-min_y); y+=nodesStep)
                    for (int x=min_x; x<=(imgMonoIn.width()-min_x); x+=nodesStep)
                        nodesPrev[cnt++]=cvPoint2D32f(x,y);

                // convert to gray-scale
                cvCvtColor(pImgBgrIn->getIplImage(),imgMonoPrev.getIplImage(),CV_BGR2GRAY);

                if (verbosity)
                {
                    // log message
                    fprintf(stdout,"Detected image of size %dx%d;\nusing %dx%d=%d nodes;\npopulated %d nodes\n",
                            imgMonoIn.width(),imgMonoIn.height(),nodesX,nodesY,nodesNum,cnt);
                }

                // skip to the next cycle
                continue;
            }

            // convert the input image to gray-scale
            cvCvtColor(pImgBgrIn->getIplImage(),imgMonoIn.getIplImage(),CV_BGR2GRAY);

            // copy input image into output image
            ImageOf<PixelBgr> &imgBgrOut=outPort.prepare();
            imgBgrOut=*pImgBgrIn;

            // get optFlow image
            ImageOf<PixelMono> &imgMonoOpt=optPort.prepare();
            imgMonoOpt.resize(imgBgrOut);
            imgMonoOpt.zero();

            // declare output bottles
            Bottle nodesBottle;
            Bottle blobsBottle;

            Bottle &nodesStepBottle=nodesBottle.addList();
            nodesStepBottle.addString("nodesStep");
            nodesStepBottle.addInt(nodesStep);

            // purge the content of variables
            activeNodesIndexSet.clear();
            blobSortedList.clear();

            // compute optical flow
            latch_t=Time::now();
            cvCalcOpticalFlowPyrLK(imgMonoPrev.getIplImage(),imgMonoIn.getIplImage(),
                                   imgPyrPrev.getIplImage(),imgPyrCurr.getIplImage(),
                                   nodesPrev,nodesCurr,nodesNum,
                                   cvSize(winSize,winSize),5,featuresFound,featureErrors,
                                   cvTermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS,20,0.3),0);
            dt0=Time::now()-latch_t;

            // assign status to the grid nodes
            latch_t=Time::now();
            for (int i=0; i<nodesNum; i++)
            {
                bool persistentNode=false;

                CvPoint node=cvPoint((int)nodesPrev[i].x,(int)nodesPrev[i].y);
                
                // handle the node persistence
                if (!inhibition && nodesPersistence[i])
                {
                    cvCircle(imgBgrOut.getIplImage(),node,1,NODE_ON,2);
                    cvCircle(imgMonoOpt.getIplImage(),node,1,cvScalar(255),2);

                    Bottle &nodeBottle=nodesBottle.addList();
                    nodeBottle.addInt((int)nodesPrev[i].x);
                    nodeBottle.addInt((int)nodesPrev[i].y);

                    // update the active nodes set
                    activeNodesIndexSet.insert(i);

                    nodesPersistence[i]--;

                    persistentNode=true;
                }
                else
                    cvCircle(imgBgrOut.getIplImage(),node,1,NODE_OFF,1);

                // do not consider the border nodes and skip if inhibition is on
                int row=i%nodesX;
                bool skip=inhibition || (i<nodesX) || (i>=(nodesNum-nodesX)) || (row==0) || (row==(nodesX-1));

                if (!skip && featuresFound[i] && (featureErrors[i]>recogThresAbs))
                {
                    // count the neighbour nodes that are ON
                    // start from -1 to avoid counting the current node
                    int cntAdjNodesOn=-1;

                    // scroll per lines
                    for (int j=i-nodesX; j<=(i+nodesX); j+=nodesX)
                        for (int k=j-1; k<=(j+1); k++)
                            cntAdjNodesOn+=(int)(featuresFound[k]&&(featureErrors[k]>recogThresAbs));

                    // highlight independent moving node if over threhold
                    if (cntAdjNodesOn>=adjNodesThres)
                    {
                        // init the node persistence timeout
                        nodesPersistence[i]=framesPersistence;

                        // update only if the node was not persistent
                        if (!persistentNode)
                        {
                            cvCircle(imgBgrOut.getIplImage(),node,1,NODE_ON,2);
                            cvCircle(imgMonoOpt.getIplImage(),node,1,cvScalar(255),2);

                            Bottle &nodeBottle=nodesBottle.addList();
                            nodeBottle.addInt((int)nodesPrev[i].x);
                            nodeBottle.addInt((int)nodesPrev[i].y);

                            // update the active nodes set
                            activeNodesIndexSet.insert(i);
                        }
                    }
                }
            }
            dt1=Time::now()-latch_t;

            latch_t=Time::now();
            findBlobs();

            // prepare the blobs output list and draw their
            // centroids location
            for (int i=0; i<(int)blobSortedList.size(); i++)
            {
                Blob &blob=blobSortedList[i];
                int blueLev=255-((100*i)%255);
                int redLev=(100*i)%255;

                CvPoint centroid=cvPoint(blob.centroid.x,blob.centroid.y);

                Bottle &blobBottle=blobsBottle.addList();
                blobBottle.addInt(centroid.x);
                blobBottle.addInt(centroid.y);
                blobBottle.addInt(blob.size);

                cvCircle(imgBgrOut.getIplImage(),centroid,4,cvScalar(blueLev,0,redLev),3);
            }
            dt2=Time::now()-latch_t;

            // send out images, propagating the time-stamp
            outPort.setEnvelope(stamp);
            outPort.write();

            optPort.setEnvelope(stamp);
            optPort.write();

            // send out data bottles, propagating the time-stamp
            if (nodesBottle.size()>1)
            {
                nodesPort.setEnvelope(stamp);
                nodesPort.write(nodesBottle);
            }

            if (blobsBottle.size())
            {
                blobsPort.setEnvelope(stamp);
                blobsPort.write(blobsBottle);
            }

            // save data for next cycle
            imgMonoPrev=imgMonoIn;
            
            double t1=Time::now();
            if (verbosity)
            {
                // dump statistics
                fprintf(stdout,"cycle timing [ms]: optflow(%g), colorgrid(%g), blobdetection(%g), overall(%g)\n",
                        1000.0*dt0,1000.0*dt1,1000.0*dt2,1000.0*(t1-t0));
            }
        }
    }

    /************************************************************************/
    virtual void onStop()
    {
        inPort.interrupt();
        outPort.interrupt();
        optPort.interrupt();
        nodesPort.interrupt();
        blobsPort.interrupt();
    }

    /************************************************************************/
    virtual void threadRelease()
    {
        disposeMem();

        inPort.close();
        outPort.close();
        optPort.close();
        nodesPort.close();
        blobsPort.close();
    }

    /************************************************************************/
    string getName()
    {
        return name;
    }

    /************************************************************************/
    void findBlobs()
    {
        // iterate until the set is empty
        while (activeNodesIndexSet.size())
        {
            Blob blob;

            // the nodes connected to the current one
            // will be removed from the list            
            floodFill(*(activeNodesIndexSet.begin()),&blob);

            // update centroid
            blob.centroid.x/=blob.size;
            blob.centroid.y/=blob.size;

            // insert iff the blob is big enough
            if (blob.size>blobMinSizeThres)
                insertBlob(blob);
        }
    }

    /************************************************************************/
    void floodFill(const int i, Blob *pBlob)
    {
        set<int>::iterator el=activeNodesIndexSet.find(i);
        if (el!=activeNodesIndexSet.end())
        {
            // update blob
            pBlob->centroid.x+=(int)nodesPrev[i].x;
            pBlob->centroid.y+=(int)nodesPrev[i].y;
            pBlob->size++;

            // remove element from the set            
            activeNodesIndexSet.erase(el);

            // perform recursive exploration
            for (int j=i-nodesX; j<=(i+nodesX); j+=nodesX)
                for (int k=j-1; k<=(j+1); k++)
                    if (k!=i)
                        floodFill(k,pBlob);
        }
    }

    /************************************************************************/
    void insertBlob(const Blob &blob)
    {
        // insert the blob keeping the decreasing order of the list wrt the size attribute
        for (deque<Blob>::iterator el=blobSortedList.begin(); el!=blobSortedList.end(); el++)
        {
            if (el->size<blob.size)
            {
                blobSortedList.insert(el,blob);
                return;
            }
        }

        // reaching this point means that 
        // we have to append the blob
        blobSortedList.push_back(blob);
    }

    /************************************************************************/
    bool execReq(const Bottle &req, Bottle &reply)
    {
        if (req.size())
        {
            string cmd=req.get(0).asString().c_str();

            if (cmd=="set")
            {
                if (req.size()<3)
                    return false;

                string subcmd=req.get(1).asString().c_str();

                if (subcmd=="winSize")
                {
                    winSize=req.get(2).asInt();
                    reply.addString("ack");
                }
                else if (subcmd=="recogThres")
                {
                    recogThres=req.get(2).asDouble();
                    recogThresAbs=recogThres*((256*256*winSize*winSize)/100.0);
                    reply.addString("ack");
                }
                else if (subcmd=="adjNodesThres")
                {
                    adjNodesThres=req.get(2).asInt();
                    reply.addString("ack");
                }
                else if (subcmd=="blobMinSizeThres")
                {
                    blobMinSizeThres=req.get(2).asInt();
                    reply.addString("ack");
                }
                else if (subcmd=="framesPersistence")
                {
                    framesPersistence=req.get(2).asInt();
                    reply.addString("ack");
                }
                else if (subcmd=="numThreads")
                {
                #ifdef _INDEP_MULTI_THREADING_
                    numThreads=req.get(2).asInt();
                    cvSetNumThreads(numThreads);
                    numThreads=cvGetNumThreads();
                    reply.addString("ack");
                #else
                    reply.addString("multi-threading not supported");
                #endif
                }
                else if (subcmd=="verbosity")
                {
                    verbosity=req.get(2).asString()=="on";
                    reply.addString("ack");
                }
                else if (subcmd=="inhibition")
                {
                    inhibition=req.get(2).asString()=="on";
                    reply.addString("ack");
                }
                else
                    return false;
            }
            else if (cmd=="get")
            {
                if (req.size()<2)
                    return false;

                string subcmd=req.get(1).asString().c_str();

                if (subcmd=="winSize")
                    reply.addInt(winSize);
                else if (subcmd=="recogThres")
                    reply.addDouble(recogThres);
                else if (subcmd=="adjNodesThres")
                    reply.addInt(adjNodesThres);
                else if (subcmd=="blobMinSizeThres")
                    reply.addInt(blobMinSizeThres);
                else if (subcmd=="framesPersistence")
                    reply.addInt(framesPersistence);
                else if (subcmd=="numThreads")
                #ifdef _INDEP_MULTI_THREADING_
                    reply.addInt(numThreads);
                #else
                    reply.addString("multi-threading not supported");
                #endif
                else if (subcmd=="verbosity")
                    reply.addString(verbosity?"on":"off");
                else if (subcmd=="inhibition")
                    reply.addString(inhibition?"on":"off");
                else
                    return false;
            }
            else
                return false;

            return true;
        }
        else
            return false;
    }
};


/************************************************************************/
class ProcessModule: public RFModule
{
private:
    ProcessThread *thr;
    Port           rpcPort;

public:
    /************************************************************************/
    ProcessModule() : thr(NULL) { }

    /************************************************************************/
    virtual bool configure(ResourceFinder &rf)
    {
        Time::turboBoost();

        thr=new ProcessThread(rf);
        if (!thr->start())
        {
            delete thr;    
            return false;
        }

        rpcPort.open(("/"+thr->getName()+"/rpc").c_str());
        attach(rpcPort);

        return true;
    }

    /************************************************************************/
    virtual bool respond(const Bottle &command, Bottle &reply)
    {
        if (thr->execReq(command,reply))
            return true;
        else
            return RFModule::respond(command,reply);
    }

    /************************************************************************/
    virtual bool close()
    {
        if (thr)
        {
            thr->stop();
            delete thr;
        }

        rpcPort.interrupt();
        rpcPort.close();

        return true;
    }

    /************************************************************************/
    virtual double getPeriod()
    {
        return 1.0;
    }

    /************************************************************************/
    virtual bool updateModule()
    {
        return true;
    }
};


/************************************************************************/
int main(int argc, char *argv[])
{
    ResourceFinder rf;
    rf.setVerbose(true);
    rf.configure("ICUB_ROOT",argc,argv);

    if (rf.check("help"))
    {
        fprintf(stdout,"Options:\n");
        fprintf(stdout,"\t--name              <string>\n");
        fprintf(stdout,"\t--coverXratio       <double>\n");
        fprintf(stdout,"\t--coverYratio       <double>\n");
        fprintf(stdout,"\t--nodesStep         <int>\n");
        fprintf(stdout,"\t--winSize           <int>\n");
        fprintf(stdout,"\t--recogThres        <double>\n");
        fprintf(stdout,"\t--adjNodesThres     <int>\n");
        fprintf(stdout,"\t--blobMinSizeThres  <int>\n");
        fprintf(stdout,"\t--framesPersistence <int>\n");
    #ifdef _INDEP_MULTI_THREADING_
        fprintf(stdout,"\t--numThreads        <int>\n");
    #endif
        fprintf(stdout,"\t--verbosity           -\n");
        
        return 0;
    }

    Network yarp;
    if (!yarp.checkNetwork())
        return -1;

    ProcessModule mod;

    return mod.runModule(rf);
}


