// -*- mode:C++; tab-width:4; c-basic-offset:4; indent-tabs-mode:nil -*-

#include <iCub/WatershedModule.h>
#include <iCub/blobFinderModule.h>


//YARP include
#include <yarp/os/all.h>
#include <yarp/sig/all.h>

//openCV include
#include <cv.h>
#include <cvaux.h>
#include <highgui.h>

//within Project Include
//#include <iCub/ImageProcessor.h>

using namespace yarp::os;
using namespace yarp::sig;
using namespace yarp::sig::draw;


#define THREADRATE 30
#define BLOB_MAXSIZE 4096
#define BLOB_MINSIZE 100


static GtkWidget *menubar;
static GtkWidget *fileMenu, *imageMenu, *helpMenu;
static GtkWidget *fileItem, *imageItem, *helpItem;
static GtkWidget *fileSingleItem, *fileSetItem, *fileQuitItem;
static GtkWidget *mainWindow = NULL;
//status bar of the windows
static GtkWidget *statusbar;
//drawing area
static GtkWidget *da;
// Current frame
static GdkPixbuf *frame = NULL;

// Image Receiver
static YARPImgRecv *ptr_imgRecv;

static YARPImgRecv *ptr_imgRecvRed;
static YARPImgRecv *ptr_imgRecvGreen;
static YARPImgRecv *ptr_imgRecvBlue;
static YARPImgRecv *ptr_imgRecvRG;
static YARPImgRecv *ptr_imgRecvGR;
static YARPImgRecv *ptr_imgRecvBY;

// Image to Display
static yarp::sig::ImageOf<yarp::sig::PixelRgb> *ptr_inputImg;

static yarp::sig::ImageOf<yarp::sig::PixelMono> *ptr_inputImgRed;
static yarp::sig::ImageOf<yarp::sig::PixelMono> *ptr_inputImgGreen;
static yarp::sig::ImageOf<yarp::sig::PixelMono> *ptr_inputImgBlue;
static yarp::sig::ImageOf<yarp::sig::PixelMono> *ptr_inputImgRG;
static yarp::sig::ImageOf<yarp::sig::PixelMono> *ptr_inputImgGR;
static yarp::sig::ImageOf<yarp::sig::PixelMono> *ptr_inputImgBY;

static yarp::sig::ImageOf<yarp::sig::PixelMono> *ptr_middleImg;
static yarp::sig::ImageOf<yarp::sig::PixelInt> *ptr_tagged;
static yarp::sig::ImageOf<yarp::sig::PixelMono>* _outputImage;
static yarp::sig::ImageOf<yarp::sig::PixelRgb>* _outputImage3;

static ImageOf<PixelMono> rgs;
static ImageOf<PixelMono> grs;
static ImageOf<PixelMono> bys;
static ImageOf<PixelMono> r2;
static ImageOf<PixelMono> g2;
static ImageOf<PixelMono> b2;


// Semaphore
static yarp::os::Semaphore *ptr_semaphore;
// Timeout ID
static guint timeout_ID;
// Watershed operator
static WatershedOperator *_wOperator;
static SalienceOperator *_salience;
static WatershedModule *wModule;
static blobFinderModule* bfModule;

#define _imgRecv (*(ptr_imgRecv))

#define _imgRecvRed (*(ptr_imgRecvRed))
#define _imgRecvGreen (*(ptr_imgRecvGreen))
#define _imgRecvBlue (*(ptr_imgRecvBlue))
#define _imgRecvRG (*(ptr_imgRecvRG))
#define _imgRecvGR (*(ptr_imgRecvGR))
#define _imgRecvBY (*(ptr_imgRecvBY))

#define _inputImg (*(ptr_inputImg))

#define _inputImgRed (*(ptr_inputImgRed))
#define _inputImgGreen (*(ptr_inputImgGreen))
#define _inputImgBlue (*(ptr_inputImgBlue))
#define _inputImgRG (*(ptr_inputImgRG))
#define _inputImgGR (*(ptr_inputImgGR))
#define _inputImgBY (*(ptr_inputImgBY))

#define _middleImg (*(ptr_middleImg))
#define _tagged (*(ptr_tagged))
#define _semaphore (*(ptr_semaphore))

WatershedModule::WatershedModule() { //:RateThread(THREADRATE){
    ct=0;

    message=new std::string();

    printf("initialising all the flags \n");
    inputImage_flag=false;

    meanColour_flag=true;
    contrastLP_flag=false;
    blobCataloged_flag=true;
    foveaBlob_flag=false;
    colorVQ_flag=false;
    blobList_flag=false;
    maxSaliencyBlob_flag=false;
    tagged_flag=false;
    watershed_flag=false;
    bluePlane_flag=false;
    redPlane_flag=false;
    greenPlane_flag=false;
    RG_flag=false;
    GR_flag=false;
    BY_flag=false;
    noOpponencies_flag=true;
    noPlanes_flag=true;
    resized_flag=false;
    //----

    maxSalienceBlob_img=new ImageOf<PixelMono>;
    outContrastLP=new ImageOf<PixelMono>;
    outMeanColourLP=new ImageOf<PixelBgr>;
    
    wModule=this;

    max_boxes = new YARPBox[3];
    //initializing the image plotted out int the drawing area
    printf("initialising the images \n");
    image_out=new ImageOf<PixelRgb>;
    _outputImage3=new ImageOf<PixelRgb>;
    _outputImage=new ImageOf<PixelMono>;

    ptr_inputRed=new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the red plane
    ptr_inputGreen= new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the green plane
    ptr_inputBlue= new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the blue plane
    ptr_inputRG= new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the R+G- colour opponency
    ptr_inputGR= new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the G+R- colour opponency
    ptr_inputBY= new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the B+Y- colour opponency
    
    _inputImgRGS=new ImageOf<PixelMono>;
    _inputImgGRS=new ImageOf<PixelMono>;
    _inputImgBYS=new ImageOf<PixelMono>;
    
    blobFov=new ImageOf<PixelMono>;
    

    salienceBU=10;
    salienceTD=10;
    maxBLOB=4096;
    minBLOB=100;

    reactivity=5.0;

    targetRED=1;
    targetGREEN=1;
    targetBLUE=1;
    searchRG=0;
    searchGR=0;
    searchBY=0;
    
}

WatershedModule::~WatershedModule(){
    delete maxSalienceBlob_img; //=new ImageOf<PixelMono>;
    delete outContrastLP;//=new ImageOf<PixelMono>;
    delete outMeanColourLP;//=new ImageOf<PixelBgr>;
    
    //initializing the image plotted out int the drawing area
    delete image_out;//=new ImageOf<PixelRgb>;
    delete _outputImage3;//=new ImageOf<PixelRgb>;
    delete _outputImage;//=new ImageOf<PixelMono>;

    delete ptr_inputRed;//=new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the red plane
    delete ptr_inputGreen;//= new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the green plane
    delete ptr_inputBlue;// new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the blue plane
    delete ptr_inputRG;//= new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the R+G- colour opponency
    delete ptr_inputGR;//= new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the G+R- colour opponency
    delete ptr_inputBY;//= new ImageOf<yarp::sig::PixelMono>; //pointer to the input image of the B+Y- colour opponency
    
    delete _inputImgRGS;//=new ImageOf<PixelMono>;
    delete _inputImgGRS;//=new ImageOf<PixelMono>;
    delete _inputImgBYS;//=new ImageOf<PixelMono>;
    
    delete blobFov;//=new ImageOf<PixelMono>;
}

void WatershedModule::resizeImages(int width, int height){
    this->width=width;
    this->height=height;

    int widthstep=256;

    ptr_tagged = new yarp::sig::ImageOf<yarp::sig::PixelInt>;
    ptr_tagged->resize(widthstep,height);
    
    this->wOperator=new WatershedOperator(true,width,height,widthstep,10);
    _wOperator=this->wOperator;
    this->salience=new SalienceOperator(width,height);
    _salience=this->salience;

    maxSalienceBlob_img->resize(width,height);
    outMeanColourLP->resize(width,height);
    outContrastLP->resize(width,height);

    image_out->resize(width,height);
    _outputImage->resize(width,height);
    _outputImage3->resize(width,height);

    ptr_inputRed->resize(width,height);
    ptr_inputGreen->resize(width,height);
    ptr_inputBlue->resize(width,height);
    ptr_inputRG->resize(width,height);
    ptr_inputGR->resize(width,height);
    ptr_inputBY->resize(width,height);

    _inputImgRGS->resize(width,height);
    _inputImgGRS->resize(width,height);
    _inputImgBYS->resize(width,height);
    blobFov->resize(width,height);

    blobList = new char [width*height+1];

    _inputImg.resize(width,height);
    _inputImgRed.resize(width,height);
    _inputImgGreen.resize(width,height);
    _inputImgBlue.resize(width,height);
    _inputImgRG.resize(width,height);
    _inputImgGR.resize(width,height);
    _inputImgBY.resize(width,height);

    resized_flag=true;
}


bool WatershedModule::threadInit(){
    // create a new window
    printf(" thread initialisation \n");
    this->createObjects();
    this->setUp();
    printf(" creating the main windows \n");
    mainWindow = this->createMainWindow();
    

    // Shows all widgets in main Window
    gtk_widget_show_all (mainWindow);
    gtk_window_move(GTK_WINDOW(mainWindow), 10,10);
    // All GTK applications must have a gtk_main(). Control ends here
    // and waits for an event to occur (like a key press or
    // mouse event).

    
    return true;
}

void WatershedModule::run(){
    printf("starting the gtk_main \n");
    gtk_main();
    gtk_widget_destroy(mainWindow);
    this->interrupt();
    this->close();
//    bfModule->close();
    //this->close();
    //yarp::os::Network::fini();
}

void WatershedModule::threadRelease(){

}

void WatershedModule::onStop(){
    gtk_main_quit();
    this->interrupt();
    this->close();
}

bool WatershedModule::close() {
    printf("Closing all the ports \n");
    /**
    * a port for reading the edge image 
    */
    port_in.close(); // 
    /**
    * a port for reading the input Image of the Red Plane
    */
    portRedPlane.close(); // 
    /**
    * a port for reading the input Image of the Green Plane
    */
    portGreenPlane.close();
    /**
    * a port for reading the input Image of the Blue Plane 
    */
    portBluePlane.close(); 
    /**
    * a port for reading the R+G- colour opponency Image
    */
    portRG.close(); // 
    /**
    * a port for reading the G+R- colour opponency Image
    */
    portGR.close(); // 
    /**
    * a port for reading the B+Y- colour opponency Image 
    */
    portBY.close(); // 
    /**
    * port where the processed image is buffered out
    */
    port_out.close(); //
    /**
    * port where the image of the found blob is put
    */
    port_Blobs.close(); //
    
    closePorts();

        
    return true;
}

// try to interrupt any communications or resource usage
bool WatershedModule::interrupt(){
    /**
    * a port for reading the edge image 
    */
        port_in.interrupt(); // 
    /**
    * a port for reading the input Image of the Red Plane
    */
        portRedPlane.interrupt(); // 
    /**
    * a port for reading the input Image of the Green Plane
    */
        portGreenPlane.interrupt();
    /**
    * a port for reading the input Image of the Blue Plane 
    */
        portBluePlane.interrupt(); 
    /**
    * a port for reading the R+G- colour opponency Image
    */
        portRG.interrupt(); // 
    /**
    * a port for reading the G+R- colour opponency Image
    */
        portGR.interrupt(); // 
    /**
    * a port for reading the B+Y- colour opponency Image 
    */
        portBY.interrupt(); // 
    /**
    * port where the processed image is buffered out
    */
    port_out.interrupt(); //
    /**
    * port where the image of the found blob is put
    */
    port_Blobs.interrupt(); //
    return true;
}

void WatershedModule::setModule(void* refModule){
    bfModule=(blobFinderModule*)refModule;
}


bool getPlanes(){
    bool ret = false;
    //ret = _imgRecvRed.Update();
    
    //ret = _imgRecvGreen.Update();
    //ret = _imgRecvBlue.Update();

    
    /*
    _semaphore.wait();
    ret = _imgRecvRed.GetLastImage(&_inputImgRed);
    wModule->ptr_inputRed=&_inputImgRed;
    _semaphore.post();
    _semaphore.wait();
    ret = _imgRecvGreen.GetLastImage(&_inputImgGreen);
    wModule->ptr_inputGreen=&_inputImgGreen;
    _semaphore.post();
    _semaphore.wait();
    ret = _imgRecvBlue.GetLastImage(&_inputImgBlue);
    wModule->ptr_inputBlue=&_inputImgBlue;
    _semaphore.post();
    */
    
    //printf("GetImage: out of the semaphore \n");
    return ret;
}

bool getOpponencies(){
    bool ret = false;
    /*
    ret = _imgRecvRG.Update();
    ret = _imgRecvGR.Update();
    ret = _imgRecvBY.Update();
    */

    
    /*
    _semaphore.wait();
    ret = _imgRecvGR.GetLastImage(&_inputImgGR);
    wModule->ptr_inputGR=&_inputImgGR;
    _semaphore.post();
    _semaphore.wait();
    ret = _imgRecvRG.GetLastImage(&_inputImgRG);
    wModule->ptr_inputRG=&_inputImgRG;
    _semaphore.post();
    _semaphore.wait();
    ret = _imgRecvBY.GetLastImage(&_inputImgBY);
    wModule->ptr_inputBY=&_inputImgBY;
    _semaphore.post();
    //printf("GetImage: out of the semaphore \n");
    */
    return ret;
}
/**
* main function that associates any pixel in the image to a tagged blob
*/
void WatershedModule::rain(){
        max_tag=wOperator->apply(*_outputImage,_tagged);
        //printf("MAX_TAG=%d",wModule->max_tag);
        /*bool ret=getPlanes();
        if(ret==false){
            //printf("No Planes! \n");
            //return;
        }
        ret=getOpponencies();
        if(ret==false){
            //printf("No Opponency! \n");
            //return;
        }*/
        int psb32s;
        IppiSize srcsize={this->width,this->height};
        Ipp32s* _inputImgRGS32=ippiMalloc_32s_C1(this->width,this->height,&psb32s);
        Ipp32s* _inputImgGRS32=ippiMalloc_32s_C1(this->width,this->height,&psb32s);
        Ipp32s* _inputImgBYS32=ippiMalloc_32s_C1(this->width,this->height,&psb32s);
        /*ImageOf<PixelMono> *_inputImgRGS=new ImageOf<PixelMono>;
        ImageOf<PixelMono> *_inputImgGRS=new ImageOf<PixelMono>;
        ImageOf<PixelMono> *_inputImgBYS=new ImageOf<PixelMono>;
        _inputImgRGS->resize(320,240);
        _inputImgGRS->resize(320,240);
        _inputImgBYS->resize(320,240);*/
        if(ptr_inputImgRG!=NULL){
            ippiScale_8u32s_C1R(_inputImgRG.getRawImage(),_inputImgRG.getRowSize(),_inputImgRGS32,psb32s,srcsize);
            ippiConvert_32s8s_C1R(_inputImgRGS32,psb32s,(Ipp8s*)wModule->_inputImgRGS->getRawImage(),wModule->_inputImgRGS->getRowSize(),srcsize);
            //_inputImgRGS->copy(_inputImgRG,320,240);
        }
        else
            return;
        if(ptr_inputImgGR!=NULL){
            ippiScale_8u32s_C1R(_inputImgGR.getRawImage(),_inputImgGR.getRowSize(),_inputImgGRS32,psb32s,srcsize);
            ippiConvert_32s8s_C1R(_inputImgGRS32,psb32s,(Ipp8s*)wModule->_inputImgGRS->getRawImage(),wModule->_inputImgGRS->getRowSize(),srcsize);
            //_inputImgGRS->copy(_inputImgGR,320,240);
        }
        else
            return;
        if(ptr_inputImgBY!=NULL){
            ippiScale_8u32s_C1R(_inputImgBY.getRawImage(),_inputImgBY.getRowSize(),_inputImgBYS32,psb32s,srcsize);
            ippiConvert_32s8s_C1R(_inputImgBYS32,psb32s,(Ipp8s*)wModule->_inputImgBYS->getRawImage(),wModule->_inputImgBYS->getRowSize(),srcsize);
            //_inputImgBYS->copy(_inputImgBY,320,240);
        }
        else
            return;
        salience->blobCatalog(_tagged, *wModule->_inputImgRGS, *wModule->_inputImgGRS, *wModule->_inputImgBYS,
            _inputImgBlue, _inputImgGreen, _inputImgRed, wModule->max_tag);
        blobCataloged_flag=true;
        //istruction to set the ptr_tagged in the Watershed Module with the static variable _tagged
        tagged=ptr_tagged; //ptr_tagged is the pointer to _tagged
        ippiFree(_inputImgRGS32); //Ipp32s* _inputImgRGS32=ippiMalloc_32s_C1(320,240,&psb32s);
        ippiFree(_inputImgGRS32); //Ipp32s* _inputImgGRS32=ippiMalloc_32s_C1(320,240,&psb32s);
        ippiFree(_inputImgBYS32); //Ipp32s* _inputImgBYS32=ippiMalloc_32s_C1(320,240,&psb32s);
}


//-------------------------------------------------
// Main Window Callbacks
//-------------------------------------------------

/* usual callback function */
static void callback( GtkWidget *widget,gpointer   data ){
    printf ("Hello again - %s was pressed \n", (char *) data);
    
    if(!strcmp((char *)data,"Rain1")){
        wModule->rain();
    }
    else if(!strcmp((char *)data,"Rain2")){
        printf("Rain2");
        wModule->max_tag=wModule->wOperator->apply(*_outputImage,_tagged);
        wModule->salience->blobCatalog(_tagged, rgs, grs, bys, r2, g2, b2, wModule->max_tag);
    }
    else if(!strcmp((char *)data,"Rain3")){
        printf("Rain3");
        wModule->max_tag=wModule->wOperator->apply(*_outputImage,_tagged);
        wModule->salience->blobCatalog(_tagged, rgs, grs, bys, r2, g2, b2, wModule->max_tag);
    }
    else if(!strcmp((char *)data,"DrawAllBlobs1")){
        printf("DrawAllBlobs1");
        wModule->drawAllBlobs(false);
    }
    else if(!strcmp((char *)data,"DrawAllBlobs2")){
        printf("DrawAllBlobs2");
        wModule->drawAllBlobs(false);
    }
    else if(!strcmp((char *)data,"DrawAllBlobs3")){
        printf("DrawAllBlobs3");
        wModule->drawAllBlobs(false);
    }
    else if(!strcmp((char *)data,"drawFoveaBlob1")){
        printf("drawFoveaBlob1");
        wModule->salience->drawFoveaBlob(*wModule->salience->foveaBlob,*wModule->tagged);
    }
    else if(!strcmp((char *)data,"drawFoveaBlob2")){
        printf("drawFoveaBlob2");
        
    }
    else if(!strcmp((char *)data,"drawFoveaBlob3")){
        printf("drawFoveaBlob3");
        
    }
    else if(!strcmp((char *)data,"drawVQColor1")){
        printf("drawColorVQ1 function");
        wModule->salience->DrawVQColor(*wModule->salience->colorVQ_img,*wModule->tagged);
    }
    else if(!strcmp((char *)data,"drawVQColor2")){
        printf("drawFoveaBlob2");
        
    }
    else if(!strcmp((char *)data,"drawVQColor3")){
        printf("drawFoveaBlob3");
        
    }
    else if(!strcmp((char *)data,"ContrastLP1")){
        printf("ContrastLP1 function");
        wModule->salience->DrawVQColor(*wModule->salience->colorVQ_img,*wModule->tagged);
    }
    else if(!strcmp((char *)data,"ContrastLP2")){
        printf("ContrastLP2");
        
    }
    else if(!strcmp((char *)data,"ContrastLP3")){
        printf("ContrastLP3");
        
    }
    else if(!strcmp((char *)data,"maxSalienceBlob1")){
        printf("drawColorVQ1 function");
        wModule->salience->maxSalienceBlob(*wModule->tagged, wModule->max_tag,wModule->max_boxes[0]);
    }
    else if(!strcmp((char *)data,"maxSalienceBlob2")){
        printf("drawFoveaBlob2");
        
    }
    else if(!strcmp((char *)data,"maxSalienceBlob3")){
        printf("drawFoveaBlob3");
        
    }
    
}

void cleanExit(){
    /*g_source_remove (timeout_ID);
    timeout_ID = 0;
    //closePorts();
    if (_options.saveOnExit != 0)
        saveOptFile(_options.fileName);
    if (frame)
        g_object_unref(frame);*/
    printf("cleanExit \n");
    // Exit from application
    gtk_main_quit ();
    //deleteObjects();
}


static gint expose_CB (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    //printf("entering expose_CB \n");
    if(frame){
        //printf("frame not null");
        if (mainWindow){
                //printf("frame and mainWindow present \n");
                guchar *pixels;
                unsigned int rowstride;
                unsigned int imageWidth,imageHeight,areaWidth, areaHeight;
                

                if(!wModule->inputImage_flag){
                    return true;
                }
                IppiSize srcsize={wModule->width,wModule->height};
                //bool ret=getPlanes();				
                /*if(ret==false){
                    //printf("No Planes! \n");
                    //return TRUE;
                }
                else{
                    wModule->noPlanes_flag=false;
                }*/
                //ret=getOpponencies();
                /*if(ret==false){
                    //printf("No Opponency! \n");
                    //return TRUE;
                }
                else{
                    wModule->noOpponencies_flag=false;
                }*/
                if(wModule->noOpponencies_flag | wModule->noPlanes_flag){
                    return true;
                }

                wModule->ct++;

                //=new yarp::sig::ImageOf<yarp::sig::PixelRgb>;
                //_outputImage->resize(320,240);
                bool conversion=true;
                //_outputImage=_wOperator->getPlane(&_inputImg); 
                //wModule->rain();
                
                if(wModule->foveaBlob_flag){
                    //wModule->salience->drawFoveaBlob(*wModule->salience->foveaBlob,*wModule->tagged);
                    //ippiCopy_8u_C1R(wModule->salience->foveaBlob->getRawImage(),wModule->salience->foveaBlob->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->redPlane_flag){
                    //ippiCopy_8u_C1R(wModule->ptr_inputRed->getRawImage(),wModule->ptr_inputRed->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->greenPlane_flag){
                    //ippiCopy_8u_C1R(wModule->ptr_inputGreen->getRawImage(),wModule->ptr_inputGreen->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->bluePlane_flag){
                    //ippiCopy_8u_C1R(wModule->ptr_inputBlue->getRawImage(),wModule->ptr_inputBlue->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->RG_flag){
                    //ippiCopy_8u_C1R(wModule->ptr_inputRG->getRawImage(),wModule->ptr_inputRG->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->GR_flag){
                    //ippiCopy_8u_C1R(wModule->ptr_inputGR->getRawImage(),wModule->ptr_inputGR->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->BY_flag){
                    //ippiCopy_8u_C1R(wModule->ptr_inputBY->getRawImage(),wModule->ptr_inputBY->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->watershed_flag){
                    //ippiCopy_8u_C1R(wModule->wOperator->tSrc.getRawImage(),wModule->wOperator->tSrc.getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->tagged_flag){
                    //printf("dimension of the tagged image %d,%d \n", wModule->tagged->width(), wModule->tagged->height());
                    for(int y=0; y<wModule->tagged->height(); y++){
                        for (int x=0; x<wModule->tagged->width(); x++){
                           // _outputImage->pixel(x,y)=(int)((wModule->max_tag/255)*wModule->tagged->pixel(x,y));
                        }
                    }
                    //cvCvtColor(wModule->tagged->getIplImage(),_outputImage->getIplImage(),CV_GRAY2RGB);
                    //cvCopy(wModule->tagged->getIplImage(),_outputImage->getIplImage());
                    //ippiCopy_8u_C1R(wModule->tagged->getRawImage(),320,_outputImage->getRawImage(),320,srcsize);
                    conversion=true;
                }
                else if(wModule->blobList_flag){
                   // wModule->drawAllBlobs(false);
                    /*if(wModule->blobList!=""){
                        ippiCopy_8u_C1R((unsigned char*)wModule->blobList,320,_outputImage->getRawImage(),320,srcsize);
                        conversion=true;
                    }*/
                }
                else if(wModule->maxSaliencyBlob_flag){
                    //wModule->drawAllBlobs(false);
                    //wModule->salience->DrawMaxSaliencyBlob(*wModule->maxSalienceBlob_img,wModule->max_tag,*wModule->tagged);
                    //ippiCopy_8u_C1R(wModule->maxSalienceBlob_img->getRawImage(),wModule->maxSalienceBlob_img->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->contrastLP_flag){
                    //wModule->drawAllBlobs(false);
                    //ippiCopy_8u_C3R(wModule->outMeanColourLP->getRawImage(),320*3,_outputImage3->getRawImage(),320*3,srcsize);	
                    //ippiCopy_8u_C1R(wModule->outContrastLP->getRawImage(),wModule->outContrastLP->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                    conversion=true;
                }
                else if(wModule->colorVQ_flag){
                    //wModule->salience->DrawVQColor(*wModule->salience->colorVQ_img,*wModule->tagged);
                    //ippiCopy_8u_C3R(wModule->salience->colorVQ_img->getRawImage(),wModule->salience->colorVQ_img->getRowSize(),_outputImage3->getRawImage(),_outputImage3->getRowSize(),srcsize);
                    conversion=false;
                }
                else if(wModule->blobCataloged_flag){
                    if(wModule->contrastLP_flag){
                        //wModule->drawAllBlobs(false);
                        //ippiCopy_8u_C3R(wModule->outMeanColourLP->getRawImage(),320*3,_outputImage3->getRawImage(),320*3,srcsize);	
                        //ippiCopy_8u_C1R(wModule->outContrastLP->getRawImage(),wModule->outContrastLP->getRowSize(),_outputImage->getRawImage(),_outputImage->getRowSize(),srcsize);
                        conversion=true;
                    }
                    else if(wModule->meanColour_flag){
                        //_outputImage=_wOperator->getPlane(&_inputImg); 
                        //rain();
                        //wModule->drawAllBlobs(false);
                        //ippiCopy_8u_C3R(wModule->outMeanColourLP->getRawImage(),wModule->outMeanColourLP->getRowSize(),_outputImage3->getRawImage(),_outputImage3->getRowSize(),srcsize);	
                        conversion=false;
                    }
                    /*else
                    _outputImage=_wOperator->getPlane(&_inputImg); //the input is a RGB image, whereas the watershed is working with a mono image*/
                }
                /*else
                _outputImage=_wOperator->getPlane(&_inputImg); //the input is a RGB image, whereas the watershed is working with a mono image*/

                //-------
                
                /*if(conversion){
                    int psb;
                    int width=wModule->width;
                    int height=wModule->height;
                    Ipp8u* im_out = ippiMalloc_8u_C1(width,height,&psb);
                    //Ipp8u* im_tmp0 = ippiMalloc_8u_C1(width,height,&psb);
                    //Ipp8u* im_tmp1= ippiMalloc_8u_C1(width,height,&psb);
                    //Ipp8u* im_tmp2 = ippiMalloc_8u_C1(width,height,&psb);
                    //two copies in order to have 2 conversions
                    //the first transform the yarp mono into a 4-channel image
                    ippiCopy_8u_C1R(_outputImage->getRawImage(), _outputImage->getRowSize(),im_out,psb,srcsize);

                    //ippiCopy_8u_C1R(im_out, width,im_tmp0,psb,srcsize);
                    //ippiCopy_8u_C1R(im_out, width,im_tmp1,psb,srcsize);
                    //ippiCopy_8u_C1R(im_out, width,im_tmp2,psb,srcsize);

                    //im_tmp0=im_out;
                    //im_tmp1=im_out;
                    //im_tmp2=im_out;

                    //Ipp8u* im_tmp[3]={im_tmp0,im_tmp1,im_tmp2};

                    Ipp8u* im_tmp[3]={im_out,im_out,im_out};
                    //Ipp8u* im_tmp[3]={_outputImage->getRawImage(),_outputImage->getRawImage(),_outputImage->getRawImage()};
                    //the second transforms the 4-channel image into colorImage for yarp
                    ippiCopy_8u_P3C3R(im_tmp,psb,wModule->image_out->getRawImage(),wModule->image_out->getRowSize(),srcsize);
                    ippiFree(im_out);
                    //printf("freeing im_tmp0  \n");	
                    //ippiFree(im_tmp0);
                    //printf("freeing im_tmp1 \n");	
                    //ippiFree(im_tmp1);
                    //printf("freeing im_tmp2  \n");	
                    //ippiFree(im_tmp2);
                    //printf("freeing ended  \n");	
                }
                else
                    ippiCopy_8u_C3R(_outputImage3->getRawImage(),_outputImage3->getRowSize(),wModule->image_out->getRawImage(),wModule->image_out->getRowSize(),srcsize);*/
                
                //----------
                _semaphore.wait();
                bool result=yarpImage2Pixbuf(wModule->image_out, frame);
                //bool result=yarpImage2Pixbuf(&_inputImg, frame);
                imageWidth = _inputImg.width();
                imageHeight = _inputImg.height();
                _semaphore.post();

                
                
                
                if (imageWidth==0||imageHeight==0) {
                    printf("exit for dimension nil \n");
                    return TRUE;
                }
     
                areaWidth = event->area.width;
                areaHeight = event->area.height;

                unsigned int pixbufWidth=gdk_pixbuf_get_width(frame);
                unsigned int pixbufHeight=gdk_pixbuf_get_height(frame);

                if ((imageWidth!=pixbufWidth) || (imageHeight!=pixbufHeight))
                    {
                        g_object_unref(frame);
                        //printf("unreferencing frame \n");
                        frame=gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, imageWidth, imageHeight);
                    }

                if ( (areaWidth != imageWidth) || (areaHeight != imageHeight) )
                    {
                        GdkPixbuf *scaledFrame;
                        //printf("scaling image... \n");
                        scaledFrame = gdk_pixbuf_scale_simple(	frame,
                                                                areaWidth,
                                                                areaHeight,
                                                                GDK_INTERP_BILINEAR); // Best quality
                        //GDK_INTERP_NEAREST); // Best speed

                        pixels = gdk_pixbuf_get_pixels (scaledFrame);
                        rowstride = gdk_pixbuf_get_rowstride(scaledFrame);
                        gdk_draw_rgb_image (widget->window,
                                            widget->style->black_gc,
                                            event->area.x, event->area.y,
                                            event->area.width, event->area.height,
                                            GDK_RGB_DITHER_NORMAL,
                                            pixels,
                                            rowstride);
                        g_object_unref(scaledFrame);
                
                    }
                else
                    {
                        pixels = gdk_pixbuf_get_pixels (frame);
                        rowstride = gdk_pixbuf_get_rowstride(frame);
                        //printf("drawing image... \n");
                        gdk_draw_rgb_image (widget->window,
                                            widget->style->black_gc,
                                            event->area.x, event->area.y,
                                            event->area.width, event->area.height,
                                            GDK_RGB_DITHER_NORMAL,
                                            pixels,
                                            rowstride);
                    }
        }
        else{
            //printf("mainWindow results nil");
        }
    
    }
    return TRUE;
}

static void cb_draw_value( GtkToggleButton *button )
{
    /* Turn the value display on the scale widgets off or on depending
     *  on the state of the checkbutton */
    printf("callbacks from draw value %s \n",button->button.label_text);
    if(!strcmp(button->button.label_text,"<--RedPlane")){
        if(button->active)
            wModule->redPlane_flag=true;
        else
            wModule->redPlane_flag=false;
    }
    else if(!strcmp(button->button.label_text,"<--GreenPlane")){
        if(button->active)
            wModule->greenPlane_flag=true;
        else
            wModule->greenPlane_flag=false;
    }
    else if(!strcmp(button->button.label_text,"<--BluePlane")){
        if(button->active)
            wModule->bluePlane_flag=true;
        else
            wModule->bluePlane_flag=false;
    }
    if(!strcmp(button->button.label_text,"<--RGImage")){
        if(button->active)
            wModule->RG_flag=true;
        else
            wModule->RG_flag=false;
    }
    else if(!strcmp(button->button.label_text,"<--GRImage")){
        if(button->active)
            wModule->GR_flag=true;
        else
            wModule->GR_flag=false;
    }
    else if(!strcmp(button->button.label_text,"<--BYImage")){
        if(button->active)
            wModule->BY_flag=true;
        else
            wModule->BY_flag=false;
    }
    else if(!strcmp(button->button.label_text,"Red-->")){
        if(button->active){
            //imageProcessModule->processor1->redPlane_flag=1;
            //imageProcessModule->processor1->greenPlane_flag=0;
            //imageProcessModule->processor1->bluePlane_flag=0;
        }
    }
    else if(!strcmp(button->button.label_text,"Green-->")){
        if(button->active){
            //imageProcessModule->processor1->redPlane_flag=0;
            //imageProcessModule->processor1->greenPlane_flag=1;
            //imageProcessModule->processor1->bluePlane_flag=0;
        }
    }
    else if(!strcmp(button->button.label_text,"Blue-->")){
        if(button->active){
            //imageProcessModule->processor1->redPlane_flag=0;
            //imageProcessModule->processor1->greenPlane_flag=0;
            //imageProcessModule->processor1->bluePlane_flag=1;
        }
    }
    
    else if(!strcmp(button->button.label_text,"ContrastLP-->")){
        if(button->active){
            wModule->contrastLP_flag=true;
            wModule->message->assign("set clp");
        }
        else
            wModule->contrastLP_flag=false;
    }
    
    else if(!strcmp(button->button.label_text,"MeanColoursLP-->")){
        if(button->active){
            wModule->meanColour_flag=true;
            wModule->message->assign("set mea");
        }
        else
            wModule->meanColour_flag=false;
            
    }
    
    else if(!strcmp(button->button.label_text,"MaxSaliencyBlob-->")){
        if(button->active){
            wModule->maxSaliencyBlob_flag=true;
            wModule->message->assign("set max");
        }
        else
            wModule->maxSaliencyBlob_flag=false;
            
    }
    
    else if(!strcmp(button->button.label_text,"FoveaBlob-->")){
        if(button->active){
            wModule->foveaBlob_flag=true;
            wModule->message->assign("set fov");
         }
        else
            wModule->foveaBlob_flag=false;
            
    }
    
    else if(!strcmp(button->button.label_text,"ColorVQ-->")){
        if(button->active)
            wModule->colorVQ_flag=true;
        else
            wModule->colorVQ_flag=false;
            
    }
    
    else if(!strcmp(button->button.label_text,"BlobList-->")){
        if(button->active)
            wModule->blobList_flag=true;
        else
            wModule->blobList_flag=false;
            
    }
    
    else if(!strcmp(button->button.label_text,"Tagged-->")){
        if(button->active){
            wModule->tagged_flag=true;
            wModule->message->assign("set tag");
        }
        else
            wModule->tagged_flag=false;
            
    }
    
    else if(!strcmp(button->button.label_text,"Watershed-->")){
        if(button->active){
            wModule->watershed_flag=true;
            wModule->message->assign("set wat");
        }
        else
            wModule->watershed_flag=false;
            
    }
    
}

bool WatershedModule::outPorts(){
    bool ret = true;

    //initialization and allocation

    /*IplImage *cvImage = cvCreateImage(cvSize(320,240),8, 3);
    cvCvtColor((IplImage*)outMeanColourLP->getIplImage(), cvImage, CV_BGR2RGB);
    image_out->wrapIplImage(cvImage);*/

    /*int psb;
    IppiSize srcsize={320,240};
    //printf("Entered in outPorts \n");
    Ipp8u* im_out = ippiMalloc_8u_C1(320,240,&psb);
    Ipp8u* im_tmp[3];
    Ipp8u* im_tmp_tmp= ippiMalloc_8u_C1(320,240,&psb);
    im_tmp[0]=im_out;
    im_tmp[1]=im_out;
    im_tmp[2]=im_out;
    ippiCopy_8u_C3P3R(this->outMeanColourLP->getRawImage(),320*3,im_tmp,psb,srcsize);
    ippiCopy_8u_C1R(im_tmp[2],psb,im_tmp_tmp,psb,srcsize);
    ippiCopy_8u_C1R(im_tmp[0],psb,im_tmp[2],psb,srcsize);
    ippiCopy_8u_C1R(im_tmp[2],psb,blobFov->getPixelAddress(0,0),psb,srcsize);*/

    //ippiCopy_8u_P3C3R(im_tmp,psb,image_out->getPixelAddress(0,0),320*3,srcsize);
    //this->_pOutPort2->prepare()=*(this->image_out);
    
    //this->_pOutPort2->prepare()=*(this->processor2->portImage);
    //this->_pOutPort3->prepare()=*(this->processor3->portImage);
    //printf("After prepares \n");
    /*ImageOf<PixelRgb>* out=new ImageOf<PixelRgb>;
    out->resize(this->width,this->height);
    IppiSize srcsize={this->width,this->height};
    ippiCopy_8u_C3R(wModule->outMeanColourLP->getRawImage(),wModule->outMeanColourLP->getRowSize(),out->getRawImage(),out->getRowSize(),srcsize);	*/

    //prepare the output on ports
    /*this->_pOutPort3->prepare()=*out;
    this->_pOutPort3->write();
    this->_pOutPort2->prepare()=*(this->image_out);
    this->_pOutPort2->write();*/
    
    
    
    if(strcmp("",message->c_str())){

        Bottle& commandBottle=commandPort->prepare();
        commandBottle.clear();
        commandBottle.addVocab(VOCAB3(message->at(0),message->at(1),message->at(2)));
        commandBottle.addVocab(VOCAB3(message->at(4),message->at(5),message->at(6)));
        if(message->length()>7){
            std::string sub=message->substr(7,message->length()-6);
            double value=atof(sub.c_str());
            commandBottle.addDouble(value);
        }

        
        commandPort->writeStrict();
        ct=1;
        message->assign("");
    }


    //deallocation
    //delete out;
    //ippiFree(im_out);
    //ippiFree(im_tmp_tmp);
    //ippiFree(im_tmp);
    return ret;
}

bool getImage(){
    /*bool ret = false;
    ret = _imgRecv.Update();

    if (ret == false){
        return false;
    }

    _semaphore.wait();
    ret = _imgRecv.GetLastImage(&_inputImg);
    _semaphore.post();
    
    //printf("GetImage: out of the semaphore \n");*/
    return false;
}


static gint menuFileQuit_CB(GtkWidget *widget, gpointer data)
{
    cleanExit();
    return TRUE;
}

static gint menuFileSingle_CB(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    if ( gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(widget)) ) 
        {
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(fileSetItem), FALSE);
            //gtk_widget_show_all (saveSingleDialog);
        
        } 
    else 
        {
            //gtk_widget_hide (saveSingleDialog);	
        }

    return TRUE;
}

static void cb_digits_scale( GtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    wModule->salienceBU=adj->value/100;
    printf("salienceBU: %f",wModule->salienceBU);
    std::string str("");
    sprintf((char *)str.c_str(),"set kbu %2.2f",wModule->salienceBU);
    wModule->message->assign(str.c_str());
}

static void cb_digits_scale2( GtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    wModule->salienceTD=adj->value/100;
    printf("salienceTD: %f",wModule->salienceTD);
    std::string str("");
    sprintf((char *)str.c_str(),"set ktd %2.2f",wModule->salienceTD);
    wModule->message->assign(str.c_str());
}

static void cb_digits_scale3( GtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    wModule->maxBLOB=adj->value;
    printf("maxBLOB: %f",wModule->maxBLOB);
    std::string str("");
    sprintf((char *)str.c_str(),"set Mdb %d",wModule->maxBLOB);
    wModule->message->assign(str.c_str());
}

static void cb_digits_scale4( GtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    wModule->minBLOB=adj->value;
    printf("minBLOB: %d",wModule->minBLOB);
    std::string str("");
    sprintf((char *)str.c_str(),"set mdb %d",wModule->minBLOB);
    wModule->message->assign(str.c_str());
}

static void cb_digits_scaler( GtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    wModule->targetRED=adj->value;
    printf("targetRED: %f",wModule->targetRED);
    std::string str("");
    sprintf((char *)str.c_str(),"set rin %2.2f",wModule->targetRED);
    wModule->message->assign(str.c_str());
}

static void cb_digits_scaleg( GtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    wModule->targetGREEN=adj->value;
    printf("targetGREEN: %f",wModule->targetGREEN);
    std::string str("");
    sprintf((char *)str.c_str(),"set gin %2.2f",wModule->targetGREEN);
    wModule->message->assign(str.c_str());
}

static void cb_digits_scaleb( GtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    wModule->targetBLUE=adj->value;
    printf("targetBLUE: %f",wModule->targetBLUE);
    std::string str("");
    sprintf((char *)str.c_str(),"set bin %2.2f",wModule->targetBLUE);
    wModule->message->assign(str.c_str());
}

static void cb_digits_scaletime( GtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    wModule->reactivity=adj->value;
    printf("constant time for the iKinControlGaze: %f",wModule->reactivity/10);
    std::string str("");
    sprintf((char *)str.c_str(),"set tco %2.2f",wModule->reactivity/10);
    wModule->message->assign(str.c_str());
}

static void cb_digits_scaletime2( GtkAdjustment *adj )
{
    /* Set the number of decimal places to which adj->value is rounded */
    wModule->timeCentroid=adj->value;
    printf("constant time for the controlGaze2: %f",wModule->timeCentroid/10);
    std::string str("");
    sprintf((char *)str.c_str(),"set tce %2.2f",wModule->timeCentroid/10);
    wModule->message->assign(str.c_str());
}


//-------------------------------------------------
// Main Window Statusbar
//-------------------------------------------------
static void updateStatusbar (GtkStatusbar  *statusbar)
{
    gchar *msg;
    float fps;
    fps = 1000 / float(50);
 
    gtk_statusbar_pop (statusbar, 0); // clear any previous message, underflow is allowed 
     
    PixelMono searchRG=0,searchGR=0,searchBY=0;
    
    searchRG=((wModule->targetRED-wModule->targetGREEN+255)/510)*255;
    wModule->searchRG=searchRG;
    //wModule->searchRG=wModule->targetRED;
    searchGR=((wModule->targetGREEN-wModule->targetRED+255)/510)*255;
    wModule->searchGR=searchGR;
    //wModule->searchGR=wModule->targetGREEN;
    PixelMono addRG=((wModule->targetRED+wModule->targetGREEN)/510)*255;
    searchBY=((wModule->targetBLUE-addRG+255)/510)*255;
    wModule->searchBY=searchBY;

    msg = g_strdup_printf ("r:%f;g:%f;b:%f     RG:%d;GR:%d;BY:%d", 
        wModule->targetRED,wModule->targetGREEN,wModule->targetBLUE, wModule->searchRG, wModule->searchGR, wModule->searchBY);
    //printf("r:%f;g:%f;b:%f     RG:%f;GR:%f;BY:%f",wModule->targetRED,wModule->targetGREEN,wModule->targetBLUE, wModule->searchRG, wModule->searchGR, wModule->searchBY);

    gtk_statusbar_push (statusbar, 0, msg);

    g_free (msg);
}


static gint timeout_CB (gpointer data){
    if (getImage()){
            //             int imageWidth, imageHeight, pixbufWidth, pixbufHeight;
            //             _semaphore.wait();
            //            imageWidth = _inputImg.width();
            //            imageHeight = _inputImg.height();
            //            _semaphore.post();
            //            pixbufWidth = gdk_pixbuf_get_width(frame);
            //            pixbufHeight = gdk_pixbuf_get_height(frame);
            //            if ( (imageWidth != pixbufWidth) || (imageHeight != pixbufHeight) )
            //                            {
            //                    g_object_unref(frame);
            //                    frame = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, imageWidth, imageHeight);
            //            }
            //            _frameN++;
            gtk_widget_queue_draw (da);
            //            if (_savingSet)
            //                saveCurrentFrame();
            if(!wModule->resized_flag)
                wModule->resizeImages(_inputImg.width(),_inputImg.height());
            wModule->inputImage_flag=true;
    }
    updateStatusbar(GTK_STATUSBAR (statusbar));
    if(true)
        wModule->outPorts();
    return TRUE;
}






//-------------------------------------------------
// Main Window Menubar
//-------------------------------------------------
GtkWidget* WatershedModule::createMenubar(void)
{
    GtkWidget *menubar;

    menubar =  gtk_menu_bar_new ();
    GtkWidget *menuSeparator;	
    // Submenus Items on menubar
    fileItem = gtk_menu_item_new_with_label ("File");
    imageItem = gtk_menu_item_new_with_label ("Image");
    helpItem = gtk_menu_item_new_with_label ("Help");
    // Submenu: File 
    fileMenu = gtk_menu_new();
    fileSingleItem = gtk_check_menu_item_new_with_label ("Save single image..");
    gtk_menu_append( GTK_MENU(fileMenu), fileSingleItem);
    gtk_signal_connect( GTK_OBJECT(fileSingleItem), "toggled", GTK_SIGNAL_FUNC(menuFileSingle_CB), mainWindow);
    fileSetItem = gtk_check_menu_item_new_with_label ("Save a set of images..");
    gtk_menu_append( GTK_MENU(fileMenu), fileSetItem);
//    gtk_signal_connect( GTK_OBJECT(fileSetItem), "toggled", GTK_SIGNAL_FUNC(menuFileSet_CB), mainWindow);
    menuSeparator = gtk_separator_menu_item_new();
    gtk_menu_append( GTK_MENU(fileMenu), menuSeparator);
    fileQuitItem = gtk_menu_item_new_with_label ("Quit");
    gtk_menu_append( GTK_MENU(fileMenu), fileQuitItem);
    gtk_signal_connect( GTK_OBJECT(fileQuitItem), "activate", GTK_SIGNAL_FUNC(menuFileQuit_CB), mainWindow);
    // Submenu: Image  
    /*imageMenu = gtk_menu_new();
    imageSizeItem = gtk_menu_item_new_with_label ("Original size");
    gtk_menu_append( GTK_MENU(imageMenu), imageSizeItem);
    gtk_signal_connect( GTK_OBJECT(imageSizeItem), "activate", GTK_SIGNAL_FUNC(menuImageSize_CB), mainWindow);
    imageRatioItem = gtk_menu_item_new_with_label ("Original aspect ratio");
    gtk_menu_append( GTK_MENU(imageMenu), imageRatioItem);
    gtk_signal_connect( GTK_OBJECT(imageRatioItem), "activate", GTK_SIGNAL_FUNC(menuImageRatio_CB), mainWindow);
    menuSeparator = gtk_separator_menu_item_new();
    gtk_menu_append( GTK_MENU(imageMenu), menuSeparator);
    imageFreezeItem = gtk_check_menu_item_new_with_label ("Freeze");
    gtk_menu_append( GTK_MENU(imageMenu), imageFreezeItem);
    gtk_signal_connect( GTK_OBJECT(imageFreezeItem), "toggled", GTK_SIGNAL_FUNC(menuImageFreeze_CB), mainWindow);
    menuSeparator = gtk_separator_menu_item_new();
    gtk_menu_append( GTK_MENU(imageMenu), menuSeparator);
    imageFramerateItem = gtk_menu_item_new_with_label ("Change refresh interval..");
    gtk_menu_append( GTK_MENU(imageMenu), imageFramerateItem);
    gtk_signal_connect( GTK_OBJECT(imageFramerateItem), "activate", GTK_SIGNAL_FUNC(menuImageFramerate_CB), mainWindow);
    imageIntervalItem = gtk_menu_item_new_with_label ("Show Interval..");
    gtk_menu_append( GTK_MENU(imageMenu), imageIntervalItem);
    gtk_signal_connect( GTK_OBJECT(imageIntervalItem), "activate", GTK_SIGNAL_FUNC(menuImageInterval_CB), mainWindow);*/
    // Submenu: Help
    /*helpMenu = gtk_menu_new();	
    helpAboutItem = gtk_menu_item_new_with_label ("About..");
    gtk_menu_append( GTK_MENU(helpMenu), helpAboutItem);
    gtk_signal_connect( GTK_OBJECT(helpAboutItem), "activate", GTK_SIGNAL_FUNC(menuHelpAbout_CB), mainWindow);*/

    // linking the submenus to items on menubar
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileItem), fileMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(imageItem), imageMenu);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpItem), helpMenu);
    // appending the submenus to the menubar
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), fileItem);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), imageItem);
    gtk_menu_item_set_right_justified (GTK_MENU_ITEM (helpItem), TRUE);
    gtk_menu_bar_append(GTK_MENU_BAR(menubar), helpItem);
  
    return menubar;
}

//-------------------------------------------------
// Service Fuctions
//-------------------------------------------------



void WatershedModule::createObjects() {
    /*ptr_imgRecv = new YARPImgRecv;
    ptr_imgRecvRed = new YARPImgRecv;
    ptr_imgRecvGreen = new YARPImgRecv;
    ptr_imgRecvBlue = new YARPImgRecv;
    ptr_imgRecvRG = new YARPImgRecv;
    ptr_imgRecvGR = new YARPImgRecv;
    ptr_imgRecvBY = new YARPImgRecv;*/

    

    ptr_inputImg = new yarp::sig::ImageOf<yarp::sig::PixelRgb>;
    ptr_inputImgRed = new yarp::sig::ImageOf<yarp::sig::PixelMono>;
    ptr_inputImgGreen = new yarp::sig::ImageOf<yarp::sig::PixelMono>;
    ptr_inputImgBlue = new yarp::sig::ImageOf<yarp::sig::PixelMono>;
    ptr_inputImgRG = new yarp::sig::ImageOf<yarp::sig::PixelMono>;
    ptr_inputImgGR = new yarp::sig::ImageOf<yarp::sig::PixelMono>;
    ptr_inputImgBY = new yarp::sig::ImageOf<yarp::sig::PixelMono>;
    
    ptr_semaphore = new yarp::os::Semaphore;
}

void WatershedModule::setName(ConstString param){
   name=param;
}

ConstString WatershedModule::getName(){
    return name;
}

ConstString WatershedModule::getName(ConstString suffix){
    std::string namestr(name.c_str());
    namestr.append("/");
    namestr.append(suffix.c_str());
    ConstString ret(namestr.c_str());
    return ret;
}

bool WatershedModule::openPorts(){
    bool ret = false;
    
    /*
    printf("Registering port %s on network %s...\n",  getName("image:i").c_str(),"default");
    ret = _imgRecv.Connect(getName("image:i").c_str(),"default");
    if (ret == true)
        {
            printf("Port registration succeed!\n");
        }
    else
        {
            printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
            return false;
        }
    //--------
    ret = _imgRecvRed.Connect((char*) getName("inRed:i").c_str(),"default");
    if (ret == true)
        {
            //reduce verbosity --paulfitz
            printf("Port registration succeed!\n");
        }
    else
        {
            printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
            return false;
        }
    ret = _imgRecvGreen.Connect((char*) getName("inGreen:i").c_str(),"default");
    if (ret == true)
        {
            //reduce verbosity --paulfitz
            printf("Port registration succeed!\n");
        }
    else
        {
            printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
            return false;
        }
    ret = _imgRecvBlue.Connect((char*) getName("inBlue:i").c_str(),"default");
    if (ret == true)
        {
            //reduce verbosity --paulfitz
            printf("Port registration succeed!\n");
        }
    else
        {
            printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
            return false;
        }
    //--------
    ret = _imgRecvRG.Connect((char*) getName("inRG:i").c_str(),"default");
    if (ret == true)
        {
            //reduce verbosity --paulfitz
            printf("Port registration succeed!\n");
        }
    else
        {
            printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
            return false;
        }
    ret = _imgRecvGR.Connect((char*) getName("inGR:i").c_str(),"default");
    if (ret == true)
        {
            //reduce verbosity --paulfitz
            printf("Port registration succeed!\n");
        }
    else
        {
            printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
            return false;
        }
    ret = _imgRecvBY.Connect((char*) getName("inBY:i").c_str(),"default");
    if (ret == true)
        {
            //reduce verbosity --paulfitz
            printf("Port registration succeed!\n");
        }
    else
        {
            printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
            return false;
        }
        */
    //-------------
    if (true)
        {		
            _pOutPort = new yarp::os::BufferedPort<yarp::os::Bottle>;
            //printf("Registering port %s on network %s...\n", getName("outputImage:o"),"default");
            bool ok = _pOutPort->open( getName("outputImage:o").c_str());
            if  (ok)
                printf("Port registration succeed!\n");
            else 
                {
                    printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
                    return false;
                }
            _pOutPort2 = new yarp::os::BufferedPort<ImageOf<PixelRgb> >;
            //printf("Registering port %s on network %s...\n", getName("outBlobs:o"),"default");
            ok = _pOutPort2->open( getName("outBlobs:o").c_str());
            if  (ok)
                printf("Port registration succeed!\n");
            else 
                {
                    printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
                    return false;
                }
            _pOutPort3 = new yarp::os::BufferedPort<ImageOf<PixelRgb> >;
            //printf("Registering port %s on network %s...\n", getName("outView:o"),"default");
            ok = _pOutPort3->open( getName("outView:o").c_str());
            if  (ok)
                printf("Port registration succeed!\n");
            else 
                {
                    printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
                    return false;
                }
            commandPort = new yarp::os::BufferedPort<Bottle >;
            //printf("Registering port %s on network %s...\n", getName("command:o"),"default");
            ok = commandPort->open( getName("command:o").c_str());
            if  (ok)
                printf("Port registration succeed!\n");
            else 
                {
                    printf("ERROR: Port registration failed.\nQuitting, sorry.\n");
                    return false;
                }
        }

    return true;
}

bool WatershedModule::closePorts(){
    bool ret = false;
    //int res = 0;
    // Registering Port(s)
    //reduce verbosity --paulfitz
    //printf("Closing port %s on network %s...\n", getName("in"),"default");
    //ret = _imgRecv.Disconnect();
    //--------
    //ret = _imgRecvRed.Disconnect(); //("/rea/Watershed/inRed","default");
    //ret = _imgRecvGreen.Disconnect();//("/rea/Watershed/inGreen","default");
    //ret = _imgRecvBlue.Disconnect(); //("/rea/Watershed/inBlue","default");
    //--------
    //ret = _imgRecvRG.Disconnect();//("/rea/Watershed/inRG","default");
    //ret = _imgRecvGR.Disconnect();//("/rea/Watershed/inGR","default");
    //ret = _imgRecvBY.Disconnect();//("/rea/Watershed/inBY","default");
    //-------------
    if (true)
        {		
            _pOutPort->close();
            //printf("Closing port %s on network %s...\n", getName("out"),"default");
            _pOutPort2->close();
            //printf("Closing port %s on network %s...\n", getName("outBlobs:o"),"default");
            _pOutPort3->close();
            //printf("Closing port %s on network %s...\n", getName("outView:o"),"default");
            commandPort->close();
            //printf("Closing port %s on network %s...\n", getName("centroid:o"),"default");
        }

    return true;
}


void WatershedModule::setUp()
{
    /*if (true)
        _imgRecv.SetLogopolar(false);
    else
        _imgRecv.SetLogopolar(true);
    
    if (true)
        _imgRecv.SetFovea(false);
    else
        _imgRecv.SetFovea(true);
        */
    
    if (openPorts() == false)
        return;
    
}

static void scale_set_default_values( GtkScale *scale )
{
    gtk_range_set_update_policy (GTK_RANGE (scale),
                                 GTK_UPDATE_CONTINUOUS);
    gtk_scale_set_digits (scale, 0);
    gtk_scale_set_value_pos (scale, GTK_POS_TOP);
    gtk_scale_set_draw_value (scale, TRUE);
}


static GtkWidget *xpm_label_box( gchar     *xpm_filename,
                                 gchar     *label_text )
{
    GtkWidget *box;
    GtkWidget *label;
    GtkWidget *image;

    /* Create box for image and label */
    box = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box), 2);

    /* Now on to the image stuff */
    if(xpm_filename!=NULL)
        image = gtk_image_new_from_file (xpm_filename);

    /* Create a label for the button */
    label = gtk_label_new (label_text);

    /* Pack the image and label into the box */
    if(xpm_filename!=NULL)
        gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 3);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 3);

    if(xpm_filename!=NULL)
        gtk_widget_show (image);
    gtk_widget_show (label);

    return box;
}

//-------------------------------------------------
// Main Window 
//-------------------------------------------------
GtkWidget* WatershedModule::createMainWindow(void)
{
    //Module=this; //it is necessary to synchronise the static function with this class
    
    GtkRequisition actualSize;
    GtkWidget* window;
    GtkWidget *label;
    GtkWidget *separator;
    
    //gtk_init (&argc, &argv);
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "saliencyBlobFinderInterface");
    gtk_window_set_default_size(GTK_WINDOW (window), 320, 500); 
    gtk_window_set_resizable (GTK_WINDOW (window), TRUE);
    g_signal_connect (G_OBJECT (window), "destroy",
                      G_CALLBACK (cleanExit),
                      NULL);

    
    // When the window is given the "delete_event" signal (this is given
    // by the window manager, usually by the "close" option, or on the
    // titlebar), we ask it to call the delete_event () function
    // as defined above. The data passed to the callback
    // function is NULL and is ignored in the callback function.
    //g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (delete_event), NULL);

    // Box for main window in a ordered list
    GtkWidget *box,*box2,*boxA,*box3,*box4;
    box = gtk_vbox_new (FALSE, 0); // parameters (gboolean homogeneous_space, gint spacing);
    gtk_container_add (GTK_CONTAINER (window), box);
    // MenuBar for main window
    menubar = createMenubar();
    gtk_box_pack_start (GTK_BOX (box), menubar, FALSE, TRUE, 0); // parameters (GtkBox *box, GtkWidget *child, gboolean expand, gboolean fill, guint padding);
    //gtk_widget_size_request(menubar, &actualSize);
    
    // Drawing Area : here the image will be drawed
    //da = gtk_drawing_area_new ();
    //g_signal_connect (da, "expose_event", G_CALLBACK (expose_CB), NULL);
    /*if (_options.outputEnabled == 1)
        {
            g_signal_connect (da, "button_press_event", G_CALLBACK (clickDA_CB), NULL);
            // Ask to receive events the drawing area doesn't normally subscribe to
            gtk_widget_set_events (da, gtk_widget_get_events (da) | GDK_BUTTON_PRESS_MASK);
        }*/
    //gtk_box_pack_start(GTK_BOX(box), da, TRUE, TRUE, 0);

    //Toolbox area
    //creates the area as collection of port processes sequence
    box2 = gtk_vbox_new (FALSE, 0); // parameters (gboolean homogeneous_space, gint spacing);
    gtk_container_add (GTK_CONTAINER (window), box2);
    GtkWidget *button,*button2,*buttonCheck;
    GtkWidget *boxButton,*boxButton2;
    GtkWidget *boxButtons;
    GtkWidget *boxSliders;
    boxButtons = gtk_hbox_new (FALSE, 0); // parameters (gboolean homogeneous_space, gint spacing);
    gtk_container_set_border_width (GTK_CONTAINER (boxButtons), 0);
    boxSliders = gtk_hbox_new (TRUE, 0); // parameters (gboolean homogeneous_space, gint spacing);
    gtk_container_set_border_width (GTK_CONTAINER (boxSliders), 0);
     /* Create a new button */
    button = gtk_button_new ();
    button2 = gtk_button_new ();
    /* Connect the "clicked" signal of the button to our callback */
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "output1");
    g_signal_connect (G_OBJECT (button2), "clicked",G_CALLBACK (callback), (gpointer) "output2");
    /* This calls our box creating func tion */
    
    boxButton = xpm_label_box (NULL,(gchar*)"output1");
    boxButton2= xpm_label_box (NULL, (gchar*)"output2");
    /* Pack and show all our widgets */
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_widget_show (boxButton2);
    gtk_container_add (GTK_CONTAINER (button2), boxButton2);
    gtk_widget_show (button2);
    //gtk_container_add (GTK_CONTAINER (boxButtons), button);
    //gtk_container_add (GTK_CONTAINER (boxButtons), button2);


    /*
    label = gtk_label_new ("CheckList:");
    gtk_box_pack_start (GTK_BOX (boxButtons), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    // A checkbutton to control whether the value is displayed or not
    buttonCheckGreen = gtk_check_button_new_with_label("<--GreenPlane");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheckGreen), FALSE);
    g_signal_connect (G_OBJECT (buttonCheckGreen), "toggled",G_CALLBACK (cb_draw_value), (gpointer) "GreenPlane");
    gtk_box_pack_start (GTK_BOX (boxButtons), buttonCheckGreen, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheckGreen);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("<--RedPlane");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), (gpointer) "RedPlane");
    gtk_box_pack_start (GTK_BOX (boxButtons), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("<--BluePlane");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), (gpointer) "BluePlane");
    gtk_box_pack_start (GTK_BOX (boxButtons), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("<--RGImage");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "<--RGImage");
    gtk_box_pack_start (GTK_BOX (boxButtons), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("<--GRImage");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "<--GRImage");
    gtk_box_pack_start (GTK_BOX (boxButtons), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("<--BYImage");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), (gpointer) "<--BYImage");
    gtk_box_pack_start (GTK_BOX (boxButtons), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);
    
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("<--WatershedRain");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), (gpointer) "<--WatershedRain");
    gtk_box_pack_start (GTK_BOX (boxButtons), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("<--MeanColorsImage");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "<--MeanColorsImage");
    gtk_box_pack_start (GTK_BOX (boxButtons), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("<--Blue1");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), (gpointer) "<--Blue1");
    gtk_box_pack_start (GTK_BOX (boxButtons), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);
    
    gtk_container_add (GTK_CONTAINER (box2), boxButtons);
    */
    
    //---- vSeparator
    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 3);
    gtk_widget_show (separator);

    
    //-----main section
    GtkWidget *scrollbar;
    
    
    GtkWidget *scale;
    GtkObject *adj1, *adj2,*adj3, *adj4,*adjr, *adjg, *adjb, *adjtime;
    GtkWidget *hscale, *vscale;


    adj1 = gtk_adjustment_new (0.0, 0.0, 101.0, 0.1, 1.0, 1.0);
    vscale = gtk_vscale_new (GTK_ADJUSTMENT (adj1));
    scale_set_default_values (GTK_SCALE (vscale));
    gtk_box_pack_start (GTK_BOX (boxSliders), vscale, TRUE, TRUE, 0);
    gtk_widget_show (vscale);

    /*separator = gtk_vseparator_new ();
    gtk_box_pack_start (GTK_BOX (boxSliders), separator, FALSE, FALSE, 0);
    gtk_widget_show (separator);*/

    

    //----------BOXA SECTION:1
    //boxA is the area that contains the two subsection for watershed and saliency operators
    boxA = gtk_hbox_new (FALSE, 0);
    
    gtk_container_set_border_width (GTK_CONTAINER (boxA), 0);
    gtk_box_pack_start (GTK_BOX (box2), boxA, TRUE, TRUE, 0);
    gtk_widget_show (boxA);
    
    //---- vSeparator
    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (boxA), separator, FALSE, TRUE, 3);
    gtk_widget_show (separator);
    
    //--box3 section A
    box3 = gtk_hbox_new (FALSE, 0);
    /*hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj1));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box3), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);*/

    label = gtk_label_new ("Options:");
    gtk_box_pack_start (GTK_BOX (box3), label, FALSE, FALSE, 0);
    gtk_widget_show (label);


    scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj1));
    /* Notice how this causes the scales to always be updated
     * continuously when the scrollbar is moved */
    /*gtk_range_set_update_policy (GTK_RANGE (scrollbar), 
                                 GTK_UPDATE_CONTINUOUS);
    gtk_box_pack_start (GTK_BOX (box3), scrollbar, TRUE, TRUE, 0);
    gtk_widget_show (scrollbar);*/

    //-----Check Buttons
    //box4=  gtk_vbox_new (FALSE, 0);
    
    //gtk_container_set_border_width (GTK_CONTAINER (box3), 0);
    //gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
    //gtk_widget_show (box3);

    box4 = gtk_vbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box4), 0);

    label = gtk_label_new ("BOTTOM-UP:saliency linear combination Kcoeff.: isolated blobs are salient");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    double maxAdj=100;
    double minAdj=1;
    double stepAdj=1;
    
    adj1 = gtk_adjustment_new(50, minAdj,maxAdj,stepAdj, 1, 1);
    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj1));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box4), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);
    g_signal_connect (G_OBJECT (adj1), "value_changed",
                      G_CALLBACK (cb_digits_scale), NULL);


    label = gtk_label_new ("TOP-DOWN:saliency linear combination Kcoeff.: match-colour blob are salient");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    adj2 = gtk_adjustment_new (50, minAdj,maxAdj,stepAdj, 1, 1);
    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj2));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box4), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);
    g_signal_connect (G_OBJECT (adj2), "value_changed",
                      G_CALLBACK (cb_digits_scale2), NULL);
    
    label = gtk_label_new ("MAXBLOB dimension: cut off bigger blobs");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    adj3 = gtk_adjustment_new (4096, 10,6000,100, 1, 1);
    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj3));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box4), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);
    g_signal_connect (G_OBJECT (adj3), "value_changed",
                      G_CALLBACK (cb_digits_scale3), NULL);

    
    label = gtk_label_new ("MINBLOB dimension: cut off smaller blobs");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    adj4 = gtk_adjustment_new (100,1,3000,1,1,1);
    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj4));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box4), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);
    g_signal_connect (G_OBJECT (adj4), "value_changed",
                      G_CALLBACK (cb_digits_scale4), NULL);
    
    label = gtk_label_new ("red intensity target:");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    adjr = gtk_adjustment_new (1,1,255,1,1,1);
    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adjr));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box4), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);
    g_signal_connect (G_OBJECT (adjr), "value_changed",
                      G_CALLBACK (cb_digits_scaler), NULL);

    label = gtk_label_new ("green intensity target:");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    adjg = gtk_adjustment_new (1,1,255,1,1,1);
    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adjg));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box4), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);
    g_signal_connect (G_OBJECT (adjg), "value_changed",
                      G_CALLBACK (cb_digits_scaleg), NULL);

    label = gtk_label_new ("blue intensity target:");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    adjb = gtk_adjustment_new (1,1,255,1,1,1);
    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adjb));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box4), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);
    g_signal_connect (G_OBJECT (adjb), "value_changed",
                      G_CALLBACK (cb_digits_scaleb), NULL);
    
    label = gtk_label_new ("time decimal constant1 (x y z):");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    adjtime = gtk_adjustment_new (10,1,100,1,1,1);
    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adjtime));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box4), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);
    g_signal_connect (G_OBJECT (adjtime), "value_changed",
                      G_CALLBACK (cb_digits_scaletime), NULL);


    label = gtk_label_new ("time decimal constant2 (set img x y):");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
    adjtime = gtk_adjustment_new (10,1,100,1,1,1);
    hscale = gtk_hscale_new (GTK_ADJUSTMENT (adjtime));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box4), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);
    g_signal_connect (G_OBJECT (adjtime), "value_changed",
                      G_CALLBACK (cb_digits_scaletime2), NULL);

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);

    //-----box4
    box4=  gtk_vbox_new (FALSE, 0);

    // A checkbutton to control whether the value is displayed or not
    /*
    buttonCheck = gtk_check_button_new_with_label("Option1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), (gpointer)"ColourOpponency11");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);
    */

    // A checkbutton to control whether the value is displayed or not
    
    /* buttonCheck = gtk_check_button_new_with_label("Option2-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "FindEdges1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);*/

    // A checkbutton to control whether the value is displayed or not
    /*buttonCheck = gtk_check_button_new_with_label("Option3-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Normalize1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);*/

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);
    //---box 4


    /*
    //-------run button
    button = gtk_button_new ();
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "Rain1");
    boxButton = xpm_label_box (NULL, (gchar*)"Rain1");
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);
    */

    gtk_container_set_border_width (GTK_CONTAINER (box3), 0);
    gtk_box_pack_start (GTK_BOX (boxA), box3, TRUE, TRUE, 0);
    gtk_widget_show (box3);

    //---- vSeparator
    separator = gtk_vseparator_new ();
    gtk_box_pack_start (GTK_BOX (box3), separator, FALSE, TRUE, 3);
    gtk_widget_show (separator);
    
    //--box3 section B
    //box3 = gtk_hbox_new (FALSE, 0);
    /*hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj1));
    gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    scale_set_default_values (GTK_SCALE (hscale));
    gtk_box_pack_start (GTK_BOX (box3), hscale, TRUE, TRUE, 0);
    gtk_widget_show (hscale);*/

    //label = gtk_label_new ("choose output:");
    //gtk_box_pack_start (GTK_BOX (box3), label, FALSE, FALSE, 0);
    //gtk_widget_show (label);


    scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj1));
    /* Notice how this causes the scales to always be updated
     * continuously when the scrollbar is moved */
    /*gtk_range_set_update_policy (GTK_RANGE (scrollbar), 
                                 GTK_UPDATE_CONTINUOUS);
    gtk_box_pack_start (GTK_BOX (box3), scrollbar, TRUE, TRUE, 0);
    gtk_widget_show (scrollbar);*/

    //-----Check Buttons
    box4=  gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);

    //-----box4
    box4=  gtk_vbox_new (FALSE, 0);

    label = gtk_label_new ("1channel output:");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    GtkWidget *buttonradio;
    GSList *group;

    buttonradio = gtk_radio_button_new_with_label (NULL, "Watershed-->");
    g_signal_connect (G_OBJECT (buttonradio), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Watershed");
    gtk_box_pack_start (GTK_BOX (box4), buttonradio, TRUE, TRUE, 0);
    gtk_widget_show (buttonradio);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (buttonradio));

    buttonradio = gtk_radio_button_new_with_label(group, "Tagged-->");
    g_signal_connect (G_OBJECT (buttonradio), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Tagged");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonradio), TRUE);
    gtk_box_pack_start (GTK_BOX (box4), buttonradio, TRUE, TRUE, 0);
    gtk_widget_show (buttonradio);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (buttonradio));
    buttonradio = gtk_radio_button_new_with_label(group, "ContrastLP-->");
    g_signal_connect (G_OBJECT (buttonradio), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "ContrastLP");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonradio), TRUE);
    gtk_box_pack_start (GTK_BOX (box4), buttonradio, TRUE, TRUE, 0);
    gtk_widget_show (buttonradio);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (buttonradio));
    buttonradio = gtk_radio_button_new_with_label(group, "FoveaBlob-->");
    g_signal_connect (G_OBJECT (buttonradio), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "FoveaBlob");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonradio), TRUE);
    gtk_box_pack_start (GTK_BOX (box4), buttonradio, TRUE, TRUE, 0);
    gtk_widget_show (buttonradio);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (buttonradio));
    buttonradio = gtk_radio_button_new_with_label(group, "MaxSaliencyBlob-->");
    g_signal_connect (G_OBJECT (buttonradio), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "MaxSaliencyBlob");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonradio), TRUE);
    gtk_box_pack_start (GTK_BOX (box4), buttonradio, TRUE, TRUE, 0);
    gtk_widget_show (buttonradio);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (buttonradio));

/*
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Watershed-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Watershed");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);
    
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Tagged-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Tagged");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("MeanColoursLP-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), TRUE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "MeanColoursLP");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("FoveaBlob-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "FoveaBlob");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);


    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("ContrastLP-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "ContrastLP");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("MaxSaliencyBlob-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "MaxSaliencyBlob");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);
    */


    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);
    //---box 4


    box4=  gtk_vbox_new (FALSE, 0);

    label = gtk_label_new ("3channels output:");
    gtk_box_pack_start (GTK_BOX (box4), label, FALSE, FALSE, 0);
    gtk_widget_show (label);    

    //GtkWidget *buttonradio;
    //GSList *group;

    buttonradio = gtk_radio_button_new_with_label (group, "MeanColoursLP-->");
    g_signal_connect (G_OBJECT (buttonradio), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "MeanColourLP");
    gtk_box_pack_start (GTK_BOX (box4), buttonradio, TRUE, TRUE, 0);
    gtk_widget_show (buttonradio);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (buttonradio));

    buttonradio = gtk_radio_button_new_with_label(group, "none-->");
    g_signal_connect (G_OBJECT (buttonradio), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "none");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonradio), TRUE);
    gtk_box_pack_start (GTK_BOX (box4), buttonradio, TRUE, TRUE, 0);
    gtk_widget_show (buttonradio);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (buttonradio));

    
    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);
    //---box 4

/*
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Watershed-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Watershed");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);
    
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Tagged-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Tagged");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    //-------run button
    
    /*
    button = gtk_button_new ();
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "DrawAllBlobs1");
    boxButton = xpm_label_box (NULL, (gchar*)"DrawAllBlobs1");
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);
    */

    /*
    button = gtk_button_new ();
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "drawFoveaBlob1");
    boxButton = xpm_label_box (NULL, (gchar*)"drawFoveaBlob1");
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);
    */

    /*
    button = gtk_button_new ();
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "drawVQColor1");
    boxButton = xpm_label_box (NULL, (gchar*)"drawVQColor1");
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);
    */

    /*
    button = gtk_button_new ();
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "maxSalienceBlob1");
    boxButton = xpm_label_box (NULL,(gchar*) "maxSalienceBlob1");
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);
    */

    //------ HSEPARATOR ---------------
    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    //----------BOXA SECTION:2
    //boxA is the area that contains the two subsection for watershed and saliency operators
    /*boxA = gtk_hbox_new (FALSE, 0);
    
    gtk_container_set_border_width (GTK_CONTAINER (boxA), 0);
    gtk_box_pack_start (GTK_BOX (box2), boxA, TRUE, TRUE, 0);
    gtk_widget_show (boxA);
    gtk_container_set_border_width (GTK_CONTAINER (box3), 0);
    gtk_box_pack_start (GTK_BOX (boxA), box3, TRUE, TRUE, 0);
    gtk_widget_show (box3);
    
    //--box3 section A
    box3 = gtk_hbox_new (FALSE, 0);
    //hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj1));
    //gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    //scale_set_default_values (GTK_SCALE (hscale));
    //gtk_box_pack_start (GTK_BOX (box3), hscale, TRUE, TRUE, 0);
    //gtk_widget_show (hscale);

    label = gtk_label_new ("Options:");
    gtk_box_pack_start (GTK_BOX (box3), label, FALSE, FALSE, 0);
    gtk_widget_show (label);


    scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj1));
    
    //gtk_range_set_update_policy (GTK_RANGE (scrollbar), 
                                 GTK_UPDATE_CONTINUOUS);
    //gtk_box_pack_start (GTK_BOX (box3), scrollbar, TRUE, TRUE, 0);
    //gtk_widget_show (scrollbar);

    //-----Check Buttons
    box4=  gtk_vbox_new (FALSE, 0);
    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Green1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Green1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Red1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), TRUE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Red1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Blue1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Blue1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);

    //-----box4
    box4=  gtk_vbox_new (FALSE, 0);
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("ColourOpponency1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "ColourOpponency11");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("MeanColoursLP1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "MeanColoursLP1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Normalize1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Normalize1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);
    //---box 4

    //-------run button
    button = gtk_button_new ();
    // Connect the "clicked" signal of the button to our callback 
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "Rain2");
    // This calls our box creating func tion 
    boxButton = xpm_label_box (NULL, "Rain2");
    // Pack and show all our widgets 
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);

    gtk_container_set_border_width (GTK_CONTAINER (box3), 0);
    gtk_box_pack_start (GTK_BOX (boxA), box3, TRUE, TRUE, 0);
    gtk_widget_show (box3);

    //---- vSeparator
    separator = gtk_vseparator_new ();
    gtk_box_pack_start (GTK_BOX (box3), separator, FALSE, TRUE, 3);
    gtk_widget_show (separator);
    
    //--box3 section B
    //box3 = gtk_hbox_new (FALSE, 0);
    //hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj1));
    //gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    //scale_set_default_values (GTK_SCALE (hscale));
    //gtk_box_pack_start (GTK_BOX (box3), hscale, TRUE, TRUE, 0);
    //gtk_widget_show (hscale);

    label = gtk_label_new ("Options:");
    gtk_box_pack_start (GTK_BOX (box3), label, FALSE, FALSE, 0);
    gtk_widget_show (label);


    scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj1));
    
    //gtk_range_set_update_policy (GTK_RANGE (scrollbar), 
                                 GTK_UPDATE_CONTINUOUS);
    gtk_box_pack_start (GTK_BOX (box3), scrollbar, TRUE, TRUE, 0);
    gtk_widget_show (scrollbar);

    //-----Check Buttons
    box4=  gtk_vbox_new (FALSE, 0);
    
    /*-------------
    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Green1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Green1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Red1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), TRUE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Red1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Blue1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Blue1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Green1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Green1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Red1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), TRUE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Red1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Blue1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Blue1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Green1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Green1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Red1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), TRUE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Red1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Blue1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Blue1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);
    ---------------*/

    /*

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);

    //-----box4
    box4=  gtk_vbox_new (FALSE, 0);
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("ContrastLP2-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "ContrastLP2");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("MeanColoursLP2-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "MeanColoursLP2");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Normalize1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Normalize1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);
    //---box 4

    button = gtk_button_new ();
    // Connect the "clicked" signal of the button to our callback 
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "DrawAllBlobs2");
    // This calls our box creating func tion 
    boxButton = xpm_label_box (NULL, "DrawAllBlobs2");
    // Pack and show all our widgets 
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);

    button = gtk_button_new ();
    // Connect the "clicked" signal of the button to our callback 
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "drawFoveaBlob2");
    // This calls our box creating func tion 
    boxButton = xpm_label_box (NULL, "drawFoveaBlob2");
    // Pack and show all our widgets 
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);

    button = gtk_button_new ();
    // Connect the "clicked" signal of the button to our callback 
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "drawVQColor2");
    // This calls our box creating func tion 
    boxButton = xpm_label_box (NULL, "drawVQColor2");
    // Pack and show all our widgets 
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);

    //------ HSEPARATOR ---------------
    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    //----------BOXA SECTION:3
    //boxA is the area that contains the two subsection for watershed and saliency operators
    boxA = gtk_hbox_new (FALSE, 0);
    box3 = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (boxA), 0);
    gtk_box_pack_start (GTK_BOX (box2), boxA, TRUE, TRUE, 0);
    gtk_widget_show (boxA);
    gtk_container_set_border_width (GTK_CONTAINER (box3), 0);
    gtk_box_pack_start (GTK_BOX (boxA), box3, TRUE, TRUE, 0);
    gtk_widget_show (box3);
    
    //--box3 section A
    //hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj1));
    // gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    // scale_set_default_values (GTK_SCALE (hscale));
    //gtk_box_pack_start (GTK_BOX (box3), hscale, TRUE, TRUE, 0);
    //gtk_widget_show (hscale);

    label = gtk_label_new ("Options:");
    gtk_box_pack_start (GTK_BOX (box3), label, FALSE, FALSE, 0);
    gtk_widget_show (label);


    scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj1));
    
    //gtk_range_set_update_policy (GTK_RANGE (scrollbar), 
                                 GTK_UPDATE_CONTINUOUS);
    //gtk_box_pack_start (GTK_BOX (box3), scrollbar, TRUE, TRUE, 0);
    //gtk_widget_show (scrollbar);

    //-----Check Buttons
    box4=  gtk_vbox_new (FALSE, 0);
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Green1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Green1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Red1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), TRUE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Red1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Blue1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Blue1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);

    //-----box4
    box4=  gtk_vbox_new (FALSE, 0);
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("ColourOpponency1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "ColourOpponency11");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("FindEdges1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "FindEdges1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Normalize1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Normalize1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);
    //---box 4

    //-------run button
    button = gtk_button_new ();
    // Connect the "clicked" signal of the button to our callback 
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "Rain3");
    // This calls our box creating func tion 
    boxButton = xpm_label_box (NULL, "Rain3");
    // Pack and show all our widgets 
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);

    gtk_container_set_border_width (GTK_CONTAINER (box3), 0);
    gtk_box_pack_start (GTK_BOX (boxA), box3, TRUE, TRUE, 0);
    gtk_widget_show (box3);

    //---- vSeparator
    separator = gtk_vseparator_new ();
    gtk_box_pack_start (GTK_BOX (box3), separator, FALSE, TRUE, 3);
    gtk_widget_show (separator);
    
    //--box3 section B
    // hscale = gtk_hscale_new (GTK_ADJUSTMENT (adj1));
    // gtk_widget_set_size_request (GTK_WIDGET (hscale), 200, -1);
    //scale_set_default_values (GTK_SCALE (hscale));
    //gtk_box_pack_start (GTK_BOX (box3), hscale, TRUE, TRUE, 0);
    //gtk_widget_show (hscale);

    label = gtk_label_new ("OCheckList:");
    gtk_box_pack_start (GTK_BOX (box3), label, FALSE, FALSE, 0);
    gtk_widget_show (label);


    scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (adj1));
    
    //gtk_range_set_update_policy (GTK_RANGE (scrollbar), 
                                 GTK_UPDATE_CONTINUOUS);
    //gtk_box_pack_start (GTK_BOX (box3), scrollbar, TRUE, TRUE, 0);
    //gtk_widget_show (scrollbar);

    //-----Check Buttons
    box4=  gtk_vbox_new (FALSE, 0);

    /*--------------------
    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Green1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Green1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Red1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), TRUE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Red1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Blue1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Blue1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Green1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Green1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Red1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), TRUE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Red1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Blue1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Blue1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Green1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Green1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Red1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), TRUE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Red1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not 
    buttonCheck = gtk_check_button_new_with_label("Blue1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value), "Blue1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    -----*/
    /*

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);

    //-----box4
    box4=  gtk_vbox_new (FALSE, 0);
    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("ContrastLP3-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "ContrastLP3");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("MeanColoursLP3-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "MeanColoursLP3");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    // A checkbutton to control whether the value is displayed or not
    buttonCheck = gtk_check_button_new_with_label("Normalize1-->");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (buttonCheck), FALSE);
    g_signal_connect (G_OBJECT (buttonCheck), "toggled",G_CALLBACK (cb_draw_value),(gpointer) "Normalize1");
    gtk_box_pack_start (GTK_BOX (box4), buttonCheck, TRUE, TRUE, 0);
    gtk_widget_show (buttonCheck);

    gtk_box_pack_start (GTK_BOX (box3), box4, TRUE, TRUE, 0);
    gtk_widget_show (box4);
    //---box 4

    //-------run button
    button = gtk_button_new ();
    // Connect the "clicked" signal of the button to our callback 
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "DrawAllBlobs3");
    // This calls our box creating func tion 
    boxButton = xpm_label_box (NULL, "DrawAllBlobs3");
    // Pack and show all our widgets 
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);

    button = gtk_button_new ();
    // Connect the "clicked" signal of the button to our callback 
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "drawFoveaBlob3");
    // This calls our box creating func tion 
    boxButton = xpm_label_box (NULL, "drawFoveaBlob3");
    // Pack and show all our widgets 
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);

    button = gtk_button_new ();
    // Connect the "clicked" signal of the button to our callback 
    g_signal_connect (G_OBJECT (button), "clicked",G_CALLBACK (callback), (gpointer) "drawVQColor3");
    // This calls our box creating func tion 
    boxButton = xpm_label_box (NULL, "drawVQColor3");
    // Pack and show all our widgets
    gtk_widget_show (boxButton);
    gtk_container_add (GTK_CONTAINER (button), boxButton);
    gtk_widget_show (button);
    gtk_box_pack_start (GTK_BOX (box3), button, TRUE, TRUE, 0);
    gtk_widget_show (button);*/

    //------ HSEPARATOR ---------------
    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (box2), separator, FALSE, TRUE, 0);
    gtk_widget_show (separator);

    //gtk_container_add (GTK_CONTAINER (box2), boxSliders);
    gtk_box_pack_start(GTK_BOX(box), box2,FALSE,FALSE, 10);
    // StatusBar for main window
    statusbar = gtk_statusbar_new ();
    updateStatusbar(GTK_STATUSBAR (statusbar));
    gtk_box_pack_start (GTK_BOX (box), statusbar, FALSE, TRUE, 0);
    gtk_widget_size_request(statusbar, &actualSize);
    //_occupiedHeight += 2*(actualSize.height);

    frame = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 320, 240);
    // TimeOut used to refresh the screen
    timeout_ID = gtk_timeout_add (100, timeout_CB, NULL);

    mainWindow=window;

    return window;
}



void WatershedModule::drawAllBlobs(bool stable)
{
    salience->ComputeSalienceAll(this->max_tag,this->max_tag);
    //extracts the PixelBgr color of a particular blob identified by the id (last parameter)
    PixelBgr varFoveaBlob = salience->varBlob(*tagged, *wModule->ptr_inputRG, *wModule->ptr_inputGR, *wModule->ptr_inputBY, 1);

    salience->drawFoveaBlob(*blobFov, *tagged);
    //__OLD//salience.drawBlobList(blobFov, tagged, blobList, max_tag, 127);
    
    //list of boolean whether is present or not a blob
    
    memset(blobList, 0, sizeof(char)*(max_tag+1));
    // - faster
    // - it considers also "lateral" pixels
    // - it doesn't add pixels iteratively
    wOperator->findNeighborhood(*tagged, 0, 0, blobList);
    salience->fuseFoveaBlob3(*tagged, blobList, varFoveaBlob,max_tag);

    // alternative method
    //__OLD//rain.fuseFoveaBlob(tagged, blobList, max_tag);
    
    //__OLD//blobList[1]=2; // so the fovea blob is eliminated by the removeBlobList
    //__OLD//salience.statBlobList(tagged, blobList, max_tag, fovBox);
    YARPBox fovBox;
    fovBox=salience->getBlobNum(1);
    //__OLD//salience.removeBlobList(blobList, max_tag);
    //----->salience->removeFoveaBlob(*tagged);
    //__OLD//salience.updateFoveaBlob(tagged, blobList, max_tag);

    if (stable) {
        for (int i=0; i<2; i++) {
            memset(blobList, 0, sizeof(char)*(max_tag+1));
            wOperator->findNeighborhood(*tagged, 0, 0, blobList);
            const int minBoundingArea=15*15;
            int count=salience->countSmallBlobs(*tagged, blobList, max_tag, minBoundingArea);
            printf("Count of small blobs: %d \n",count);
            blobList[1]=0;
            salience->mergeBlobs(*tagged, blobList, max_tag, 1);
        }

        /*__OLD//while (num!=0) {
            blobList[1]=0;
            salience.mergeBlobs(tagged, blobList, max_tag, 1);
            memset(blobList, 0, sizeof(char)*max_tag);
            rain.findNeighborhood(tagged, 0, 0, blobList);
            num = salience.checkSmallBlobs(tagged, blobList, max_tag, minBoundingArea);
        }*/
    }
        
    //__OLD//salience.drawFoveaBlob(blobFov, tagged);
    //__OLD//salience.drawBlobList(blobFov, tagged, blobList, max_tag, 127);
    
    // Comment the following line to disable the elimination of non valid blob
    //salience->RemoveNonValidNoRange(max_tag, BLOB_MAXSIZE, BLOB_MINSIZE);
    salience->RemoveNonValidNoRange(max_tag,maxBLOB,minBLOB);
    
    //__OLD//salience.DrawContrastLP(rg, gr, by, tmp1, tagged, max_tag, 0, 1, 30, 42, 45); // somma coeff pos=3 somma coeff neg=-3
    //__OLD//salience.checkIOR(tagged, IORBoxes, num_IORBoxes);
    //__OLD//salience.doIOR(tagged, IORBoxes, num_IORBoxes);
    //float salienceBU=1.0,salienceTD=0.0;
    IppiSize srcsize={this->width,this->height};
    PixelMono searchTD=0;
    searchRG=((targetRED-targetGREEN+255)/510)*255;
    searchGR=((targetGREEN-targetRED+255)/510)*255;
    PixelMono addRG=((targetRED+targetGREEN)/510)*255;
    searchBY=((targetBLUE-addRG+255)/510)*255;
    int psb32s,psb8u;
    Ipp32s* _inputImgRGS32=ippiMalloc_32s_C1(this->width,this->height,&psb32s);
    Ipp32s* _inputImgGRS32=ippiMalloc_32s_C1(this->width,this->height,&psb32s);
    Ipp32s* _inputImgBYS32=ippiMalloc_32s_C1(this->width,this->height,&psb32s);
    //Ipp8s* _inputImgRGS8s=ippiMalloc_8u_C1(width,height,&psb8u);
    //_inputImgGR
    if(ptr_inputImgRG!=NULL){
        //_inputImgRGS->copy(*ptr_inputImgRG,320,240);
        ippiScale_8u32s_C1R(_inputImgRG.getRawImage(),_inputImgRG.getRowSize(),_inputImgRGS32,psb32s,srcsize);
        //_inputImgRGS8s=(Ipp8s*)_inputImgRGS->getRawImage();
        ippiConvert_32s8s_C1R(_inputImgRGS32,psb32s,(Ipp8s*)_inputImgRGS->getRawImage(),_inputImgRGS->getRowSize(),srcsize);
        //ippiCopy_8u_C1R(_inputImgRG.getPixelAddress(0,0),320,_inputImgRGS->getPixelAddress(0,0),320,srcsize);
    }
    else
        return;
    //_inputImgGR
    if(ptr_inputImgGR!=NULL){
        //_inputImgGRS->copy(*ptr_inputImgGR,320,240);
        ippiScale_8u32s_C1R(_inputImgGR.getRawImage(),_inputImgGR.getRowSize(),_inputImgGRS32,psb32s,srcsize);
        ippiConvert_32s8s_C1R(_inputImgGRS32,psb32s,(Ipp8s*)_inputImgGRS->getRawImage(),_inputImgGRS->getRowSize(),srcsize);
        //ippiCopy_8u_C1R(_inputImgGR.getPixelAddress(0,0),320,_inputImgGRS->getPixelAddress(0,0),320,srcsize);
    }
    else
        return;
    //_inputImgBY
    if(ptr_inputImgBY!=NULL){
        //_inputImgBYS->copy(*ptr_inputImgBY,320,240);
        ippiScale_8u32s_C1R(_inputImgBY.getRawImage(),_inputImgBY.getRowSize(),_inputImgBYS32,psb32s,srcsize);
        ippiConvert_32s8s_C1R(_inputImgBYS32,psb32s,(Ipp8s*)_inputImgBYS->getRawImage(),_inputImgBYS->getRowSize(),srcsize);
        //ippiCopy_8u_C1R(_inputImgBY.getPixelAddress(0,0),320,_inputImgBYS->getPixelAddress(0,0),320,srcsize);
    }
    else
        return;
    int nBlobs=salience->DrawContrastLP2(*_inputImgGRS, *_inputImgGRS, *_inputImgBYS,
        *outContrastLP, *tagged, max_tag,
        salienceBU, salienceTD,
        searchRG, searchGR, searchBY, 255); // somma coeff pos=3 somma coeff neg=-3
    //printf("The number of blobs: %d",nBlobs);
    salience->ComputeMeanColors(max_tag); //compute for every box the mean Red,Green and Blue Color.
    salience->DrawMeanColorsLP(*outMeanColourLP,*tagged);
    
    //__OLD//meanOppCol.Zero();
    //__OLD//salience.DrawMeanOpponentColorsLP(meanOppCol, tagged);

    /*__OLD//blobFinder.DrawGrayLP(tmp1, tagged, 200);
    //__OLD//ACE_OS::sprintf(savename, "./rain.ppm");
    //__OLD//YARPImageFile::Write(savename, tmp1);*/

    

    //__OLD//rain.tags2Watershed(tagged, oldWshed);

    //delete(blobList);
    ippiFree(_inputImgRGS32); //Ipp32s* _inputImgRGS32=ippiMalloc_32s_C1(320,240,&psb32s);
    ippiFree(_inputImgGRS32); //Ipp32s* _inputImgGRS32=ippiMalloc_32s_C1(320,240,&psb32s);
    ippiFree(_inputImgBYS32); //Ipp32s* _inputImgBYS32=ippiMalloc_32s_C1(320,240,&psb32s);
    //ippiFree(_inputImgRGS8s);
}


