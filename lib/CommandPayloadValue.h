#ifndef COMMANDPAYLOADVALUE_H
#define COMMANDPAYLOADVALUE_H
#include <cstdint>

namespace THR
{
	/// <summary>
	/// The command payload that handles the various numeric types that can be used in the SABR command frame.
	/// </summary>
	class CommandPayloadValue
	{
	private:
		uint32_t payloadHigh;
		uint32_t payloadLow;
	public:
		CommandPayloadValue();
		CommandPayloadValue(bool value);
		CommandPayloadValue(int value);
		CommandPayloadValue(uint64_t value);
		CommandPayloadValue(uint32_t inputPayloadHigh, uint32_t inputPayloadLow);

		/// <summary>
		/// Get the upper 32 bits of the command payload as an unsigned 32 bit integer.
		/// </summary>
		/// <returns></returns>
		uint32_t GetPayloadHigh();

		/// <summary>
		/// Get the lower 32 bits of the command payload as an unsigned 32 bit integer.
		/// </summary>
		/// <returns></returns>
		uint32_t GetPayloadLow();

		/// <summary>
		/// Get the command payload as an unsigned 64 bit integer.
		/// </summary>
		/// <returns></returns>
		uint64_t GetAsUInt64();

		/// <summary>
		/// Get the command payload as an unsigned 32 bit integer. Same as GetPayloadLow().
		/// </summary>
		/// <returns></returns>
		uint32_t GetAsUInt32();

		/// <summary>
		/// Get the command payload as an int.
		/// </summary>
		/// <returns></returns>
		int GetAsInt32();

		/// <summary>
		/// Get the command payload as a boolean.
		/// </summary>
		/// <returns></returns>
		bool GetAsBool();

		/// <summary>
		/// Get the command payload serialized as an array of 8 bytes.
		/// </summary>
		/// <returns></returns>
		uint8_t* ToSerializedBytes();
	};
}

#endif