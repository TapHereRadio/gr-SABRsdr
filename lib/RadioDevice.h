#ifndef RADIODEVICE_H
#define RADIODEVICE_H
#include "ftd3xx.h"
#include "ErrorFlags.h"
#include "DeviceCommand.h"
#include <iostream>
#include <string>
#include <mutex>
#include <vector>

namespace THR
{
	/// <summary>
	/// Defines if the device is uninitialized/idle/transmitting/receiving.
	/// </summary>
	enum DeviceStatus
	{
		/// <summary>
		/// Device is not doing anything and has not been initialized.
		/// </summary>
		IdleNotInitialized,
		/// <summary>
		/// Device is not doing anything and has been initialized.
		/// </summary>
		IdleInitialized,
		/// <summary>
		/// Device is actively capturing IQ samples
		/// </summary>
		Receiving,
		/// <summary>
		/// Device is actively transmitting IQ samples
		/// </summary>
		Transmitting
	};

	/// <summary>
	/// This enum contains the channel count mappings a SABR product may have. Not all products support all modes in this enum; check the DeviceSpec.
	/// For instance R2T0 means two receivers active, zero transmitters active.
	/// Typically this activates the lowest numerically valued channels when there are more channels on the device than specified by the enum member.
	/// For instance: R1T3 on a 4x4 capable device means RadioChannel.One active and RadioChannel.Two,Four,Six active (assuming One is RX, Two,Four,Six are transmit).
	/// </summary>
	enum IQChannelConfig
	{
		/// <summary>
		/// Represents R1T0, R0T1, and R1T1 modes.
		/// </summary>
		Default = 0,
		R2T0,
		R3T0,
		R4T0,
		R0T2,
		R0T3,
		R0T4,
		R1T2,
		R1T3,
		R2T1,
		R2T2,
		R3T1
	};

	enum RadioGainMode
	{
		/// <summary>
		/// No algorithm for gain; user must set the gain to fixed values.
		/// </summary>
		Manual = 0,
		/// <summary>
		/// AGC variant best for slow changing signals (WCDMA, FDD LTE).
		/// </summary>
		SlowAGC,
		/// <summary>
		/// AGC variant best for bursty signals (TDD or FDD GSM/EDGE).
		/// </summary>
		FastAGC,
	};

	struct ProductInfo
	{
		std::string serialNumber;
		std::string deviceDescription;

		ProductInfo(std::string serialNum, std::string description) : serialNumber(serialNum), deviceDescription(description){}
	};

#define CHECK_DEVICE_STATUS(status) ((status) == FT_OK)

	class RadioDevice
	{
	private:
		bool isSetup = false;
		bool isCaptureEnabled = false;
		bool isTransmitEnabled = false;
		std::string attachedSerialNumber;
		std::mutex commandSyncObject;
		FT_HANDLE deviceHandle;
		FT_STATUS ftStatus;
		uint16_t uwVID;
		uint16_t uwPID;
		bool isUSB3 = false;
		const uint32_t FAST_RATE_STREAM_SIZE_BYTES = 4194304;
		const uint32_t MED_RATE_STREAM_SIZE_BYTES = 1048576;
		const uint32_t MED_LOW_RATE_STREAM_SIZE_BYTES = 262144;
		const uint32_t SLOW_RATE_STREAM_SIZE_BYTES = 65536;
		uint32_t iqStreamSize = MED_RATE_STREAM_SIZE_BYTES;
		const char IQ_READ_PIPE = 0x82;
		const char IQ_WRITE_PIPE = 0x02;
		const char CMD_READ_PIPE = 0x83;
		const char CMD_WRITE_PIPE = 0x03;
		const float MIN_ATTENUATION = 0.0f;
		const float MAX_ATTENUATION = 89.75f;
		const DWORD CMD_PIPE_TIMEOUT_MS = 2500;
		const DWORD IQ_PIPE_TIMEOUT_MS = 1000;
		const uint64_t MIN_LO = 70000000;
		const uint64_t MAX_LO = 6000000000;
		std::vector<uint64_t> supportedRates =
		{
			640000,
			960000,
			1000000,
			1920000,
			2000000,
			3840000,
			4000000,
			6000000,
			7680000,
			8000000,
			10000000,
			14000000,
			15360000,
			16000000,
			20000000,
			24000000,
			28000000,
			30720000,
			32000000,
			36000000,
			40000000,
			44000000,
			48000000,
			52000000,
			56000000,
			60000000,
			61440000
		};

		ErrorFlags GetDescriptors();

		ErrorFlags DeviceSetup();

		/// <summary>
		/// Attempt to open up the first found FTDI device.
		/// </summary>
		/// <returns></returns>
		ErrorFlags OpenDevice();

		/// <summary>
		/// Setup the GPIO pins.
		/// </summary>
		/// <returns></returns>
		ErrorFlags SetupGPIO();

		/// <summary>
		/// Attempt to acheive USB 3.0 speeds. Not currently being used as it is not working on Linux and can usually be solved by flipping the USB-C connector.
		/// </summary>
		/// <returns></returns>
		ErrorFlags SetupSuperSpeed();

		/// <summary>
		/// Setup the pipe timeouts.
		/// </summary>
		/// <returns></returns>
		ErrorFlags SetTimeouts();

		ErrorFlags ProcessCommand(CommandType commandType, int radioChannel, bool isSetCommand, CommandPayloadValue commandPayload, CommandPayloadValue& responsePayload);
		ErrorFlags CommandChannelTransact(DeviceCommand command, DeviceCommand*& response);
		ErrorFlags CommandChannelTransmit(DeviceCommand command);
		ErrorFlags CommandChannelReceive(DeviceCommand*& response);
	public:
		/// <summary>
		/// Determine if there are any connected FTDI devices and return their serial numbers. Also sets the provided boolean to indicate wheter any devices were found.
		/// </summary>
		/// <returns>A list of the product info of connected SABR devices (product info includes the serial number, description, and if the device is open). Sets the supplied boolean parameter to True if at least one device was found, false if no connected devices were detected.</returns>
		std::vector<ProductInfo> GetConnectedDevices(bool& deviceFound);

		/// <summary>
		/// Close the device.
		/// </summary>
		/// <returns></returns>
		ErrorFlags CloseDevice();

		/// <summary>
		/// Attempt to acquire a SABR device and perform setup on it.
		/// This might behave unexpectedly if multiple FTDI devices or radios are connected.
		/// To specify which device to use (when multiple are connected) it is advised to use the GetConnectedDevices function
		/// and then use the overloaded Setup(string deviceSerialNumber) function that takes in a specific device's serial number.
		/// </summary>
		/// <returns></returns>
		ErrorFlags Setup();

		/// <summary>
		/// Attempt to open and setup a SABR device with the provided serial number.
		/// </summary>
		/// <param name="deviceSerialNumber">Serial number of the device to setup</param>
		/// <returns></returns>
		ErrorFlags Setup(std::string deviceSerialNumber);

		/// <summary>
		/// Get the number of bytes that ReceiveSamples() will return if you don't specify the desired number of bytes out. Note that the number of IQ samples will be the 
		/// returned value divided by 4 since each IQ sample is serialized as 4 bytes (2 bytes for I and 2 bytes for Q).
		/// </summary>
		/// <returns></returns>
		uint32_t GetIQStreamSize();

		/// <summary>
		/// Receive raw IQ samples from the radio hardware as a serialized array of bytes. Will return a the raw IQ samples as an array of bytes (the number of bytes returned can be found using GetIQStreamSize());
		/// </summary>
		/// <param name="rawIQBytes"></param>
		/// <returns></returns>
		ErrorFlags ReceiveSamples(uint8_t*& rawIQBytes);

		/// <summary>
		/// Receive the specified number of raw IQ samples from the radio hardware as a serailized array of bytes.
		/// </summary>
		/// <param name="rawIQBytes"></param>
		/// <param name="numReceiveBytes">-Number of bytes to receive from the radio hardware. Should be a multiple of 4 since each IQ sample is serialized as 4 bytes.</param>
		/// <returns></returns>
		ErrorFlags ReceiveSamples(uint8_t*& rawIQBytes, uint64_t numReceiveBytes);

		/// <summary>
		/// Transmit the supplied raw IQ sample bytes.
		/// </summary>
		/// <param name="rawIQBytes"></param>
		/// <param name="numTransmitBytes">-Number of bytes being supplied to the radio hardware. Should be a multiple of 4 since each IQ sample is serialized as 4 bytes.</param>
		/// <returns></returns>
		ErrorFlags TransmitSamples(uint8_t* rawIQBytes, uint64_t numTransmitBytes);

		/// <summary>
	   /// Initializes the device. Needs to be called first before anything else.
	   /// </summary>
	   /// <returns>
	   /// <list type="table">
	   ///     <listheader>
	   ///         <term>ErrorFlags</term>
	   ///         <description>Meaning</description>
	   ///     </listheader>
	   ///     <item>
	   ///         <term>None</term>
	   ///         <description>Command accepted and applied.</description>
	   ///     </item>
	   ///     <item>
	   ///         <term>NotInitialized</term>
	   ///         <description>The RadioDevice object itself is not setup. Normally a fatal internal error, means the specific class isn't designed properly. Does not relate to the hardware device.</description>
	   ///     </item>
	   ///     <item>
	   ///         <term>NotResponding</term>
	   ///         <description>No response from the device. Normally a fatal error. Would require restarting the software and making sure the hardware device is connected properly.</description>
	   ///     </item>
	   ///     <item>
	   ///         <term>ChecksumFailure</term>
	   ///         <description>Checksum failed for the from-device response. Normally a fatal error. See NotResponding.</description>
	   ///     </item>   
	   ///     <item>
	   ///         <term>FramingError</term>
	   ///         <description>From-device response not framed properly. Normally a fatal error. See NotResponding.</description>
	   ///     </item>
	   ///     <item>
	   ///         <term>InvalidState</term>
	   ///         <description>
	   ///         Can mean a few things, essentially just means the user needs to review the specific SABR product API/capabilities.
	   ///         Can mean: hardware device not initialized, cannot do the desired action in the current hardware device state (changing manual gain when in AGC mode, for instance), or a bug with the SABR hardware. Non-fatal.
	   ///         </description>
	   ///     </item>
	   ///     <item>
	   ///         <term>OperationUnsupported</term>
	   ///         <description>Device simply does not support that command. Not fatal, just have to let the user know to review the specific SABR product capabilities.</description>
	   ///     </item>
	   ///     <item>
	   ///         <term>InvalidParameter</term>
	   ///         <description>The provided input parameter is out of bounds or not what the device expects. Not-fatal, user needs to review the specific SABR product capabilities.</description>
	   ///     </item>    
	   /// </list>
	   /// </returns>
		ErrorFlags InitDevice();

		/// <summary>
		/// Reset the device. Will automatically call InitDevice() afterwards. 
		/// All current parameters/settings for the device could be reset to default values. Can take several seconds to complete. Refer to device API reference to see exact effects of this command. Soft vs. hard reset can be mutually exclusive in what they reset.
		/// </summary>
		/// <param name="softReset">true to perform a full reset (slow); false to perform a quicker soft reset (fast).</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags ResetDevice(bool softReset);

		/// <summary>
		/// Gets the state of the device. This will tell you if the device is capturing or not, for instance. Check the returned ErrorFlags before accepting the output value.
		/// </summary>
		/// <param name="deviceStatus">See DeviceStatus enum.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags GetDeviceStatus(DeviceStatus& deviceStatus);

		/// <summary>
		/// Get the current multiplex mode. Check the returned ErrorFlags before accepting the output value.
		/// </summary>
		/// <param name="isTDM">
		/// true if in TDM mode, false if in FDM mode. 
		/// TDM is akin to TDD; transmitter and receiver are not active at the same time ever.
		/// FDM is akin to FDD; transmitter and receiver are active at the same time, and may or may not be at the same "frequency".
		/// </param>
		/// <param name="channelConfig">Specifies how many IQ channels are active.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags GetMultiplexMode(bool& isTDM, IQChannelConfig& channelConfig);

		/// <summary>
		/// Set the multiplex mode. If unsuccessful, the current parameter will remain unchanged. Note that the device node presents as many streaming iq nodes as there are channels on the device no matter what.
		/// Whether or not they actually do anything is defined by calls to this method. Note that all SABR devices default to a R1T0, R0T1, or R1T1 mode so you do not need to use this if that's all you need (unless you need to control isTDM).
		/// </summary>
		/// <param name="isTDM">
		/// true for TDM mode, false for FDM mode. 
		/// TDM is akin to TDD; transmitter and receiver are not active at the same time ever.
		/// FDM is akin to FDD; transmitter and receiver are active at the same time, and may or may not be at the same "frequency".
		/// </param>
		/// <param name="channelConfig">Specifies how many IQ channels are active. Note that a device will not support every single member in this enum, you should check the Dev Spec.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags SetMultiplexMode(bool isTDM, IQChannelConfig channelConfig);

		/// <summary>
		/// Get the current LO frequency. Check the returned ErrorFlags before accepting the output value.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="frequency">The current LO frequency, in Hz</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags GetLOFrequency(int radioChannel, uint64_t& frequency);

		/// <summary>
		/// Set the LO frequency. If unsuccessful, the current parameter will remain unchanged.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="frequency">The desired LO frequency, in Hz</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags SetLOFrequency(int radioChannel, uint64_t frequency);

		/// <summary>
		/// Get the current transmit attenuation setting. Check the returned ErrorFlags before accepting the output value.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="attenuation">The current transmit attenuation, in dB</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags GetTransmitAttenuation(int radioChannel, float& attenuation);

		/// <summary>
		/// Set the transmit attenuation.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to (should be a transmit channel)</param>
		/// <param name="attenuation">The desired attenuation.</param>
		/// <returns></returns>
		ErrorFlags SetTransmitAttenuation(int radioChannel, float attenuation);

		/// <summary>
		/// Get the current manual gain setting. This is only valid when in manual gain control mode. Check the returned ErrorFlags before accepting the output value.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="gain">The current manual gain, in dB</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags GetGain(int radioChannel, int& gain);

		/// <summary>
		/// Set the manual gain setting. This is only valid when in manual gain control mode. If unsuccessful, the current parameter will remain unchanged.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="gain">The desired manual gain, in dB</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags SetGain(int radioChannel, int gain);

		/// <summary>
		/// Get the current gain control mode. Check the returned ErrorFlags before accepting the output value.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="gainMode">Defines the current gain control mode; check the GainMode enum.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags GetGainMode(int radioChannel, RadioGainMode& gainMode);

		/// <summary>
		/// Set the gain control mode. If unsuccessful, the current parameter will remain unchanged.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="gainMode">The desired gain control mode.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags SetGainMode(int radioChannel, RadioGainMode gainMode);

		/// <summary>
		/// Get the current analog low pass filter complex bandwidth. Check the returned ErrorFlags before accepting the output value.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="complexBandwidth">The current analog low pass filter complex bandwidth, in Hz.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags GetComplexBandwidth(int radioChannel, uint64_t& complexBandwidth);

		/// <summary>
		/// Set the analog low pass filter complex bandwidth. If unsuccessful, the current parameter will remain unchanged.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="complexBandwidth">The desired analog low pass filter complex bandwidth, in Hz.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags SetComplexBandwidth(int radioChannel, uint64_t complexBandwidth);

		/// <summary>
		/// Get the sample rate of the device, where a sample is one IQ pair. Check the returned ErrorFlags before accepting the output value.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="sampleRate">The current sample rate, in Hz.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags GetSampleRate(int radioChannel, uint64_t& sampleRate);

		/// <summary>
		/// Set the sample rate of the device, where a sample is one IQ pair. If unsuccessful, the current parameter will remain unchanged.
		/// </summary>
		/// <param name="radioChannel">The RadioChannel this should apply to.</param>
		/// <param name="sampleRate">The desired sample rate, in Hz.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		/// <exception cref="ArgumentException">If radioChannel is not valid.</exception>
		ErrorFlags SetSampleRate(int radioChannel, uint64_t sampleRate);

		/// <summary>
		/// Gets the current device temperature. It's best to look at the device reference manual to understand where this comes from.
		/// </summary>
		/// <param name="tempCelsius">The temperature of the device. -99 if this is N/A for the device.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags GetDeviceTemperature(float& tempCelsius);

		/// <summary>
		/// Starts capturing IQ samples. Captures on all active channels configured by SetMultiplexMode()
		/// </summary>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags StartCapture();

		/// <summary>
		/// Stops capturing IQ samples. Stops for all channels that exist on the device.
		/// </summary>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags StopCapture();

		/// <summary>
		/// Starts transmission of IQ samples.  Transmits on all active channels defined by SetMultiplexMode().
		/// </summary>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags StartTransmit();

		/// <summary>
		/// Stops transmission of IQ samples for all channels on the device.
		/// </summary>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags StopTransmit();

		/// <summary>
		/// NOT CURRENTLY IMPLEMENTED!!! Set the reference source. Not all devices have this capability; check the device specs.
		/// This should only be called once after InitDevice() but before any other commands. Otherwise the command will be ignored.
		/// The device will automatically be reset so do not call ResetDevice() after this.
		/// </summary>
		/// <param name="isInternal">true if the device should use the onboard reference; false if it should use an external one.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags SetReferenceSource(bool isInternal);

		/// <summary>
		/// NOT CURRENTLY IMPLEMENTED!!! Get the current reference source. Check the returned ErrorFlags before accepting the output value.
		/// </summary>
		/// <param name="isInternal">true if the device is using the onboard reference; false if it is using an external one.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags GetReferenceSource(bool& isInternal);

		/// <summary>
		/// NOT CURRENTLY IMPLEMENTED!!! Advanced usage. There are sometimes many parameters that can tweak the operation of the AGC used on the device, if applicable. Not all devices support these, and the parameters themselves are specific to the device. You need to refer to the device manual to understand what to send in.
		/// </summary>
		/// <param name="radioChannel">The radio path to set the AGC parameters of.</param>
		/// <param name="paramList">Array of desired AGC parameters. You need to refer to the device manual to understand what this means per device.</param>
		/// <returns>
		/// See InitRadio() returns.
		/// </returns>
		ErrorFlags SetAdditionalAGCParameters(int radioChannel, int* paramList);

		/// <summary>
		/// NOT CURRENTLY IMPLEMENTED!!! Advanced usage. Same idea as the Set version of this method, but this allows you to get the default values for all AGC params, as sometimes you don't want to update certain parameters, so you'll want to keep those the same as what is found here.
		/// </summary>
		/// <param name="radioChannel">The radio path to get the default AGC parameters of.</param>
		/// <returns>
		/// Null if additional AGC params are unsupported by the device, otherwise array of the default AGC parameters supported by the device.
		/// </returns>
		int* GetAdditionalAGCParameterDefaults(int radioChannel);

		/// <summary>
		/// Gets the version of the embedded software if there is any. It's best to look at the device reference manual to understand what this means. Returns an error if this command is N/A for the device.
		/// </summary>
		/// <param name="version">The version of the embedded software. 0 if this is N/A for the device.</param>
		/// <returns>See InitRadio() returns.</returns>
		ErrorFlags GetERMSoftwareVersion(uint32_t& version);

		/// <summary>
		/// Gets the version of the embedded FPGA bitstream if there is one. It's best to look at the device reference manual to understand what this means. Returns an error if this command is N/A for the device.
		/// </summary>
		/// <param name="version">The version of the FPGA bitstream. 0 if this is N/A for the device.</param>
		/// <returns>See InitRadio() returns.</returns>
		ErrorFlags GetERMHardwareVersion(uint32_t& version);

		/// <summary>
		/// Highly device specific. Refer to the device reference manual. Returns an error if this command is N/A for the device.
		/// </summary>
		/// <param name="type">A number representing the FPGA installed on the device. 0 if N/A for the device.</param>
		/// <returns>See InitRadio() returns.</returns>
		ErrorFlags GetFPGAType(uint32_t& type);

		/// <summary>
		/// Certain devices can report if they have a corrupted main image and are running on a backup bootloader-type image. This let's you figure out if the device is in that mode.
		/// </summary>
		/// <param name="isInRecoveryMode">True if the device is running a backup firmware, false if it is running the intended production one.</param>
		/// <returns>See InitRadio() returns.</returns>
		ErrorFlags IsDeviceInRecoveryMode(bool& isInRecoveryMode);
	};
}

#endif