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
| `w` | Increase exposure |
| `a` | Decrease gain |
| `s` | Increase gain |
| `u` | Toggle auto-focus |
| `t` | Decrease focus value (focus farther objects) |
| `y` | Increase focus value (focus nearer objects) |

### Notes
* Auto-exposure actually sets both the GAIN and the EXPOSURE automatically on
  my setup.
* On my setup, ISO\_SPEED does not correspond to anything.
* There can be a mismatch between a command value and actual value (eg. for
  resolution or exposure) but the webcam will just adjust to the closest
  matching value it supports.

## Caveats
* There are colorimetric flags for cameras in OpenCV. But this is a can of
  worms I do not want to get into.

## References
* [OpenCV VideoIO flags] (https://docs.opencv.org/4.3.0/d4/d15/group__videoio__flags__base.html)
