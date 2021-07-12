# Video Stream Live Capture and Processing
Live video capture using [Decklink](https://www.blackmagicdesign.com/products/decklink/) devices.
Video frames are processed with [OpenCV](https://opencv.org/) and the stream is then forwarded to localhost using [ZeroMQ](https://zeromq.org/).

Project used for the live augmented reality of surgical procedures using the
[DaVinci](https://www.intuitive.com/) surgical system endoscope as video source.

Obtain a copy including all dependencies with:
```bash
git clone --recurse-submodules https://github.com/aledelmo/LiveCapture
```

## How to Build

### Prerequisites

Please refer to the [OpenCV official guide](https://docs.opencv.org/4.5.2/d7/d9f/tutorial_linux_install.html) for dependencies
(e.g. libgtk2.0-dev).

Download DesktopVideo [here](https://www.blackmagicdesign.com/developer/product/capture-and-playback) for the latest BlackMagic drivers.
Once downloaded, extract DesktopVideo and install the deb packages with the following instructions:
```bash
sudo dpkg -i desktopvideo_*.deb desktopvideo-gui_*.deb mediaexpress_*.deb
```

### Libraries

Please refer to the [libzmq](https://github.com/zeromq/libzmq), [cppzmq](https://github.com/zeromq/cppzmq) and [opencv](https://github.com/opencv/opencv) GitHub pages for more detailed instructions.

#### libzmq

```bash
cd libs/ZMQ/libzmq
cmake -Hlibzmq-4.3.4 -Bbuild -DWITH_PERF_TOOL=OFF \
      -DZMQ_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_DRAFTS=OFF .
cd build
make -j20
```

#### cppzmq

```bash
cd libs/ZMQ/cppzmq
cmake -H. -Bbuild -DENABLE_DRAFTS=OFF -DCOVERAGE=OFF \
      -D CMAKE_PREFIX_PATH=../libzmq/build
cd build
make -j20
```

#### OpenCV

```bash
cd libs/opencv
mkdir -p build && cd build
cmake ..
make -j20
```

### Project

Compile client-side streaming service
```bash
mkdir -p build && cd build
cmake -DWITH_FFMPEG=0 .. 
make -j20
```

Prepare server-side Python environment
```bash
pip install -r requirements.txt
```

## How to Use

1. Video ReadBack: Acquire video input on device <d> with mode <m> and forwarding the stream to the port <h>
    ```bash
    ./Client -d <device id> -m <mode id> -h <port> [OPTIONS]
    ```
   *OPTIONS: -3 <stereoscopic 3D> -p <pixel_format>*
   
2. Signal simulation
   Generate test video stream on device <d> with mode <m>
    ```bash
    ./GeneratePattern -d <device id> -m <mode id> [OPTIONS]
    ```
   *OPTIONS: -3 <stereoscopic 3D> -p <pixel_format> -c <channels> -s <depth>*

3.  Server: Request/Reply Python server receiving video stream
    ```bash
    python Server.py --port1 <port_number> --port2 <port_number>
    ```

## System Requirements

Ubuntu 20.04 - *Linux support only.*

Tested using a DeckLink Duo 2 video acquisition card.

## Contacts

For any inquiries please contact:
[Alessandro Delmonte](https://aledelmo.github.io) @ [alessandro.delmonte@institutimagine.org](mailto:alessandro.delmonte@institutimagine.org)

## License

This project is licensed under the [Apache License 2.0](LICENSE) - see the [LICENSE](LICENSE) file for
details
