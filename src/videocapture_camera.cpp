#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>  // cv::Canny()
#include <iostream>
#include <vector>

using namespace cv;
using std::cout; using std::cerr; using std::endl;

void print_cap_prop(VideoCapture capture)
{
    /* Print a report of current resolution and FPS for VideoCapture. */
    cout << "Frame width: " << capture.get(CAP_PROP_FRAME_WIDTH) << endl;
    cout << "     height: " << capture.get(CAP_PROP_FRAME_HEIGHT) << endl;
    cout << "Capturing FPS: " << capture.get(CAP_PROP_FPS) << endl;
}

struct resolution {
    unsigned int width;
    unsigned int height;
};

static size_t res_index = 3;
const static std::vector<resolution> resolutions =
    {{1920, 1080},
     {1280, 720},
     {854, 480},
     {640, 360},
     {426, 240}};

size_t toggle_resolution(VideoCapture capture)
{
    /* Toggle a resolution change among available resolutions */
    size_t res_length = resolutions.size();
    res_index = (res_index + 1) % res_length;
    
    capture.set(CAP_PROP_FRAME_WIDTH, resolutions[res_index].width);
    capture.set(CAP_PROP_FRAME_HEIGHT, resolutions[res_index].height);
    capture.set(CAP_PROP_FPS, 1000); // Set maximum FPS for current resolution.

    print_cap_prop(capture);
    return res_index;
}

int main(int, char**)
{
    Mat frame;
    cout << "Opening camera..." << endl;
    VideoCapture capture(0); // open the first camera, /dev/video0
    if (!capture.isOpened()) {
        cerr << "ERROR: Can't initialize camera capture" << endl;
        return 1;
    }

    print_cap_prop(capture);
    cout << endl << "Press 'ESC' to quit, 'space' to toggle frame processing"
         << endl;
    cout << endl << "Start grabbing..." << endl;

    size_t nFrames = 0;
    bool enableProcessing = false;
    const int N = 50; // Print a report every N frames.
    int64 t0 = cv::getTickCount();
    int64 processingTime = 0;
    for (;;) {
        capture >> frame; // read the next frame from camera
        if (frame.empty()) {
            cerr << "ERROR: Can't grab camera frame." << endl;
            break;
        }
        nFrames++;
        if (nFrames % N == 0) {
            // Print a report.
            int64 t1 = cv::getTickCount();
            cout << "Frames captured: " 
                 << cv::format("%5lld", (long long int)nFrames)
                 << "    Average FPS: " << cv::format("%9.1f",
                    (double)getTickFrequency() * N / (t1 - t0))
                 << "    Average time per frame: " << cv::format("%9.2f ms",
                    (double)(t1 - t0) * 1000.0f / (N * getTickFrequency()))
                 << "    Average processing time: " << cv::format("%9.2f ms",
                    (double)(processingTime)*1000.0f / (N * getTickFrequency()))
                 << std::endl;
            t0 = t1;
            processingTime = 0;
        }
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
            default: break;
        }
    }
}
