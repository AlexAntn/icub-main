// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-
#ifndef _BLOBFINDERTHREAD_H_
#define _BLOBFINDERTHREAD_H_

//within project includes
#include <iCub/WatershedOperator.h>
#include <iCub/SalienceOperator.h>

//IPP include
#include <ippi.h>

//YARP include
#include <yarp/os/all.h>
#include <yarp/sig/all.h>

using namespace yarp::os;
using namespace yarp::sig;


class blobFinderThread : public Thread{
private:
    /**
    * port used for centroid position to controlGaze2
    */
    BufferedPort<Bottle> centroidPort;
    /**
    * port used for centroid position to iKinHead
    */
    Port triangulationPort;
    /**
    * port used for sending responses from triangulationPort back into i
    */
    BufferedPort<Bottle> gazeControlPort;
    /**
    * port where the input image is read from
    */
    BufferedPort<ImageOf<PixelRgb> > inputPort;
    /**
    * port that returns the image output
    */
    BufferedPort<ImageOf<PixelRgb> > outputPort;
    /**
    * port where the red plane of the image is streamed
    */
    BufferedPort<ImageOf<PixelMono> > redPort;
    /**
    * port where the green plane of the image is streamed
    */
    BufferedPort<ImageOf<PixelMono> > greenPort;
    /**
    * port where the blue plane of the image is streamed
    */
    BufferedPort<ImageOf<PixelMono> > bluePort;
    /**
    * port where the difference of gaussian R+G- is streamed
    */
    BufferedPort<ImageOf<PixelMono> > rgPort;
    /**
    * port where the difference of gaussian G+R- is streamed
    */
    BufferedPort<ImageOf<PixelMono> > grPort;
    /**
    * port where the difference of gaussian B+Y- of the image is streamed
    */
    BufferedPort<ImageOf<PixelMono> > byPort;
    /**
    * port where the yellow plane of the image is streamed
    */
    BufferedPort<ImageOf<PixelMono> > yellowPort;
    /**
    * image result of the function outContrastLP
    */
    ImageOf<PixelMono> *outContrastLP;
    /**
    * buffer image for received image
    */
    ImageOf<PixelMono> *tmpImage;
    /**
    * image result of the function meanColourLP;
    */
    ImageOf<PixelBgr> *outMeanColourLP;
    /**
    * ipp reference to the size of the input image
    */
    IppiSize srcsize;
    /**
    * width of the input image
    */
    int width;
    /**
    * height of the input image
    */
    int height;
    /**
    * name of the module and rootname of the connection
    */
    std::string name;
    /**
    * flag that indicates when the reinitiazation has already be done
    */
    bool reinit_flag;
    /*
    * flag that indicates when the thread has been interrupted
    */
    bool interrupted_flag;
    /**
     * semaphore for the respond function
     */
     Semaphore mutex;
     /**
    * execution step counter
    */
    int ct;
    /**
    * input image
    */
    ImageOf<PixelRgb> *img;
    /**
    * reference to the watershed operator
    */
    WatershedOperator *wOperator;
    /**
    * vector of boolean which tells whether there is a blob or not
    */
    char* blobList;
    /**
    * pointer to the 3 channels output image of the watershed algorithm
    */
    ImageOf<PixelRgb>* _outputImage3;
    /**
    * input image of the opponency R+G-
    */
    ImageOf<PixelMono> *_inputImgRGS;
    /**
    * input image of the opponency G+R-
    */
    ImageOf<PixelMono> *_inputImgGRS;
    /**
    * input image of the opponency B+Y-
    */
    ImageOf<PixelMono> *_inputImgBYS;
    /**
    * pointer to the image of tags
    */
    ImageOf<PixelInt> *ptr_tagged;    
    /**
    *vector of tags to the sequence of blobs
    */
    ImageOf<PixelInt>* tagged;
    /**
    * image of the fovea blob
    */
    ImageOf<PixelMono> *blobFov;
    /**
    * R+G- value for the search
    */
    int searchRG;
    /**
    * G+R- value for the search
    */
    int searchGR;
    /**
    * B+Y- value for the search
    */
    int searchBY;
    /**
    * flag that indicates if the images have been resized
    */
    bool resized_flag;
    

    //_________ private methods ____________
    /**
    * resizes all the needed images
    * @param width width of the input image
    * @param height height of the input image
    */
    void resizeImages(int width, int height);
    /**
    * function that extracts characteristics of all the blobs in the catalogue and save them
    * @param stable parameters that enable some lines of code for the stable version
    */
    void drawAllBlobs(bool stable);    
    
public:
    /**
    * default constructor
    */
    blobFinderThread();
    /**
    * destructor
    */
    ~blobFinderThread(){};
    /**
    *	initialization of the thread 
    */
    bool threadInit();
    /**
    * active loop of the thread
    */
    void run();
    /**
    *	releases the thread
    */
    void threadRelease();
    /**
    * function that reinitiases some attributes of the class
    * @param height height of the input image
    * @param width width of the input image
    */
    void reinitialise(int width,int height);
    /**
    * function called when the module is poked with an interrupt command
    */
    void interrupt();
    /**
    * function that gives reference to the name of the module
    * @param name of the module
    */
    void setName(std::string name);
    /**
    * function that returns the name of the module
    * @param str string to be added
    * @return name of the module
    */
    std::string getName(const char* str);
    /**
    * function the applies the watershed (rain falling) algorithm
    */
    void rain();
    /**
    * function that resets all the flags for the desired output
    */
    void resetFlags();
    /**
    * streams out data on ports
    * @return return whether the operation was successful
    */
    bool outPorts();
    /**
    * function that reads the ports for colour RGB opponency maps
    */
    bool getOpponencies();
    /**
    * function that reads the ports for the RGB planes
    */
    bool getPlanes();

    //_________ public attributes _______________
    /**
    * reference to the salience operator
    */
    SalienceOperator *salience;
    /**
    * pointer to the most salient blob
    */
    YARPBox* max_boxes;
    /**
    * flag that allows the thread to run since all the inputs are ready
    */
    bool freetorun;
    /**
    * image which is plotted in the drawing area
    */
    ImageOf<PixelRgb> *image_out; //
    /**
    * image which is plotted in the drawing area
    */
    ImageOf<PixelRgb> *image_out2;
     /**
    * pointer to the input image
    */
    ImageOf<yarp::sig::PixelRgb> *ptr_inputImg;
    /**
    * pointer to the red plane input image
    */
    ImageOf<PixelMono> *ptr_inputImgRed;
    /**
    * pointer to the green plane input image
    */
    ImageOf<PixelMono> *ptr_inputImgGreen;
    /**
    * pointer to the input blue plane image
    */
    ImageOf<PixelMono> *ptr_inputImgBlue;
    /**
    * pointer to the input image R+G-
    */
    ImageOf<PixelMono> *ptr_inputImgRG;
    /**
    * pointer to the input image G+R-
    */
    ImageOf<PixelMono> *ptr_inputImgGR;
    /**
    * pointer to the input image B+Y-
    */
    ImageOf<PixelMono> *ptr_inputImgBY;
    
   /**
    * pointer to the output image of the watershed algorithm
    */
    ImageOf<PixelRgb>* _procImage;
    /**
    * pointer to the output image of the watershed algorithm
    */
    ImageOf<PixelMono>* _outputImage;
    //---------- flags --------------------------
    /**
    * flag for drawing contrastLP
    */
    bool contrastLP_flag;
    /**
    * flag for drawing meanColourImage
    */
    bool meanColour_flag;
    /**
    * flag for drawing blobCatalog
    */
    bool blobCataloged_flag;
    /**
    * flag for drawing foveaBlob
    */
    bool foveaBlob_flag;
    /**
    * flag for drawing colorVQ
    */
    bool colorVQ_flag;
    /**
    * flag for drawing maxSaliencyBlob
    */
    bool maxSaliencyBlob_flag;
    /**
    * flag for drawing blobList
    */
    bool blobList_flag;
    /**
    * flag for the drawings
    */
    bool tagged_flag;
    /**
    * flag for drawing watershed image
    */
    bool watershed_flag;
    /**
    * function that indicates if the stimuli have to be processed
    */
    bool filterSpikes_flag;

    //------------parameters modified by the interface
    /**
    * maxBLOB dimension
    */
    int maxBLOB;
    /**
    * minBLOB dimension
    */
    int minBLOB;
    /**
    * saliencyTOT linear combination Ktd coefficient (TOP DOWN saliency weight)
    */
    double salienceTD;
    /**
    * saliencyTOT linear combination Kbu coefficient (BOTTOM-UP saliency weight)
    */
    double salienceBU;
    /**
    * red intensity of the target that has been found 
    */
    double targetRED;
    /**
    * green intensity of the target that has been found 
    */
    double targetGREEN;
    /**
    * blue intensity of the target that has been found 
    */
    double targetBLUE;
    /**
    * value that represent the constantTimeGazeControl of the sensorial system in terms of second
    */
    double constantTimeGazeControl;
    /**
    * value that represent the constantTimeCentroid of the sensorial system in terms of second
    */
    double constantTimeCentroid;
    /**
    * counter of cycle for maxsaliency blob
    */
    int count;
    /**
    * number of blobs
    */
    int max_tag;
    /**
    * number of spikes which are count to get the strongest
    */
    int countSpikes;
};

#endif //__BLOBFINDERTHREAD_H_

//----- end-of-file --- ( next line intentionally left blank ) ------------------
