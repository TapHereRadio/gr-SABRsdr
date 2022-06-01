#ifndef BINARYCONVERTER_H
#define BINARYCONVERTER_H
#include <cstdint>

namespace BinaryConverter
{
	/// <summary>
	/// Convert an array of 4 bytes into an unsigned 32 bit integer. (Deserialize)
	/// </summary>
	/// <param name="values"></param>
	/// <returns></returns>
	uint32_t ToUInt32(uint8_t* values);

	/// <summary>
	/// Convert an unsigned 32 bit integer into an array of 4 bytes. (Serialize)
	/// </summary>
	/// <param name="value"></param>
	/// <returns></returns>
	uint8_t* GetBytes(uint32_t value);
}

#endif