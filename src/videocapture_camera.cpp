#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>  // cv::Canny()
#include <opencv2/core/cuda.hpp>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fcntl.h>

using namespace cv;
using std::cout; using std::cerr; using std::endl;

void print_cap_prop(VideoCapture capture)
{ /* Print a report of current resolution and FPS for VideoCapture. */
    cout << "Frame width: " << capture.get(CAP_PROP_FRAME_WIDTH) << endl;
    cout << "     height: " << capture.get(CAP_PROP_FRAME_HEIGHT) << endl;
    cout << "Capturing FPS: " << capture.get(CAP_PROP_FPS) << endl;
}

/// Resolution management
struct resolution {
    unsigned int width;
    unsigned int height;
};

static size_t res_index = 0;
const static std::vector<resolution> resolutions =
    {{1920, 1080},
     {1280, 720},
     {854, 480},
     {640, 360},
     {426, 240}};

size_t toggle_resolution(VideoCapture capture)
{ /* Toggle a resolution change among available resolutions */
    size_t res_length = resolutions.size();
    res_index = (res_index + 1) % res_length;
    
    capture.set(CAP_PROP_FRAME_WIDTH, resolutions[res_index].width);
    capture.set(CAP_PROP_FRAME_HEIGHT, resolutions[res_index].height);
    capture.set(CAP_PROP_FPS, 1000); // Set maximum FPS for current resolution.

    print_cap_prop(capture);
    return res_index;
}

/// Capture codec management
static size_t codec_index = 0;
const static std::vector<int> codecs =
    {VideoWriter::fourcc('M','J','P','G'),
     VideoWriter::fourcc('Y','U','Y','V')};

size_t toggle_codec(VideoCapture capture)
{ /* Toggle the capture codec among available capture codecs */
    size_t codec_length = codecs.size();
    codec_index = (codec_index + 1) % codec_length;

    capture.set(CAP_PROP_FOURCC, codecs[codec_index]);
    capture.set(CAP_PROP_FPS, 1000); // Set maximum FPS for current resolution.
    
    print_cap_prop(capture);
    return codec_index;
}

std::string decode_fourcc (const int fourcc_code)
{ /* Decode a fourcc_code as provided by opencv as a 4-bytes concatenation of
     chars. */
     char fourcc_chars[] = {
          (char)(fourcc_code & 0xff),
          (char)((fourcc_code >> 8) & 0xff),
          (char)((fourcc_code >> 16) & 0xff),
          (char)((fourcc_code >> 24) & 0xff)};
    std::string fourcc_str(fourcc_chars, 4);
    return fourcc_str;
}

/// Capture initialization
void capture_init(VideoCapture capture)
{ /* Initialize the capture */
    capture.set(CAP_PROP_FRAME_WIDTH, resolutions[res_index].width);
    capture.set(CAP_PROP_FRAME_HEIGHT, resolutions[res_index].height);
    capture.set(CAP_PROP_FOURCC, codecs[codec_index]);
    capture.set(CAP_PROP_FPS, 1000); // Set maximum FPS for current resolution.
}

/// Current report
static int64 t0; // Starting time for next report.
const static int N = 20; // Print a report every N frames.
static size_t nFrames = 0; // Total number of frames acquired.
static int64 processingTime = 0; // Time it took to process frames.

void print_report(const String& winname, const VideoCapture capture)
{ /* Print a real time performance report in overlay. */
    if (nFrames % N == 0) {
            int64 t1 = getTickCount();
            std::ostringstream perf_string;
            perf_string
                 << "FOURCC: "<<decode_fourcc(capture.get(CAP_PROP_FOURCC))<<" "
                 << "Target FPS: " << capture.get(CAP_PROP_FPS) << " "
                 << capture.get(CAP_PROP_FRAME_WIDTH) << " x "
                 << capture.get(CAP_PROP_FRAME_HEIGHT) << " "
                 << "FPS: " << cv::format("%.1f ",
                    (double)getTickFrequency() * N / (t1 - t0))
                 << "Per frame: " << cv::format("%.1f ms ",
                    (double)(t1 - t0) * 1000.0f / (N * getTickFrequency()))
                 << "Processing: " << cv::format("%.1f ms ",
                   (double)(processingTime)*1000.0f / (N * getTickFrequency()));
            t0 = t1;
            processingTime = 0;

            displayOverlay(winname, perf_string.str());
    }
}

/// v4l2loopback management
v4l2_format v4l2_init (int v4l2lo, const VideoCapture cap)
{ /* Initialize the necessary v4l2 configuration. */
    struct v4l2_format v;
    int t;
    v.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    t = ioctl(v4l2lo, VIDIOC_G_FMT, &v);
    if (t < 0) {
        cerr << "Unable to read v4l2 format data." << endl;
        exit(t);
    }

    v.fmt.pix.width = cap.get(CAP_PROP_FRAME_WIDTH);
    v.fmt.pix.height = cap.get(CAP_PROP_FRAME_HEIGHT);
    //Internal format for opencv frames?
    v.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    v.fmt.pix.sizeimage = v.fmt.pix.width * v.fmt.pix.height * 3;
    t = ioctl(v4l2lo, VIDIOC_S_FMT, &v);
    if (t < 0) {
        cerr << "Unable to set v4l2 video format." << endl;
        exit(t);
    }
    return v;
}

int v4l2_refresh_size (int v4l2lo, v4l2_format v, const VideoCapture cap)
{ /* Match the v4l2 device resolution with the current capture
     resolution.
     Note: we need to close and reopen the device for clients such
           as `ffplay` to be able to pick up the resolution change after
           they are restarted. No idea why atm. */
    int t;
    close(v4l2lo);
    v4l2lo = open("/dev/video20", O_WRONLY); 
    if(v4l2lo < 0) {
        cerr << "Error opening v4l2l device: " << endl;
        return 1;
    }

    v.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    t = ioctl(v4l2lo, VIDIOC_G_FMT, &v);
    if (t < 0) {
        cerr << "Unable to read v4l2 format data." << endl;
        exit(t);
    }

    v.fmt.pix.width = cap.get(CAP_PROP_FRAME_WIDTH);
    v.fmt.pix.height = cap.get(CAP_PROP_FRAME_HEIGHT);
    v.fmt.pix.sizeimage = v.fmt.pix.width * v.fmt.pix.height * 3;
    t = ioctl(v4l2lo, VIDIOC_S_FMT, &v);
    if (t < 0) {
        cerr << "Unable to refresh v4l2 video format." << endl;
        exit(t);
    }
    return v4l2lo;
}

int main(int, char**)
{
    // Check for CUDA.
    if (cuda::getCudaEnabledDeviceCount() < 1) {
        cerr << "No CUDA-enabled device detected." << endl;
    }
    // Create a QT window
    namedWindow("Frame", WINDOW_GUI_NORMAL | WINDOW_NORMAL);
    Mat frame;

    /// Initialize capture.
    cout << "Opening camera..." << endl;
    VideoCapture capture(0); // open the first camera, /dev/video0
    if (!capture.isOpened()) {
        cerr << "ERROR: Can't initialize camera capture" << endl;
        return 1;
    }
    capture_init(capture);
    print_cap_prop(capture);

    /// Initialize v4l2loopback output.
    int v4l2lo = open("/dev/video20", O_WRONLY); 
    if(v4l2lo < 0) {
        cerr << "Error opening v4l2l device: " << endl;
        return 1;
    }
    v4l2_format v = v4l2_init(v4l2lo, capture);

    cout << endl << "Press 'ESC' to quit, 'space' to toggle frame processing"
         << endl;
    cout << endl << "Start grabbing..." << endl;

    bool enableProcessing = false;
    t0 = getTickCount();
    for (;;) {
        capture >> frame; // read the next frame from camera
        if (frame.empty()) {
            cerr << "ERROR: Can't grab camera frame." << endl;
            break;
        }
        nFrames++;
        print_report("Frame", capture);
        Mat processed;
        if (!enableProcessing) {processed = frame;}
        else {
            // Process the captured frame.
            int64 tp0 = getTickCount();
            cv::Canny(frame, processed, 400, 1000, 5);
            cvtColor(processed, processed, COLOR_GRAY2BGR);
            processingTime += getTickCount() - tp0;
        }
        imshow("Frame", processed);
        // Write the frame also to the v4l2loopback device:
        cvtColor(processed, processed, COLOR_BGR2RGB);
	size_t written = write(v4l2lo, processed.data,
                               processed.total() * processed.elemSize());
        if (written < 0) {
            cerr << "Error writing to v4l2loopback device." << endl;
            close(v4l2lo);
            return 1;
        }
        // User interaction.
        switch (int key = waitKey(1)) {
            case 27/*ESC*/: //Quit.
                std::cout << "Number of captured frames: " << nFrames << endl;
                return nFrames > 0 ? 0 : 1;
            case 32/*SPACE*/: //Toggle frame processing.
                enableProcessing = !enableProcessing;
                cout << "Enable frame processing ('space' key): "
                     << enableProcessing << endl; break;
            case 114/*r*/: //Toggle resolution change.
                toggle_resolution(capture); 
                v4l2lo = v4l2_refresh_size(v4l2lo, v, capture);
                break;
            case 99/*c*/: //Toggle codec change.
                toggle_codec(capture); break;
            default: break;
        }
    }
}
