#include "BinaryConverter.h"

namespace BinaryConverter
{
	uint32_t ToUInt32(uint8_t* values)
	{
		return (uint32_t)(((uint32_t)values[0] << 24) | ((uint32_t)values[1] << 16) | ((uint32_t)values[2] << 8) | (uint32_t)values[3]);
	}

	uint8_t* GetBytes(uint32_t value)
	{
		uint8_t* bytesOut = new uint8_t[4];
		bytesOut[0] = ((uint8_t)(value >> 24));
		bytesOut[1] = ((uint8_t)(value >> 16));
		bytesOut[2] = ((uint8_t)(value >> 8));
		bytesOut[3] = ((uint8_t)value);
		return bytesOut;
	}
}