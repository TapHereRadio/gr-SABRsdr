# gr-sabrSDR
Includes GNU Radio blocks for TapHere Radio's SABR SDR

## Dependencies
* GNU Radio(3.8)
* BOOST
* SWIG
* libusb
* liborc-0.4

## Installation Instructions
To build module...
cd to module directory then run the following commands
<pre>
mkdir build
cd build
cmake ../
sudo make install
sudo ldconfig
</pre>
To remove...
<pre>
sudo make uninstall
make clean
</pre>
## Known Issues
The GNURadio blocks currently only support TDD operation. This means that you can not currently use the source block (RX) and sink block (TX) at the same time.
