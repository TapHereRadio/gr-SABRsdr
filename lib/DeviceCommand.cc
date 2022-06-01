#include "DeviceCommand.h"
#include "BinaryConverter.h"

namespace THR
{
	DeviceCommand::DeviceCommand(CommandType commandType, RadioChannel radioChannel, bool isSetCommand, CommandPayloadValue payloadValue)
	{
		header = PACKET_PREFIX;
		footer = PACKET_SUFFIX;
		header |= isSetCommand ? SET_CMD_BIT : GET_CMD_BIT;
		responseError = DeviceResponseError::None;
		isValid = true;
		switch (commandType)
		{
		case CommandType::InitDevice:
			header |= INIT_CMD_ID;
			break;
		case CommandType::CaptureEnable:
			header |= CAPTURE_IQ_CMD_ID;
			break;
		case CommandType::LOFrequency:
			header |= LO_CMD_ID;
			break;
		case CommandType::Gain:
			header |= GAIN_CMD_ID;
			break;
		case CommandType::GainMode:
			header |= GAINMODE_CMD_ID;
			break;
		case CommandType::Bandwidth:
			header |= BANDWIDTH_CMD_ID;
			break;
		case CommandType::SampleRate:
			header |= SAMPLERATE_CMD_ID;
			break;
		case CommandType::IRFilterCfg:
			header |= IR_FILTER_CFG_CMD_ID;
			break;
		case CommandType::TransmitEnable:
			header |= TRANSMIT_IQ_CMD_ID;
			break;
		case CommandType::DeviceStatus:
			header |= DEV_STAT_CMD_ID;
			break;
		case CommandType::MultiplexMode:
			header |= MULTIPLEX_MODE_CMD_ID;
			break;
		case CommandType::Reset:
			header |= RESET_CMD_ID;
			break;
		case CommandType::ReferenceSource:
			header |= REFERENCE_SOURCE_CMD_ID;
			break;
		case CommandType::IRFilterUse:
			header |= IR_FILTER_USE_CMD_ID;
			break;
		case CommandType::AGCParams:
			header |= AGC_PARAMS_CMD_ID;
			break;
		case CommandType::CmdCounter:
			header |= CMD_COUNTER_CMD_ID;
			break;
		case CommandType::ChipsetID:
			header |= CHIPSET_CMD_ID;
			break;
		case CommandType::FirmwareUpdate:
			header |= FIRMWARE_UPDATE_CMD_ID;
			break;
		case CommandType::Temperature:
			header |= TEMPERATURE_CMD_ID;
			break;
		case CommandType::ERMVersion:
			header |= ERM_VERSION_CMD_ID;
			break;
		case CommandType::DebugB:
			header |= DEBUG_B_CMD_ID;
			break;
		case CommandType::DebugA:
			header |= DEBUG_A_CMD_ID;
			break;
		case CommandType::Nop:
			header |= NOP_CMD_ID;
			break;
		default:
			header |= NOP_CMD_ID;
			isValid = false;
			responseError = DeviceResponseError::CommandNotRecognized;
			break;
		}
		header |= (uint32_t)radioChannel;
		this->payloadValue = payloadValue;
		footer |= CalculateChecksum(header, payloadValue, footer);
	}

	DeviceCommand::DeviceCommand(DeviceCommand& failedDeviceCommand)
	{
		header = failedDeviceCommand.header;
		payloadValue = failedDeviceCommand.payloadValue;
		footer = failedDeviceCommand.footer;
		isValid = false;
		responseError = DeviceResponseError::DeviceNotResponding;
	}

	DeviceCommand::DeviceCommand(uint32_t inputHeader, CommandPayloadValue inputPayloadValue, uint32_t inputFooter)
	{
		header = inputHeader;
		payloadValue = inputPayloadValue;
		footer = inputFooter;
		isValid = IsRawPacketValid(header, payloadValue, footer, responseError);
	}

	bool DeviceCommand::IsRawPacketValid(uint32_t header, CommandPayloadValue payloadValue, uint32_t footer, DeviceResponseError& responseError)
	{
		// Check the received checksum vs. calculated.
		uint32_t calculatedChecksum = CalculateChecksum(header, payloadValue, footer);
		if (calculatedChecksum != (CHECKSUM_FIELD_MASK & footer))
		{
			responseError = DeviceResponseError::ChecksumFailure;
			return false;
		}

		// Check for frame prefix
		if ((header & PACKET_DELIMITER_MASK) != PACKET_PREFIX)
		{
			responseError = DeviceResponseError::FramingError;
			return false;
		}

		// Check for frame suffix
		if ((footer & PACKET_DELIMITER_MASK) != PACKET_SUFFIX)
		{
			responseError = DeviceResponseError::FramingError;
			return false;
		}

		// Check for device ACK.
		if ((header & ACK_NACK_FIELD_MASK) != DEV_ACK_RESP)
		{
			responseError = DeviceResponseError::NotAcknowledged;
			return false;
		}

		responseError = DeviceResponseError::None;
		return true;
	}

	uint32_t DeviceCommand::CalculateChecksum(uint32_t header, CommandPayloadValue payloadValue, uint32_t footer)
	{
		return 0x00000000;
	}

	bool DeviceCommand::IsValid(DeviceResponseError& deviceResponseError)
	{
		deviceResponseError = responseError;
		return isValid;
	}

	bool DeviceCommand::IsSetCommand()
	{
		return (header & SET_GET_CMD_FIELD_MASK) == SET_CMD_BIT;
	}

	CommandType DeviceCommand::GetCommandType()
	{
		int idFieldValue = (int)((header & CMD_ID_FIELD_MASK) >> 4);
		return (CommandType)idFieldValue;
	}

	CommandPayloadValue DeviceCommand::GetPayloadValue()
	{
		return payloadValue;
	}

	DeviceCommand* CreateInvalidResponse(DeviceCommand failedDeviceCommand)
	{
		return new DeviceCommand(failedDeviceCommand);
	}

	bool CreateCommand(CommandType commandType, RadioChannel radioChannel, bool isSetCommand, CommandPayloadValue payloadValue, DeviceCommand*& radioCommand)
	{
		radioCommand = new DeviceCommand(commandType, radioChannel, isSetCommand, payloadValue);
		DeviceResponseError tempResponseError;
		return radioCommand->IsValid(tempResponseError);
	}

	DeviceCommand* FromSerializedBytes(unsigned char* serializedBytes)
	{
		unsigned char headerBytes[4];
		unsigned char payloadHighBytes[4];
		unsigned char payloadLowBytes[4];
		unsigned char footerBytes[4];
		int currIndex = 0;
		for (int i = 0; i < 4; i++)
		{
			headerBytes[i] = serializedBytes[currIndex];
			currIndex++;
		}
		for (int i = 0; i < 4; i++)
		{
			payloadHighBytes[i] = serializedBytes[currIndex];
			currIndex++;
		}
		for (int i = 0; i < 4; i++)
		{
			payloadLowBytes[i] = serializedBytes[currIndex];
			currIndex++;
		}
		for (int i = 0; i < 4; i++)
		{
			footerBytes[i] = serializedBytes[currIndex];
			currIndex++;
		}
		uint32_t header = BinaryConverter::ToUInt32(headerBytes);
		uint32_t payloadHigh = BinaryConverter::ToUInt32(payloadHighBytes);
		uint32_t payloadLow = BinaryConverter::ToUInt32(payloadLowBytes);
		uint32_t footer = BinaryConverter::ToUInt32(footerBytes);

		return new DeviceCommand(header, CommandPayloadValue(payloadHigh, payloadLow), footer);
	}

	uint8_t* DeviceCommand::ToSerializedBytes()
	{
		uint8_t* serializedMessage = new uint8_t[16];
		uint8_t* serializedHeader = BinaryConverter::GetBytes(header);
		uint8_t* serializedPayload = payloadValue.ToSerializedBytes();
		uint8_t* serializedFooter = BinaryConverter::GetBytes(footer);

		// Now we copy over to the serialized message
		int currIndex = 0;
		for (int i = 0; i < 4; i++)
		{
			serializedMessage[currIndex] = serializedHeader[i];
			currIndex++;
		}
		for (int i = 0; i < 8; i++)
		{
			serializedMessage[currIndex] = serializedPayload[i];
			currIndex++;
		}
		for (int i = 0; i < 4; i++)
		{
			serializedMessage[currIndex] = serializedFooter[i];
			currIndex++;
		}
		delete[] serializedHeader;
		delete[] serializedFooter;
		delete[] serializedPayload;
		return serializedMessage;
	}
}