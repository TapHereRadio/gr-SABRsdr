#include "CommandPayloadValue.h"
#include "BinaryConverter.h"
#include <algorithm>

using namespace BinaryConverter;
using namespace std;
using namespace THR;

CommandPayloadValue::CommandPayloadValue()
{
	payloadHigh = 0;
	payloadLow = 0;
}

CommandPayloadValue::CommandPayloadValue(uint64_t value)
{
	payloadHigh = (uint32_t)(value >> 32);
	payloadLow = (uint32_t)value;
}

CommandPayloadValue::CommandPayloadValue(int value)
{
	payloadHigh = 0;
	payloadLow = (uint32_t)value;
}

CommandPayloadValue::CommandPayloadValue(bool value)
{
	payloadHigh = 0;
	if (value)
	{
		payloadLow = 1;
	}
	else
	{
		payloadLow = 0;
	}
}

CommandPayloadValue::CommandPayloadValue(uint32_t inputPayloadHigh, uint32_t inputPayloadLow)
{
	payloadHigh = inputPayloadHigh;
	payloadLow = inputPayloadLow;
}

uint32_t CommandPayloadValue::GetPayloadHigh()
{
	return payloadHigh;
}

uint32_t CommandPayloadValue::GetPayloadLow()
{
	return payloadLow;
}

uint64_t CommandPayloadValue::GetAsUInt64()
{
	return ((uint64_t)payloadHigh << 32) | payloadLow;
}

uint32_t CommandPayloadValue::GetAsUInt32()
{
	return payloadLow;
}

int CommandPayloadValue::GetAsInt32()
{
	return (int)payloadLow;
}

bool CommandPayloadValue::GetAsBool()
{
	return payloadLow != 0;
}

uint8_t* CommandPayloadValue::ToSerializedBytes()
{
	uint8_t* serializedPayload = new unsigned char[8];
	uint8_t* serializedPayloadHigh = GetBytes(payloadHigh);
	uint8_t* serializedPayloadLow = GetBytes(payloadLow);
	copy(serializedPayloadHigh, serializedPayloadHigh + 4, serializedPayload);
	copy(serializedPayloadLow, serializedPayloadLow + 4, serializedPayload + 4);
	delete[] serializedPayloadHigh;
	delete[] serializedPayloadLow;
	return serializedPayload;
}