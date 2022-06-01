#ifndef SPECSENUMS_H
#define SPECSENUMS_H

namespace THR
{
	/// <summary>
/// A radio channel is what we define as an individual ADC or DAC. 
/// A channel can be receive or transmit; not both. You need to check the DeviceSpec to see whether the channel is transmit or receive.
/// This can be extended for more possible channels, but you must update a lot of other code if you do. Recommend this doesn't get changed.
/// Typically One is receive. If there are TX+RX, then One would typically be receive, Two would be transmit, Three would be receive, Four would be transmit. However you really should confirm with the DeviceSpec.
/// </summary>
	enum class RadioChannel
	{
		/// <summary>
		/// RX1 port
		/// </summary>
		One = 0,
		/// <summary>
		/// TX1 port
		/// </summary>
		Two,
		/// <summary>
		/// RX2 port
		/// </summary>
		Three,
		/// <summary>
		/// TX2 port
		/// </summary>
		Four
	};

	/// <summary>
	/// Defines the antenna connector attached to a device radio channel section. Assume female gendered usually.
	/// </summary>
	enum class AntennaConnector
	{
		SMA = 0,
		SMB,
		SMC,
		MMCX,
		UFL
	};

	/// <summary>
	/// Defines the RF design architecture for a device channel. All assume IQ architecture.
	/// </summary>
	enum class Architecture
	{
		/// <summary>
		/// Single conversion stage, LO is set to the carrier frequency to mix down/up to/from DC.
		/// </summary>
		DirectConversion = 0,
		/// <summary>
		/// No conversion stage, antenna to/from ADC/DAC.
		/// </summary>
		DirectSampling,
		/// <summary>
		/// Multiple conversion stages; arbitrary baseband/intermediate frequencies.
		/// </summary>
		Heterodyne
	};

	/// <summary>
	/// Defines the bit depth of an individual I or Q portion of a sample. For instance, TwelveBit means an IQ sample is 24 bits (12 I + 12 Q bits).
	/// Note that this just means what the device channel ADC is capable of; IQ samples are always transported from devices sign extended to 32 bits for one IQ sample.
	/// </summary>
	enum class BitDepth
	{
		EightBit = 0,
		TenBit,
		ElevenBit,
		TwelveBit,
		FourteenBit,
		SixteenBit
	};

	/// <summary>
	/// Defines different kinds of gain control. Does not mean a device supports this mode, however.
	/// </summary>
	enum class GainMode
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

	/// <summary>
	/// Defines what kind of gain tables there are, which effects how a gain setting is distributed across different front-end components. Does not mean a device supports this mode, however.
	/// </summary>
	enum class GainTableType
	{
		/// <summary>
		/// 
		/// </summary>
		Full = 0,
		/// <summary>
		/// 
		/// </summary>
		Split
	};

	/// <summary>
	/// Defines the LO to Channel relationships.
	/// </summary>
	enum class LocalOscillatorRelation
	{
		/// <summary>
		/// This should never be seen in a valid DeviceSpec; means something wasn't setup properly. Do not use.
		/// </summary>
		NotInitialized = 0,
		/// <summary>
		/// Phase incoherent, independent control.
		/// </summary>
		IncoherentIndependent,
		/// <summary>
		/// Phase incoherent, shared control.
		/// </summary>
		IncoherentShared,
		/// <summary>
		/// Phase coherent, independent control.
		/// </summary>
		CoherentIndependent,
		/// <summary>
		/// Phase coherent, shared control.
		/// </summary>
		CoherentShared
	};

	/// <summary>
	/// Defines how channels pairings may be configured for transmit/receive for a device.
	/// </summary>
	enum class ChannelDirectionRelations
	{
		/// <summary>
		/// This should never be seen in a valid DeviceSpec; means something wasn't setup properly. Do not use.
		/// </summary>
		NotInitialized = 0,
		/// <summary>
		/// 
		/// </summary>
		HalfDuplex = 1,
		/// <summary>
		/// 
		/// </summary>
		FullDuplex = 2,
		/// <summary>
		/// 
		/// </summary>
		All = HalfDuplex | FullDuplex
	};

	/// <summary>
	/// Defines the supported gain control modes for a device.
	/// </summary>
	enum class SupportedGainModes
	{
		/// <summary>
		/// This should never be seen in a valid DeviceSpec; means something wasn't setup properly. Do not use.
		/// </summary>
		NotInitialized = 0,
		Manual = 1,
		SlowAGC = 2,
		FastAGC = 4,
		HybridAGC = 8,
		/// <summary>
		/// Supports manual, slow, and fast AGC.
		/// </summary>
		AllButHybrid = Manual | SlowAGC | FastAGC,
		/// <summary>
		/// Supports manual, slow, fast, and hybrid AGC.
		/// </summary>
		All = AllButHybrid | HybridAGC,
	};

	/// <summary>
	/// Defines the supported gain table types for a device.
	/// </summary>
	enum class SupportedGainTableTypes
	{
		/// <summary>
		/// This should never be seen in a valid DeviceSpec; means something wasn't setup properly. Do not use.
		/// </summary>
		NotInitialized = 0,
		Full = 1,
		Split = 2,
		/// <summary>
		/// Both full and split gain tables.
		/// </summary>
		Both = Full | Split
	};
}

#endif