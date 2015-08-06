
#include <stdio.h>
#include <linux/V4L_camera.hpp>
// #include <conversions.hpp>
#include <yarp/os/LogStream.h>
#include <list.hpp>
#include <yarp/os/Time.h>

#include <yuv.h>
#include <Leopard_MT9M021C.h>

#define errno_exit printf

#include "libv4lconvert.h"

struct v4lconvert_data *_v4lconvert_data;


using namespace yarp::os;
using namespace yarp::dev;

V4L_camera::V4L_camera() : RateThread(1000/DEFAULT_FRAMERATE)
{
    param.width  = DEFAULT_WIDTH;
    param.height = DEFAULT_HEIGHT;
    param.fps = DEFAULT_FRAMERATE;
    param.io = IO_METHOD_MMAP;
    param.deviceName = "/dev/video0";
    param.fd  = -1;
    param.image_size = 0;
    param.dst_image = NULL;
    param.n_buffers = 0;
    param.buffers = NULL;
    param.camModel = SEE3CAMCU50;
    myCounter = 0;
    timeTot = 0;
}


/**
 *    open device
 */
bool V4L_camera::open(yarp::os::Searchable& config)
{
    struct stat st;
    yTrace() << "input params are " << config.toString();

    if(!fromConfig(config))
        return false;

    // stat file
    if (-1 == stat(param.deviceName.c_str(), &st))
    {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", param.deviceName.c_str(), errno, strerror(errno));
        return false;
    }
    
    // check if it is a device
    if (!S_ISCHR(st.st_mode))
    {
        fprintf(stderr, "%s is no device\n", param.deviceName.c_str());
        return false;
    }
    
    // open device
    param.fd = v4l2_open(param.deviceName.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);
    
    // check if opening was successfull
    if (-1 == param.fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s\n", param.deviceName.c_str(), errno, strerror(errno));
        return false;
    }

    // Initting video device
    deviceInit();
    yInfo() << "START enumerating controls";
    enumerate_controls();
    yInfo() << "DONE enumerating controls\n\n";
    captureStart();
    start();

    return true;
}

bool V4L_camera::fromConfig(yarp::os::Searchable& config)
{
    if(!config.check("width") )
    {
        yDebug() << "width parameter not found, using default value of " << DEFAULT_WIDTH;
        param.width = DEFAULT_WIDTH;
    }
    else
        param.width = config.find("width").asInt();

    if(!config.check("height") )
    {
        yDebug() << "height parameter not found, using default value of " << DEFAULT_HEIGHT;
        param.height = DEFAULT_HEIGHT;
    }
    else
        param.height = config.find("height").asInt();

    if(!config.check("framerate") )
    {
        yDebug() << "framerate parameter not found, using default value of " << DEFAULT_FRAMERATE;
        param.fps = DEFAULT_FRAMERATE;
    }
    else
        param.fps = config.find("framerate").asInt();

    if(!config.check("deviceName") )
    {
        yError() << "No 'deviceName' was specified!";
        return false;
    }
    else
        param.deviceName = config.find("deviceName").asString();


    if(!config.check("camModel") )
    {
        yError() << "No 'camModel' was specified!";
        return false;
    }
    else
        param.camModel = (supported_cams) config.find("camModel").asInt();


    int type = 0;
    if(!config.check("pixelType") )
    {
        yError() << "No 'pixelType' was specified!";
        return false;
    }
    else
        type = (supported_cams) config.find("pixelType").asInt();

    switch(type)
    {
        case VOCAB_PIXEL_MONO:
            param.pixelType = V4L2_PIX_FMT_GREY;
            param.dst_image_size = param.width * param.height;
            break;

        case VOCAB_PIXEL_RGB:
            param.pixelType = V4L2_PIX_FMT_RGB24;
            param.dst_image_size = param.width * param.height * 3;
            break;

        default:
            printf("Error, no valid pixel format found!! This should not happen!!\n");
            return false;
            break;
    }

    yDebug() << "using following device " << param.deviceName << "with the configuration: " << param.width << "x" << param.height << "; camModel is " << param.camModel;
    return true;
}

int V4L_camera::getfd()
{
    return param.fd;
}

bool V4L_camera::threadInit()
{
    yTrace();

    timeStart = timeNow = timeElapsed = yarp::os::Time::now();

    frameCounter = 0;
    return true;
}

void V4L_camera::run()
{
//     yTrace();
    if(full_FrameRead())
        frameCounter++;
    else
        yError() << "Failed acquiring new frame";

    timeNow = yarp::os::Time::now();
    if( (timeElapsed = timeNow - timeStart) > 1.0f)
    {
        printf("frames acquired %d in %f sec\n", frameCounter, timeElapsed);
        frameCounter = 0;
        timeStart = timeNow;
    }
}

void V4L_camera::threadRelease()
{
    yTrace();
}


/**
 *    initialize device
 */
bool V4L_camera::deviceInit()
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_streamparm frameint;
    unsigned int min;
    
    if (-1 == xioctl(param.fd, VIDIOC_QUERYCAP, &cap)) 
    {
        if (EINVAL == errno) 
        {
            fprintf(stderr, "%s is no V4L2 device\n", param.deviceName.c_str());
        } 
        return false;
    }
    
    list_cap_v4l2(param.fd);
    
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) 
    {
        fprintf(stderr, "%s is no video capture device\n", param.deviceName.c_str());
        return false;
    }
    else
        fprintf(stderr, "%s is good V4L2_CAP_VIDEO_CAPTURE\n", param.deviceName.c_str());
    
    switch (param.io) 
    {
        
        case IO_METHOD_READ:
        {
            if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                fprintf(stderr, "%s does not support read i/o\n", param.deviceName.c_str());
                return false;
            }
        } break;
            
        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
        {
            if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                fprintf(stderr, "%s does not support streaming i/o\n", param.deviceName.c_str());
                return false;
            }
        } break;

        default:
            fprintf(stderr, "Unknown io method for device %s\n", param.deviceName.c_str());
            return false;
            break;
    }
    
    /* Select video input, video standard and tune here. */
    CLEAR(cropcap);
    
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    if (0 == xioctl(param.fd, VIDIOC_CROPCAP, &cropcap)) 
    {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */
        
        if (-1 == xioctl(param.fd, VIDIOC_S_CROP, &crop)) 
        {
            switch (errno) 
            {
                case EINVAL:
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    break;
            }
        }
    } 
    else 
    {
        /* Errors ignored. */
    }
    
    CLEAR(param.src_fmt);
    CLEAR(param.dst_fmt);

    _v4lconvert_data = v4lconvert_create(param.fd);
    if (_v4lconvert_data == NULL)
        printf("\nERROR: v4lconvert_create\n");
    else
        printf("\nDONE: v4lconvert_create\n");

    /* Here we set the pixel format we want to have to display, for getImage
    // set the desired format after conversion and ask v4l which input format I should
    // ask to the camera
    */

    param.dst_fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    param.dst_fmt.fmt.pix.width       = param.width;
    param.dst_fmt.fmt.pix.height      = param.height;
    param.dst_fmt.fmt.pix.field       = V4L2_FIELD_NONE;
    param.dst_fmt.fmt.pix.pixelformat = param.pixelType;

    if (v4lconvert_try_format(_v4lconvert_data, &(param.dst_fmt), &(param.src_fmt)) != 0)
        printf("ERROR: v4lconvert_try_format\n\n");
    else
        printf("DONE: v4lconvert_try_format\n\n");

    
    if (-1 == xioctl(param.fd, VIDIOC_S_FMT, &param.src_fmt))
        std::cout << "xioctl error VIDIOC_S_FMT" << std::endl;
    
    
    /* Note VIDIOC_S_FMT may change width and height. */
    if (param.width != param.src_fmt.fmt.pix.width)
    {
        param.width = param.src_fmt.fmt.pix.width;
        std::cout << "Image width set to " << param.width << " by device " << param.deviceName << std::endl;
    }
    
    if (param.height != param.src_fmt.fmt.pix.height)
    {
        param.height = param.src_fmt.fmt.pix.height;
        std::cout << "Image height set to " << param.height << " by device " << param.deviceName << std::endl;
    }
    
    /* If the user has set the fps to -1, don't try to set the frame interval */
    if (param.fps != -1)
    {
        CLEAR(frameint);
        
        /* Attempt to set the frame interval. */
        frameint.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        frameint.parm.capture.timeperframe.numerator = 1;
        frameint.parm.capture.timeperframe.denominator = param.fps;
        if (-1 == xioctl(param.fd, VIDIOC_S_PARM, &frameint))
            fprintf(stderr,"Unable to set frame interval.\n");
    }
    
    /* Buggy driver paranoia. */
    min = param.src_fmt.fmt.pix.width * 2;
    if (param.src_fmt.fmt.pix.bytesperline < min)
    {
        printf("bytesperline bugged!!\n");
        param.src_fmt.fmt.pix.bytesperline = min;
    }
    min = param.src_fmt.fmt.pix.bytesperline * param.src_fmt.fmt.pix.height;
    if (param.src_fmt.fmt.pix.sizeimage < min)
    {
        printf("sizeimage bugged!!\n");
        param.src_fmt.fmt.pix.sizeimage = min;
    }

    param.image_size = param.src_fmt.fmt.pix.sizeimage;

    switch (param.io) 
    {
        case IO_METHOD_READ:
            readInit(param.src_fmt.fmt.pix.sizeimage);
            break;
            
        case IO_METHOD_MMAP:
            mmapInit();
            break;
            
        case IO_METHOD_USERPTR:
            userptrInit(param.src_fmt.fmt.pix.sizeimage);
            break;
    }
    param.dst_image = (unsigned char*) malloc(param.src_fmt.fmt.pix.width * param.src_fmt.fmt.pix.height * 3);  // 3 for rgb without gamma

    query_current_image_fmt_v4l2(param.fd);

    return true;
}

bool V4L_camera::deviceUninit()
{
    unsigned int i;
    bool ret = true;
    
    switch (param.io) {
        case IO_METHOD_READ:
        {
            free(param.buffers[0].start);
        } break;
            
        case IO_METHOD_MMAP:
        {
            for (i = 0; i < param.n_buffers; ++i)
            {
                if (-1 == v4l2_munmap(param.buffers[i].start, param.buffers[i].length))
                    ret = false;
            }
        } break;
            
        case IO_METHOD_USERPTR:
        {
            for (i = 0; i < param.n_buffers; ++i)
                free(param.buffers[i].start);
        } break;
    }
    
    if(param.buffers != 0)
        free(param.buffers);
    return ret;
}

/**
 *    close device
 */
bool V4L_camera::close()
{
    yTrace();

    stop();

    if(param.fd != -1)
    {
        captureStop();
        deviceUninit();

        if (-1 == v4l2_close(param.fd))
            yError() << "Error closing V4l2 device";
        return false;
    }
    param.fd = -1;
    return true;
}


// IFrameGrabberRgb Interface
bool V4L_camera::getRgbBuffer(unsigned char *buffer)
{
    mutex.wait();
//     imageProcess(param.raw_image);
    memcpy(buffer, param.dst_image, param.width * param.height * 3);
    mutex.post();
    return true;
}

// IFrameGrabber Interface
bool V4L_camera::getRawBuffer(unsigned char *buffer)
{
//     buffer = (unsigned char *) param.raw_image;
    memcpy(buffer, param.dst_image, param.dst_image_size);
    return true;
}

int V4L_camera::getRawBufferSize()
{
    return 0;
}

/**
 * Return the height of each frame.
 * @return image height
 */
int V4L_camera::height() const
{
    yTrace();
    return param.height;
}

/**
 * Return the width of each frame.
 * @return image width
 */
int V4L_camera::width() const
{
    yTrace();
    return param.width;
}



/**
 *    Do ioctl and retry if error was EINTR ("A signal was caught during the ioctl() operation."). Parameters are the same as on ioctl.
 * 
 *    \param fd file descriptor
 *    \param request request
 *    \param argp argument
 *    \returns result from ioctl
 */
int V4L_camera::xioctl(int fd, int request, void* argp)
{
    int r;
    
    do r = v4l2_ioctl(fd, request, argp);
    while (-1 == r && EINTR == errno);
    
    return r;
}


////////////////////////////////////////////////////


struct v4l2_queryctrl queryctrl;
struct v4l2_querymenu querymenu;

void V4L_camera::enumerate_menu (void)
{
    printf ("  Menu items:\n");

    memset (&querymenu, 0, sizeof (querymenu));
    querymenu.id = queryctrl.id;

    for (querymenu.index = queryctrl.minimum;
         querymenu.index <= queryctrl.maximum;
    querymenu.index++) {
        if (0 == ioctl (param.fd, VIDIOC_QUERYMENU, &querymenu)) {
            printf ("  %s\n", querymenu.name);
        } else {
            perror ("VIDIOC_QUERYMENU");
            return;
        }
    }
}


bool V4L_camera::enumerate_controls()
{
    memset (&queryctrl, 0, sizeof (queryctrl));

    for (queryctrl.id = V4L2_CID_BASE; queryctrl.id < V4L2_CID_LASTP1; queryctrl.id++)
    {
        if (0 == ioctl (param.fd, VIDIOC_QUERYCTRL, &queryctrl))
        {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                continue;

            printf ("Control %s\n", queryctrl.name);

            if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                enumerate_menu ();
        }
        else
        {
            if (errno == EINVAL)
                continue;

            perror ("VIDIOC_QUERYCTRL");
            return false;
        }
    }

    for (queryctrl.id = V4L2_CID_PRIVATE_BASE; ; queryctrl.id++)
    {
        if (0 == ioctl (param.fd, VIDIOC_QUERYCTRL, &queryctrl))
        {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                continue;

            printf ("Control %s\n", queryctrl.name);

            if (queryctrl.type == V4L2_CTRL_TYPE_MENU)
                enumerate_menu ();
        }
        else
        {
            if (errno == EINVAL)
                break;

            perror ("VIDIOC_QUERYCTRL");
            return false;
        }
    }
    return true;
}

/**
 *   mainloop: read frames and process them
 */
void* V4L_camera::full_FrameRead(void)
{
    bool got_it = false;
    void *image_ret = NULL;
    unsigned int count;
    unsigned int numberOfTimeouts;

    numberOfTimeouts = 0;
    count = 10;  //trials

    for (int i=0; i<count; i++)
    {
        fd_set fds;
        struct timeval tv;
        int r;

        FD_ZERO(&fds);
        FD_SET(param.fd, &fds);

        /* Timeout. */
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        r = select(param.fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r)
        {
            if (EINTR == errno)
                continue;

            errno_exit("select");
        }

        if (0 == r)
        {
            if (numberOfTimeouts <= 0)
            {
                count++;
            }
            else
            {
                printf("select timeout\n");
                //exit(EXIT_FAILURE);
                got_it = false;
            }
        }

        if( (image_ret=frameRead()) != NULL)
        {
//             printf("got an image\n");
            got_it = true;
            break;
        }
        else
        {
            printf("trial %d failed\n", count);
        }
        /* EAGAIN - continue select loop. */
    }
    if(!got_it)
        printf("NO GOOD image got \n");

    return image_ret; //param.dst_image;
}

/**
 *    read single frame
 */
void* V4L_camera::frameRead()
{
    struct v4l2_buffer buf;
    unsigned int i;
    
    switch (param.io) 
    {
        case IO_METHOD_READ:
            printf("IO_METHOD_READ\n");
            if (-1 == v4l2_read(param.fd, param.buffers[0].start, param.buffers[0].length))
            {
                switch (errno) 
                {
                    case EAGAIN:
                        return NULL;
                        
                    case EIO:
                        // Could ignore EIO, see spec.
                        // fall through
                        
                    default:
                        errno_exit("read");
                        return NULL;
                }
            }
            
//             memcpy(param.raw_image, param.buffers[0].start, param.image_size);
            imageProcess(param.buffers[0].start);
            break;


            case IO_METHOD_MMAP:
            {
//                 printf("IO_METHOD_MMAP\n");

                CLEAR(buf);
                
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                
                if (-1 == xioctl(param.fd, VIDIOC_DQBUF, &buf)) 
                {
                    switch (errno) 
                    {
                        default:
                            printf("\n ERROR: VIDIOC_DQBUF\n");
                            return NULL;
                    }
                }
                
                if( !(buf.index < param.n_buffers) )
                {
                    yError() << "at line " << __LINE__;
                }
                

                mutex.wait();
                yError() << " param.image_size is " <<  param.image_size << "at line " << __LINE__;
                memcpy(param.raw_image, param.buffers[buf.index].start, param.image_size);
//                 param.raw_image = param.buffers[buf.index].start;
                imageProcess(param.raw_image);
                mutex.post();

                if (-1 == xioctl(param.fd, VIDIOC_QBUF, &buf))
                {
                    errno_exit("VIDIOC_QBUF");
                    return NULL;
                }

            } break;
            
            case IO_METHOD_USERPTR:
            {
                printf("IO_METHOD_USERPTR\n");

                CLEAR (buf);
                
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;
                
                if (-1 == xioctl(param.fd, VIDIOC_DQBUF, &buf)) 
                {
                    switch (errno) 
                    {
                        case EAGAIN:
                            return 0;
                            
                        case EIO:
                            // Could ignore EIO, see spec.
                            // fall through
                            
                        default:
                            errno_exit("VIDIOC_DQBUF");
                    }
                }
                
                for (i = 0; i < param.n_buffers; ++i)
                    if (buf.m.userptr == (unsigned long)param.buffers[i].start && buf.length == param.buffers[i].length)
                        break;
                    
                    if(! (i < param.n_buffers) )
                    {
                        yError() << "at line " << __LINE__;
                    }

                mutex.wait();
                memcpy(param.raw_image, param.buffers[buf.index].start, param.image_size);
//                 param.raw_image = (void*) buf.m.userptr;
                mutex.post();


                if (-1 == xioctl(param.fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
            }  break;

        default:
        {
            printf("frameRead, default case\n");
//             param.raw_image = NULL;
        }
    }
    return (void*) param.raw_image; //param.dst_image;
}

/**
 *   process image read
 */
void V4L_camera::imageProcess(void* p)
{
    static double _start;
    static double _end;

    _start = yarp::os::Time::now();

    switch(param.camModel)
    {
        case RAW_DATA:
        {
            break;
        }

        case SEE3CAMCU50:
        {
            if( v4lconvert_convert((v4lconvert_data*) _v4lconvert_data, &param.src_fmt, &param.dst_fmt,  (unsigned char *)p, param.image_size,  (unsigned char *)param.dst_image, param.dst_image_size)  <0 )
                printf("error converting \n");
            break;
        }

        case LEOPARD_MT9M021C:
        {
//             raw_to_bmp( (uint8_t*) p, (uint8_t*) param.dst_image, param.width, param.height, 12, 0,
//                         true, 1.6,
//                         600, -92, -70, -97, 389, -36, -130, -304, 690, 0, 0, 0);
            break;
        }

        default:
        {
            yError() << "Unsupported camera, don't know how to do color reconstruction to RGB";
            break;
        }
    }

    _end = yarp::os::Time::now();
    yDebug("Conversion time is %.6f ms", (_end - _start)*1000);
}

/**
 *    stop capturing
 */
void V4L_camera::captureStop()
{
    enum v4l2_buf_type type;
    
    switch (param.io) {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;
            
        case IO_METHOD_MMAP:
            
        case IO_METHOD_USERPTR:
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            
            if (-1 == xioctl(param.fd, VIDIOC_STREAMOFF, &type))
                errno_exit("VIDIOC_STREAMOFF");
            
            break;
    }
}

/**
 *  start capturing
 */
void V4L_camera::captureStart()
{
    unsigned int i;
    enum v4l2_buf_type type;
    
    switch (param.io)
    {
        case IO_METHOD_READ:
            /* Nothing to do. */
            break;
            
        case IO_METHOD_MMAP:
            for (i = 0; i < param.n_buffers; ++i)
            {
                struct v4l2_buffer buf;
                
                CLEAR(buf);
                
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;
                
                if (-1 == xioctl(param.fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
            }
            
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            
            if (-1 == xioctl(param.fd, VIDIOC_STREAMON, &type))
                errno_exit("VIDIOC_STREAMON");
            
//             param.raw_image = param.buffers[0].start;
            break;
            
        case IO_METHOD_USERPTR:
            for (i = 0; i < param.n_buffers; ++i) {
                struct v4l2_buffer buf;
                
                CLEAR (buf);
                
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;
                buf.index = i;
                buf.m.userptr = (unsigned long) param.buffers[i].start;
                buf.length = param.buffers[i].length;
                
                if (-1 == xioctl(param.fd, VIDIOC_QBUF, &buf))
                    errno_exit("VIDIOC_QBUF");
            }
            
            type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            
            if (-1 == xioctl(param.fd, VIDIOC_STREAMON, &type))
                errno_exit("VIDIOC_STREAMON");
            
            break;
    }
}



bool V4L_camera::readInit(unsigned int buffer_size)
{
    param.buffers = (struct buffer *) calloc(1, sizeof(*(param.buffers)));
    
    if (!param.buffers) 
    {
        fprintf(stderr, "Out of memory\n");
        return false;
    }
    
    param.buffers[0].length = buffer_size;
    param.buffers[0].start = malloc(buffer_size);
    
    if (!param.buffers[0].start) 
    {
        fprintf (stderr, "Out of memory\n");
        return false;    
    }
    return true;
}

bool V4L_camera::mmapInit()
{
    CLEAR(param.req);
    
    param.n_buffers = VIDIOC_REQBUFS_COUNT;
    param.req.count = param.n_buffers;
    param.req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    param.req.memory = V4L2_MEMORY_MMAP;
    
    if (-1 == xioctl(param.fd, VIDIOC_REQBUFS, &param.req))
    {
        if (EINVAL == errno)
        {
            fprintf(stderr, "%s does not support memory mapping\n", param.deviceName.c_str());
            return false;
        }
        else
        {
            fprintf(stderr, "Error on device %s requesting memory mapping (VIDIOC_REQBUFS)\n", param.deviceName.c_str());
            return false;
        }
    }
    
    if (param.req.count < 1)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n", param.deviceName.c_str());
        return false;
    }

    if (param.req.count == 1)
    {
        fprintf(stderr, "Only 1 buffer was available, you may encounter performance issue acquiring images from device %s\n", param.deviceName.c_str());
    }
    
    param.buffers = (struct buffer *) calloc(param.req.count, sizeof(*(param.buffers)));
    
    if (!param.buffers) 
    {
        fprintf(stderr, "Out of memory\n");
        return false;
    }

    struct v4l2_buffer buf;

    printf("n buff is %d\n", param.req.count);

    for (param.n_buffers = 0; param.n_buffers < param.req.count; param.n_buffers++)
    {
        
        CLEAR(buf);
        
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = param.n_buffers;
        
        if (-1 == xioctl(param.fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");
        
        param.buffers[param.n_buffers].length = buf.length;
        param.buffers[param.n_buffers].start = v4l2_mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, param.fd, buf.m.offset);
        
        if (MAP_FAILED == param.buffers[param.n_buffers].start)
            errno_exit("mmap");
    }
    param.raw_image = malloc(param.image_size);
    return true;
}

bool V4L_camera::userptrInit(unsigned int buffer_size)
{
//     struct v4l2_requestbuffers req;
    unsigned int page_size;
    
    page_size = getpagesize();
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
    
    CLEAR(param.req);
    
    param.req.count = VIDIOC_REQBUFS_COUNT;
    param.req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    param.req.memory = V4L2_MEMORY_USERPTR;
    
    if (-1 == xioctl(param.fd, VIDIOC_REQBUFS, &param.req))
    {
        if (EINVAL == errno) 
        {
            fprintf(stderr, "%s does not support user pointer i/o\n", param.deviceName.c_str());
            return false;
        } 
        else 
        {
            fprintf(stderr, "Error requesting VIDIOC_REQBUFS for device %s\n", param.deviceName.c_str());
            return false;
        }
    }
    
    param.buffers = (struct buffer *) calloc(4, sizeof(*(param.buffers)));
    
    if (!param.buffers) 
    {
        fprintf(stderr, "Out of memory\n");
        return false;
    }
    
    for (param.n_buffers = 0; param.n_buffers < 4; ++param.n_buffers) 
    {
        param.buffers[param.n_buffers].length = buffer_size;
        param.buffers[param.n_buffers].start = memalign(/* boundary */ page_size, buffer_size);
        
        if (!param.buffers[param.n_buffers].start) 
        {
            fprintf(stderr, "Out of memory\n");
            return false;    
        }
    }
    return true;
}

double V4L_camera::getBrightness()
{
    return 0;
}

double V4L_camera::getExposure()
{
    return 0;
}

double V4L_camera::getGain()
{
    return 0;
}

double V4L_camera::getGamma()
{
    return 0;
}

double V4L_camera::getHue()
{
    return 0;
}

double V4L_camera::getIris()
{
    return 0;
}

double V4L_camera::getSaturation()
{
    return 0;
}

double V4L_camera::getSharpness()
{
    return 0;
}

double V4L_camera::getShutter()
{
    return 0;
}

bool V4L_camera::getWhiteBalance(double& blue, double& red)
{
    return 0;
}

bool V4L_camera::setBrightness(double v)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;

    memset (&queryctrl, 0, sizeof (queryctrl));
    queryctrl.id = V4L2_CID_BRIGHTNESS;

    if (-1 == ioctl (param.fd, VIDIOC_QUERYCTRL, &queryctrl))
    {
        if (errno != EINVAL)
        {
            perror ("VIDIOC_QUERYCTRL");
            return false;
        }
        else
        {
            printf ("V4L2_CID_BRIGHTNESS is not supported\n");
        }
    }
    else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
    {
        printf ("V4L2_CID_BRIGHTNESS is not supported\n");
    }
    else
    {
        memset (&control, 0, sizeof (control));
        control.id = V4L2_CID_BRIGHTNESS;
        control.value = queryctrl.default_value;

        if (-1 == ioctl(param.fd, VIDIOC_S_CTRL, &control))
        {
            perror ("VIDIOC_S_CTRL");
            return false;
        }
    }
    return true;
}

bool V4L_camera::setExposure(double v)
{
    return false;
}

bool V4L_camera::setGain(double v)
{
    return false;
}

bool V4L_camera::setGamma(double v)
{
    return false;
}

bool V4L_camera::setHue(double v)
{
    return false;
}

bool V4L_camera::setIris(double v)
{
    return false;
}

bool V4L_camera::setSaturation(double v)
{
    return false;
}

bool V4L_camera::setSharpness(double v)
{
    return false;
}

bool V4L_camera::setShutter(double v)
{
    return false;
}

bool V4L_camera::setWhiteBalance(double blue, double red)
{
    return false;
}

