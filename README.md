# gr-sabrSDR
Includes GNU Radio blocks for TapHere Radio's SABR SDR

## Dependencies
* GNU Radio(3.8)
* FTDI D3XXX Drivers (https://ftdichip.com/drivers/d3xx-drivers/)
* BOOST
* SWIG
* libusb
* liborc-0.4

## Installation Instructions
First please make sure that the FTDI D3XXX drivers are installed. FTDI provides links for the x86 and x64 drivers, if ARMv7 or ARMv8 drivers are needed please contact us. There is a readme provided by FTDI to install but basically you just run the following commands...
<pre>
sudo rm /usr/lib/libftd3xx.so
sudo cp libftd3xx.so /usr/lib/
sudo cp libftd3xx.so.0.5.21 /usr/lib/
sudo cp 51-ftd3xx.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules
</pre>
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
