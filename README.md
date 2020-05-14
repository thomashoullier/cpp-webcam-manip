# OpenCV Webcam Manipulation
We use OpenCV in C++ to manipulate the webcam feed and write it to a virtual
video device.

This is developed in Void Linux.

## Building
(Works for my development environment, but it is setup weirdly)

```shell
/usr/bin/g++ src/videocapture_camera.cpp -o build/videocapture_camera `pkg-config --cflags --libs opencv4 cblas`
```

## Usage

### Key bindings
| Key        | Function |
|:-------------:|:-------------:|
| `Esc`      |  Quit |
| `Space`    |  Toggle frame processing |
| `r` | Cycle between capture resolutions |
| `c` | Cycle between capture codecs |
| `e` | Toggle auto-exposure |
| `q` | Decrease exposure |
| `w` | Increase exposre |
| `a` | Decrease gain |
| `s` | Increase gain |

### Notes
* Auto-exposure actually sets both the GAIN and the EXPOSURE automatically on
  my setup.
* On my setup, ISO\_SPEED does not correspond to anything.

## Caveats

## References
* [OpenCV VideoIO flags] (https://docs.opencv.org/4.3.0/d4/d15/group__videoio__flags__base.html)
