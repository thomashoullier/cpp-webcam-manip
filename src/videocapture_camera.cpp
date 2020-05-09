#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>  // cv::Canny()
#include <iostream>
#include <vector>
#include <string>

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

int main(int, char**)
{
    // Create a QT window
    namedWindow("Frame", WINDOW_GUI_NORMAL | WINDOW_NORMAL);
    Mat frame;
    cout << "Opening camera..." << endl;
    VideoCapture capture(0); // open the first camera, /dev/video0
    if (!capture.isOpened()) {
        cerr << "ERROR: Can't initialize camera capture" << endl;
        return 1;
    }
    capture_init(capture);
    print_cap_prop(capture);
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
        if (!enableProcessing) {
            // Display a capture frame without processing.
            imshow("Frame", frame);
        }
        else {
            // Process the captured frame and then display.
            int64 tp0 = cv::getTickCount();
            Mat processed;
            cv::Canny(frame, processed, 400, 1000, 5);
            processingTime += cv::getTickCount() - tp0;
            imshow("Frame", processed);
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
                toggle_resolution(capture); break;
            case 99/*c*/: //Toggle codec change.
                toggle_codec(capture); break;
            default: break;
        }
    }
}
