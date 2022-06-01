// Linux specific usb device reset for USB 3.0
#ifndef _WIN32
	// May need to install using: sudo apt-get install libusb-1.0-0-dev
	// Or similar command depending on distro
#include <libusb-1.0/libusb.h>
#include <thread>
#endif
#include "RadioDevice.h"
#include <chrono>

using namespace std;
using namespace THR;

vector<ProductInfo> RadioDevice::GetConnectedDevices(bool& deviceFound)
{
	deviceFound = false;
	vector<ProductInfo> foundSABRDevices;
	DWORD numDevices;
	ftStatus = FT_CreateDeviceInfoList(&numDevices);
	if (CHECK_DEVICE_STATUS(ftStatus))
	{
		cout << "Detected " << numDevices << " connected FTDI device(s)!" << endl;
	}
	else
	{
		// Returned vector will be empty
		return foundSABRDevices;
	}

	if (CHECK_DEVICE_STATUS(ftStatus))
	{
		if (numDevices > 0)
		{
			FT_HANDLE ftHandle = NULL;
			char serialNumber[16] = { 0 };
			char description[32] = { 0 };
			for (DWORD i = 0; i < numDevices; i++)
			{
				ftStatus = FT_GetDeviceInfoDetail(i, NULL, NULL, NULL, NULL, serialNumber, description, &ftHandle);
				if (!FT_FAILED(ftStatus))
				{
					string currSerialNumber = serialNumber;
					if (currSerialNumber.rfind("SM3000", 0) == 0 || currSerialNumber.rfind("SM1000", 0) == 0)
					{
						ProductInfo currInfo(currSerialNumber, (string)description);
						foundSABRDevices.push_back(currInfo);
						// We will just make the assumption initially that only one device is connected and set the serail number used
						// for setup to this device. If there are multiple then this will be set to whichever device is last in the enumeration...
						// To select a specific device then GetConnectedDevices should be used followed by the Setup function that takes in the serial number as 
						// an additional parameter.
						attachedSerialNumber = currSerialNumber;
						deviceFound = true;
					}
				}
			}
		}
		else
		{
			cout << "Failed to find any connected SABR devices!" << endl;
			deviceFound = false;
		}
	}
	return foundSABRDevices;
}

ErrorFlags RadioDevice::DeviceSetup()
{
	// If there is a device detected attempt to open it
	ErrorFlags resultFlags = OpenDevice();
	if (resultFlags != ErrorFlags::None)
	{
		cout << "Failed to open device!" << endl;
		return ErrorFlags::Unsuccessful;
	}

	// Setup the GPIO pins
	resultFlags = SetupGPIO();
	if (resultFlags != ErrorFlags::None)
	{
		cout << "Failed to setup GPIO!" << endl;
		return ErrorFlags::Unsuccessful;
	}

	if (!isUSB3)
	{
		resultFlags = SetupSuperSpeed();
		if (resultFlags != None || !isUSB3)
		{
			cout << "Failed to get USB 3.0 speeds.\nPlease make sure a USB 3.0 port and cable are used!\nIf problems still persist try flipping the connector and trying again!" << endl;
		}
		// This is verifed working on multiple Windows and Linux machines
		// If it doesn't work for the user then it is either a cable, machine, or device issue.
		// The radio will still work if we don't acheive USB 3.0 speeds (although higher sample rates may not)
		// So this isn't a fatal error and we can continue (although we should warn the user)
	}
	else
	{
		cout << "USB 3.0 speeds acheived" << endl;
	}

	// Set the pipe timeouts
	resultFlags = SetTimeouts();
	if (resultFlags != ErrorFlags::None)
	{
		cout << "Failed to set pipe timeouts!" << endl;
		return ErrorFlags::Unsuccessful;
	}
	isSetup = true;
	return ErrorFlags::None;
}

ErrorFlags RadioDevice::CloseDevice()
{
	ftStatus = FT_Close(deviceHandle);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		cout << "Couldn't close device. Error Code: " << ftStatus << endl;
		return ErrorFlags::Unsuccessful;
	}
	return ErrorFlags::None;
}

ErrorFlags RadioDevice::Setup()
{
	// First check if there are any connected devices
	bool deviceFound = false;
	vector<ProductInfo> connectedDevices = GetConnectedDevices(deviceFound);
	if (!deviceFound)
	{
		return ErrorFlags::Unsuccessful;
	}

	return DeviceSetup();
}

ErrorFlags RadioDevice::Setup(string deviceSerialNumber)
{
	attachedSerialNumber = deviceSerialNumber;
	return DeviceSetup();
}

ErrorFlags RadioDevice::OpenDevice()
{
	// This is a little smarter way to do it where we will get the first connected FTDI device that has a known serial number prefix
	ftStatus = FT_Create((PVOID)attachedSerialNumber.c_str(), FT_OPEN_BY_SERIAL_NUMBER, &deviceHandle);
	if (CHECK_DEVICE_STATUS(ftStatus))
	{
		GetDescriptors();
	}
	else
	{
		return ErrorFlags::Unsuccessful;
	}
	return ErrorFlags::None;
}

ErrorFlags RadioDevice::GetDescriptors()
{
	// Now we need to try to initialize and get descriptors
	FT_DEVICE_DESCRIPTOR deviceDescriptor;
	// @TODO replace with the generic FT_GetDescriptor call
	ftStatus = FT_GetDeviceDescriptor(deviceHandle, &deviceDescriptor);
	if (CHECK_DEVICE_STATUS(ftStatus))
	{
		// For this particular command there is a bug
		//return Unsuccessful;
	}
	// Extract info from the device descriptor
	uwVID = deviceDescriptor.idVendor;
	uwPID = deviceDescriptor.idProduct;
	isUSB3 = deviceDescriptor.bcdUSB >= 0x0300;

	// Get string descriptors
	// We don't really need to do anything with these so consider removal
	FT_STRING_DESCRIPTOR manufacturer;
	ftStatus = FT_GetStringDescriptor(deviceHandle, 1, &manufacturer);

	FT_STRING_DESCRIPTOR product;
	ftStatus = FT_GetStringDescriptor(deviceHandle, 2, &product);

	FT_STRING_DESCRIPTOR serialNumber;
	ftStatus = FT_GetStringDescriptor(deviceHandle, 3, &serialNumber);

	// Now try to get configuration descriptor
	FT_CONFIGURATION_DESCRIPTOR configDescriptor;
	ftStatus = FT_GetConfigurationDescriptor(deviceHandle, &configDescriptor);

	return ErrorFlags::None;
}

ErrorFlags RadioDevice::SetupGPIO()
{
	// Read in the GPIO status to prevent us from trying to set up all the GPIO again if we are reusing the device after it has
	// already been set up and the direction pin has been changed (muxed after plugging in the wrong way).
	// If the read in value is a 5 then we can skip this whole procedure. Trying to setup when this value is a 5 has been determined to cause many issues
	// and causes the device to enter into a state in which it can't be reset and must be unplugged and plugged back in.
	DWORD pulData = 0;
	ftStatus = FT_ReadGPIO(deviceHandle, &pulData);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		cout << "Couldn't read GPIO values. Error Code: " << ftStatus << endl;
		return ErrorFlags::Unsuccessful;
	}
	if (pulData == 5)
	{
		return ErrorFlags::None;
	}

	// Setup defaults for the GPIO - GPIO0 is for USB SS Mux Control, GPIO1 is for FPGA PRGM_B (reset) as of SABR Micro Rev. B.
	// Sets both GPIO as outputs (Bits 1 and 0).
	uint32_t directionValues = (FT_GPIO_DIRECTION_OUT << FT_GPIO_1) | (FT_GPIO_DIRECTION_OUT << FT_GPIO_0);
	ftStatus = FT_EnableGPIO(deviceHandle, FT_GPIO_ALL, directionValues);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		cout << "Couldn't set GPIO as outputs. Error Code: " << ftStatus << endl;	
		return ErrorFlags::Unsuccessful;
	}

	// GPIO0 AND GPIO1 should be outputting '0'.
	uint32_t outputDefaultValues = 0x00000000;
	ftStatus = FT_WriteGPIO(deviceHandle, FT_GPIO_ALL, outputDefaultValues);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		cout << "Couldn't set GPIO values. Error Code: " << ftStatus << endl;
		return ErrorFlags::Unsuccessful;
	}

	// Setup GPIO0 and GPIO1 as pull-down
	uint32_t pullValues = 0x00000000;
	ftStatus = FT_SetGPIOPull(deviceHandle, FT_GPIO_ALL, pullValues);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		cout << "Couldn't set GPIO keepers. Error Code: " << ftStatus << endl;
		return ErrorFlags::Unsuccessful;
	}
	return ErrorFlags::None;
}

// Attempts to reset the SABR USB to help acheive USB 3.0 speeds since FTDI's CycleDevicePort isn't supported on Linux
ErrorFlags LinuxUSBReset()
{
	libusb_device** devs; //pointer to pointer of device, used to retrieve a list of devices
	libusb_device_handle* dev_handle; //a device handle
	libusb_context* ctx = NULL; //a libusb session
	int r; //for return values
	ssize_t cnt; //holding number of devices in list
	r = libusb_init(&ctx); // initialize the library for the session we just declared
	if (r < 0) {
		cout << "Init Error " << r << endl;
		return ErrorFlags::Unsuccessful;
	}
	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation

	cnt = libusb_get_device_list(ctx, &devs); //get the list of devices
	if (cnt < 0) {
		cout << "Get Device Error" << endl;
		return ErrorFlags::Unsuccessful;
	}

	for (size_t idx = 0; idx < cnt; ++idx) {
		libusb_device* device = devs[idx];
		libusb_device_descriptor desc = { 0 };

		int rc = libusb_get_device_descriptor(device, &desc);
		// Look for our device in particular
		if (desc.idVendor == 0x0403 && desc.idProduct == 0x601f)
		{
			cout << "Reseting FT601" << endl;
			libusb_device_handle* handle;
			handle = libusb_open_device_with_vid_pid(NULL, desc.idVendor, desc.idProduct);
			libusb_reset_device(handle);
			// Wait for the device to come back online
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		}
	}
	return ErrorFlags::None;
}

ErrorFlags RadioDevice::SetupSuperSpeed()
{
	if (isUSB3)
	{
		return ErrorFlags::None;
	}
	cout << "Attempting to get USB 3.0 speeds..." << endl;
	ftStatus = FT_WriteGPIO(deviceHandle, 0x01, 0x01);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		cout << "Couldn't set GPIO0 output value. Error Code: " << ftStatus << endl;
		return ErrorFlags::Unsuccessful;
	}

#ifdef _WIN32
	ftStatus = FT_CycleDevicePort(deviceHandle);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		cout << "Couldn't cycle dev port. Error Code: " << ftStatus << endl;
		return ErrorFlags::Unsuccessful;
	}
#else
	cout << "Consider flipping the USB-C connector!" << endl;
	cout << "Device occasionally behaves unexpectly when connector is plugged in with the current orientation." << endl;
	if (ERROR_FLAGS_FAILURE(LinuxUSBReset()))
	{
		return ErrorFlags::Unsuccessful;
	}
#endif

	ftStatus = FT_Close(deviceHandle);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		cout << "Couldn't close device. Error Code: " << ftStatus << endl;
		return ErrorFlags::Unsuccessful;
	}

	// We will try to reopen the device for at max 3 seconds after we close it
	auto start = chrono::system_clock::now();
	chrono::duration<double> elapsedSeconds;
	ErrorFlags result = OpenDevice();
	while (result != ErrorFlags::None)
	{
		elapsedSeconds = chrono::system_clock::now() - start;
		if (elapsedSeconds.count() > 3.0)
		{
			return ErrorFlags::Unsuccessful;
		}
		result = OpenDevice();
	}
	if (isUSB3)
	{
		cout << "Successfully acheived USB 3.0 speeds!" << endl;
	}
	return ErrorFlags::None;
}

ErrorFlags RadioDevice::SetTimeouts()
{
	ftStatus = FT_SetPipeTimeout(deviceHandle, CMD_READ_PIPE, CMD_PIPE_TIMEOUT_MS);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		return ErrorFlags::Unsuccessful;
	}
	ftStatus = FT_SetPipeTimeout(deviceHandle, CMD_WRITE_PIPE, CMD_PIPE_TIMEOUT_MS);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		return ErrorFlags::Unsuccessful;
	}
	ftStatus = FT_SetPipeTimeout(deviceHandle, IQ_READ_PIPE, IQ_PIPE_TIMEOUT_MS);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		return ErrorFlags::Unsuccessful;
	}
	ftStatus = FT_SetPipeTimeout(deviceHandle, IQ_WRITE_PIPE, IQ_PIPE_TIMEOUT_MS);
	if (!CHECK_DEVICE_STATUS(ftStatus))
	{
		return ErrorFlags::Unsuccessful;
	}
	return ErrorFlags::None;
}

uint32_t RadioDevice::GetIQStreamSize()
{
	return iqStreamSize;
}

ErrorFlags RadioDevice::CommandChannelTransact(DeviceCommand command, DeviceCommand*& response)
{
	ErrorFlags result = CommandChannelTransmit(command);
	if (result != ErrorFlags::None)
	{
		response = CreateInvalidResponse(command);
		return result;
	}
	result = CommandChannelReceive(response);
	return result;
}

ErrorFlags RadioDevice::CommandChannelTransmit(DeviceCommand command)
{
	ULONG numCmdTrans = 0;
	uint8_t* buf = command.ToSerializedBytes();
	ftStatus = FT_WritePipe(deviceHandle, CMD_WRITE_PIPE, buf, 16, &numCmdTrans, NULL);
	if (FT_FAILED(ftStatus))
	{
		cout << "CMD TX timeout: " << ftStatus << endl;
		return ErrorFlags::Unsuccessful;
	}
	return ErrorFlags::None;
}

ErrorFlags RadioDevice::CommandChannelReceive(DeviceCommand*& response)
{
	ULONG bufferLength = 16;
	ULONG bytesTransferred = 0;
	unsigned char responseFromDeviceByteBuffer[16];
	ftStatus = FT_ReadPipe(deviceHandle, CMD_READ_PIPE, responseFromDeviceByteBuffer, bufferLength, &bytesTransferred, NULL);
	if (FT_FAILED(ftStatus))
	{
		cout << "Command RX timeout: " << ftStatus << endl;
		response = new DeviceCommand(0, CommandPayloadValue(0, 0), 0);
		return ErrorFlags::Unsuccessful;
	}
	response = FromSerializedBytes(responseFromDeviceByteBuffer);
	return ErrorFlags::None;
}

ErrorFlags RadioDevice::ProcessCommand(CommandType commandType, int radioChannel, bool isSetCommand, CommandPayloadValue commandPayload, CommandPayloadValue& responsePayload)
{
	responsePayload = CommandPayloadValue();
	if (!isSetup)
	{
		return ErrorFlags::NotInitialized;
	}
	DeviceCommand* deviceCommandPtr = NULL;
	bool result = CreateCommand(commandType, (RadioChannel)radioChannel, isSetCommand, commandPayload, deviceCommandPtr);
	if (!result)
	{
		return ErrorFlags::Unsuccessful;
	}

	DeviceCommand* deviceResponsePtr = NULL;
	commandSyncObject.lock();
	ErrorFlags transactionStatus = CommandChannelTransact(*deviceCommandPtr, deviceResponsePtr);
	commandSyncObject.unlock();
	if (ERROR_FLAGS_FAILURE(transactionStatus))
	{
		cout << "Didn't get a command response from the device!" << endl;
		return ErrorFlags::NotResponding;
	}

	DeviceResponseError responseError = DeviceResponseError::None;
	bool isResponseValid = deviceResponsePtr->IsValid(responseError);
	if (!isResponseValid)
	{
		switch (responseError)
		{
		case DeviceResponseError::ChecksumFailure:
			cout << "SABR response checksum failed." << endl;
			return ErrorFlags::ChecksumFailure;
		case DeviceResponseError::FramingError:
			cout << "SABR response framing invalid." << endl;
			return ErrorFlags::FramingError;
		case DeviceResponseError::NotAcknowledged:
			cout << "SABR did not ACK the command." << endl;
			return ErrorFlags::InvalidState;
		default:
			throw;
		}
	}

	responsePayload = deviceResponsePtr->GetPayloadValue();
	return ErrorFlags::None;
}

ErrorFlags RadioDevice::InitDevice()
{
	CommandPayloadValue responsePayload;
	return ProcessCommand(CommandType::InitDevice, 0, true, CommandPayloadValue(), responsePayload);
}

ErrorFlags RadioDevice::ResetDevice(bool softReset)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::Reset, 0, true, CommandPayloadValue(), responsePayload);
	if (ERROR_FLAGS_SUCCESS(result))
	{
		return 	InitDevice();
	}
	return result;
}

ErrorFlags RadioDevice::GetMultiplexMode(bool& isTDM, IQChannelConfig& channelConfig)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::MultiplexMode, 0, false, CommandPayloadValue(), responsePayload);
	channelConfig = (IQChannelConfig)responsePayload.GetPayloadHigh();
	isTDM = responsePayload.GetAsBool();
	return result;
}

ErrorFlags RadioDevice::SetMultiplexMode(bool isTDM, IQChannelConfig channelConfig)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::MultiplexMode, 0, true, CommandPayloadValue((uint32_t)channelConfig, (uint32_t)isTDM), responsePayload);
	return result;
}

ErrorFlags RadioDevice::GetDeviceStatus(DeviceStatus& deviceStatus)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::DeviceStatus, 0, false, CommandPayloadValue(), responsePayload);
	deviceStatus = (DeviceStatus)responsePayload.GetAsInt32();
	return result;
}

ErrorFlags RadioDevice::GetLOFrequency(int radioChannel, uint64_t& frequency)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::LOFrequency, radioChannel, false, CommandPayloadValue(), responsePayload);
	frequency = responsePayload.GetAsUInt64();
	return result;
}

ErrorFlags RadioDevice::SetLOFrequency(int radioChannel, uint64_t frequency)
{
	if (frequency < 70000000 || frequency > 6000000000)
	{
		return ErrorFlags::InvalidParameter;
	}
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::LOFrequency, radioChannel, true, CommandPayloadValue(frequency), responsePayload);
	return result;
}

ErrorFlags RadioDevice::GetGain(int radioChannel, int& gain)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::Gain, radioChannel, false, CommandPayloadValue(), responsePayload);
	gain = responsePayload.GetAsInt32();
	return result;
}

ErrorFlags RadioDevice::SetGain(int radioChannel, int gain)
{
	//@TODO figure out all the other check stuff based on the gain table
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::Gain, radioChannel, true, CommandPayloadValue((uint64_t)gain), responsePayload);
	return result;
}

ErrorFlags RadioDevice::GetGainMode(int radioChannel, RadioGainMode& gainMode)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::GainMode, radioChannel, false, CommandPayloadValue(), responsePayload);
	gainMode = (RadioGainMode)responsePayload.GetAsInt32();
	return result;
}

ErrorFlags RadioDevice::SetGainMode(int radioChannel, RadioGainMode gainMode)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::GainMode, radioChannel, true, CommandPayloadValue((int)gainMode), responsePayload);
	return result;
}

ErrorFlags RadioDevice::GetTransmitAttenuation(int radioChannel, float& attenuation)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::Gain, radioChannel, false, new CommandPayloadValue(0), responsePayload);
	// Device returns atten in +mdB->divide by 1000
	int tempAttenuation = responsePayload.GetAsInt32();
	attenuation = tempAttenuation / 1000.0f;
	return result;
}

ErrorFlags RadioDevice::SetTransmitAttenuation(int radioChannel, float attenuation)
{
	if (attenuation >= MIN_ATTENUATION && attenuation <= MAX_ATTENUATION)
	{
		CommandPayloadValue responsePayload;
		// Need to send the value as mdB
		int attenIndex = (int)(attenuation * 1000);
		ErrorFlags result = ProcessCommand(CommandType::Gain, radioChannel, true, CommandPayloadValue(attenIndex), responsePayload);
		return result;
	}
	else
	{
		return ErrorFlags::InvalidParameter;
	}
}

ErrorFlags RadioDevice::GetComplexBandwidth(int radioChannel, uint64_t& bandwidth)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::Bandwidth, radioChannel, false, CommandPayloadValue(), responsePayload);
	bandwidth = responsePayload.GetAsUInt64();
	return result;
}

ErrorFlags RadioDevice::SetComplexBandwidth(int radioChannel, uint64_t bandwidth)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::Bandwidth, radioChannel, true, CommandPayloadValue(bandwidth), responsePayload);
	return result;
}

ErrorFlags RadioDevice::GetSampleRate(int radioChannel, uint64_t& sampleRate)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::SampleRate, radioChannel, false, CommandPayloadValue(), responsePayload);
	sampleRate = responsePayload.GetAsUInt64();
	if (sampleRate % 2 != 0)
	{
		if ((sampleRate & 0x02) == 0x02)
		{
			// I'm assuming this is a case where the reported SR is one less than what it should be. 
			sampleRate += 1;
		}
		else
		{
			// Case where the reported rate is one more than what it should be.
			sampleRate -= 1;
		}
	}
	return result;
}

ErrorFlags RadioDevice::SetSampleRate(int radioChannel, uint64_t sampleRate)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::SampleRate, radioChannel, true, CommandPayloadValue(sampleRate), responsePayload);
	uint32_t optimalNewStreamSize = 0;
	if (ERROR_FLAGS_SUCCESS(result))
	{
		if (sampleRate <= 1000000)
		{
			optimalNewStreamSize = SLOW_RATE_STREAM_SIZE_BYTES;
		}
		else if (sampleRate <= 2000000)
		{
			optimalNewStreamSize = MED_LOW_RATE_STREAM_SIZE_BYTES;
		}
		else if (sampleRate < 30000000)
		{
			optimalNewStreamSize = MED_RATE_STREAM_SIZE_BYTES;
		}
		else
		{
			optimalNewStreamSize = FAST_RATE_STREAM_SIZE_BYTES;
		}
		iqStreamSize = optimalNewStreamSize;
	}

	return result;
}

ErrorFlags RadioDevice::GetDeviceTemperature(float& tempCelsius)
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::Temperature, 0, false, CommandPayloadValue(), responsePayload);
	tempCelsius = responsePayload.GetAsInt32() / 1000.0f;
	if (ERROR_FLAGS_FAILURE(result))
	{
		tempCelsius = -99.0f;
	}
	return result;
}

ErrorFlags RadioDevice::StartCapture()
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::CaptureEnable, 0, true, CommandPayloadValue(true), responsePayload);
	if (ERROR_FLAGS_SUCCESS(result))
	{
		isCaptureEnabled = true;
	}
	return result;
}

ErrorFlags RadioDevice::StopCapture()
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::CaptureEnable, 0, true, CommandPayloadValue(false), responsePayload);
	if (ERROR_FLAGS_SUCCESS(result))
	{
		isCaptureEnabled = false;
	}
	return result;
}

ErrorFlags RadioDevice::StartTransmit()
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::TransmitEnable, 0, true, CommandPayloadValue(true), responsePayload);
	if (ERROR_FLAGS_SUCCESS(result))
	{
		isTransmitEnabled = true;
	}
	return result;
}

ErrorFlags RadioDevice::StopTransmit()
{
	CommandPayloadValue responsePayload;
	ErrorFlags result = ProcessCommand(CommandType::TransmitEnable, 0, true, CommandPayloadValue(false), responsePayload);
	if (ERROR_FLAGS_SUCCESS(result))
	{
		isTransmitEnabled = false;
	}
	return result;
}

ErrorFlags RadioDevice::ReceiveSamples(uint8_t*& rawIQBytes)
{
	ULONG numTransferred = 0;
	rawIQBytes = new uint8_t[iqStreamSize];
	ftStatus = FT_ReadPipe(deviceHandle, IQ_READ_PIPE, rawIQBytes, (ULONG)iqStreamSize, &numTransferred, NULL);
	if (FT_SUCCESS(ftStatus))
	{
		return ErrorFlags::None;
	}
	else
	{
		return ErrorFlags::Unsuccessful;
	}
}

ErrorFlags RadioDevice::ReceiveSamples(uint8_t*& rawIQBytes, uint64_t numReceiveBytes)
{
	ULONG numTransferred = 0;
	rawIQBytes = new uint8_t[numReceiveBytes];
	ftStatus = FT_ReadPipe(deviceHandle, IQ_READ_PIPE, rawIQBytes, (ULONG)numReceiveBytes, &numTransferred, NULL);
	if (FT_SUCCESS(ftStatus))
	{
		return ErrorFlags::None;
	}
	else
	{
		return ErrorFlags::Unsuccessful;
	}
}

/// <summary>
/// Transmit the provided samples to the device.
/// Samples need to be fed at the sample rate.
/// It seems that smaller chunks of around 1024 to 4096 bytes work better than larger chunks but the optimal size is still under investingation.
/// Future improvements should take care of chunking and delaying sample transfers for the user rather than requiring them to do it.
/// </summary>
/// <param name="rawIQBytes"></param>
/// <param name="numTransmitBytes"></param>
/// <returns></returns>
ErrorFlags RadioDevice::TransmitSamples(uint8_t* rawIQBytes, uint64_t numTransmitBytes)
{
	ULONG numBytesTransferred = 0;
	ftStatus = FT_WritePipe(deviceHandle, IQ_WRITE_PIPE, rawIQBytes, (ULONG)numTransmitBytes, &numBytesTransferred, NULL);
	if (FT_SUCCESS(ftStatus))
	{
		return ErrorFlags::None;
	}
	else
	{
		return ErrorFlags::Unsuccessful;
	}
}

ErrorFlags RadioDevice::GetReferenceSource(bool& isInternal)
{
	isInternal = true;
	return ErrorFlags::OperationUnsupported;
}

ErrorFlags RadioDevice::SetReferenceSource(bool isInternal)
{
	return ErrorFlags::OperationUnsupported;
}

ErrorFlags RadioDevice::SetAdditionalAGCParameters(int radioChannel, int* paramList)
{
	return ErrorFlags::OperationUnsupported;
}

// @TODO 
int* RadioDevice::GetAdditionalAGCParameterDefaults(int radioChannel)
{
	return NULL;
}

ErrorFlags RadioDevice::GetERMSoftwareVersion(uint32_t& version)
{
	CommandPayloadValue responsePayload;
	version = 0;
	ErrorFlags result = ProcessCommand(CommandType::ERMVersion, 0, false, CommandPayloadValue(), responsePayload);
	if (ERROR_FLAGS_SUCCESS(result))
	{
		version = responsePayload.GetPayloadHigh();
		version &= 0x00007FFF;
	}
	return result;
}

ErrorFlags RadioDevice::GetERMHardwareVersion(uint32_t& version)
{
	CommandPayloadValue responsePayload;
	version = 0;
	ErrorFlags result = ProcessCommand(CommandType::ERMVersion, 0, false, CommandPayloadValue(), responsePayload);
	if (ERROR_FLAGS_SUCCESS(result))
	{
		version = responsePayload.GetPayloadLow();
		version &= 0x0000FFFF;
	}
	return result;
}

ErrorFlags RadioDevice::GetFPGAType(uint32_t& type)
{
	CommandPayloadValue responsePayload;
	type = 0;
	ErrorFlags result = ProcessCommand(CommandType::ERMVersion, 0, false, CommandPayloadValue(), responsePayload);
	if (ERROR_FLAGS_SUCCESS(result))
	{
		type = responsePayload.GetPayloadHigh() >> 16;
	}
	return result;
}

ErrorFlags RadioDevice::IsDeviceInRecoveryMode(bool& isInRecoveryMode)
{
	CommandPayloadValue responsePayload;
	isInRecoveryMode = false;
	ErrorFlags result = ProcessCommand(CommandType::ERMVersion, 0, false, CommandPayloadValue(), responsePayload);
	if (ERROR_FLAGS_SUCCESS(result))
	{
		uint32_t softwareVersionWord = responsePayload.GetPayloadHigh();
		if ((softwareVersionWord & 0x00008000) == 0x00008000)
		{
			isInRecoveryMode = true;
		}
		else
		{
			isInRecoveryMode = false;
		}
	}
	return result;
}