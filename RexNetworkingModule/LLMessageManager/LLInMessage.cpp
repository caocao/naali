// For conditions of distribution and use, see copyright notice in license.txt
#include "StableHeaders.h"

#include <iostream>

#include "Poco/Net/DatagramSocket.h" // To get htons etc.

#include "LLInMessage.h"
#include "ZeroCode.h"

#include "QuatUtils.h"
#include "RexUUID.h"

using namespace RexTypes;

#undef min

namespace RexNetworking
{

/// Reads the message number from the given byte stream that represents an SLUDP message body.
/// @param data Pointer to the start of the message body. See LLMessageManager.cpp for a detailed description of the structure.
/// @param numBytes The number of bytes in data.
/// @param [out] messageIDLength The number of bytes taken by the VLE-encoding of the message ID.
/// @return The message number, or 0 to denote an invalid message.
static LLMsgID ExtractNetworkMessageID(const uint8_t *data, size_t numBytes, size_t *messageIDLength)
{
    assert(data || numBytes == 0);
    assert(messageIDLength);

    // Defend against past-buffer read for malformed packets: What is the max size the packetNumber can be in bytes?
    int maxMsgNumBytes = std::min((int)numBytes, 4); 
    if (maxMsgNumBytes <= 0)
    {
        *messageIDLength = 0;
        return 0;
    }
    if (maxMsgNumBytes >= 1 && data[0] != 0xFF)
    {
        *messageIDLength = 1;
        return data[0];
    }
    if (maxMsgNumBytes >= 2 && data[1] != 0xFF)
    {
        *messageIDLength = 2;
        return ntohs(*(u_short*)&data[0]);
    }
    if (maxMsgNumBytes >= 4)
    {
        *messageIDLength = 4;
        return ntohl(*(u_long*)&data[0]);
    }

    return 0;
}
/*
/// This function skips the messageNumber from the given message body buffer.
/// @param data A pointer to message body that has *already* been zerodecoded.
/// @param numBytes The number of bytes in the message body.
/// @param messageLength [out] The remaining length of the buffer is returned here, in bytes.
/// @return A pointer to the start of the message content, or 0 if the size of the body is 0 bytes or if the message was malformed.
static uint8_t *ComputeMessageContentStartAddrAndLength(uint8_t *data, size_t numBytes, size_t *messageLength)
{
    // Then there's a VLE-encoded packetNumber (1,2 or 4 bytes) we need to skip: 
    // Defend against past-buffer read for malformed packets: What is the max size the packetNumber can be in bytes?
    int maxMsgNumBytes = min((int)numBytes, 4); 
    if (maxMsgNumBytes <= 0) // Packet too short.
    {
        if (messageLength)
            *messageLength = 0;
        return 0;
    }
    // packetNumber is one byte long.
    if (maxMsgNumBytes >= 1 && data[0] != 0xFF)
    {
        if (messageLength)
            *messageLength = numBytes - 1;
        return *messageLength > 0 ? data + 1 : 0;
    }
    // packetNumber is two bytes long.
    if (maxMsgNumBytes >= 2 && data[1] != 0xFF)
    {
        if (messageLength)
            *messageLength = numBytes - 2;
        return *messageLength > 0 ? data + 2 : 0;
    }
    // packetNumber is four bytes long.
    if (maxMsgNumBytes >= 4)
    {
        if (messageLength)
            *messageLength = numBytes - 4;
        return *messageLength > 0 ? data + 4 : 0;
    }

    // The packet was malformed if we get here.
    if (messageLength)
        *messageLength = 0;
    return 0;
}
*/

LLInMessage::LLInMessage(size_t seqNum, const uint8_t *data, size_t numBytes, bool zeroCoded) :
    messageInfo(0), sequenceNumber(seqNum)
{
    if (zeroCoded)
    {
        size_t decodedLength = CountZeroDecodedLength(data, numBytes);
        if (decodedLength == 0)
            throw Exception("Corrupted zero-encoded stream received!");
        messageData.resize(decodedLength, 0); ///\todo Can optimize here for extra-extra bit of performance if profiling shows the need for so..
        bool success = ZeroDecode(&messageData[0], decodedLength, data, numBytes);
        if (!success)
            throw Exception("Zero-decoding input data failed!");
    }
    else
    {
        messageData.reserve(numBytes);
        messageData.insert(messageData.end(), data, data + numBytes);
    }

    size_t messageIDLength = 0;
    messageID = ExtractNetworkMessageID(&messageData[0], numBytes, &messageIDLength);
    if (messageIDLength == 0)
        throw Exception("Malformed SLUDP packet read! MessageID not present!");
    
    // We remove the messageID from the beginning of the message data buffer, since we just want to store the message content.
    messageData.erase(messageData.begin(), messageData.begin() + messageIDLength);
}

LLInMessage::LLInMessage(const LLInMessage &rhs)
{
    sequenceNumber = rhs.sequenceNumber;
    messageInfo = rhs.messageInfo;
    messageData = rhs.messageData;
    currentBlock = rhs.currentBlock;
    currentBlockInstanceNumber = rhs.currentBlockInstanceNumber;
    currentBlockInstanceCount = rhs.currentBlockInstanceCount;
    currentVariable = rhs.currentVariable;
    currentVariableSize = rhs.currentVariableSize;
    bytesRead = rhs.bytesRead;
    messageID = rhs.messageID;
}

LLInMessage::~LLInMessage()
{
}

void LLInMessage::SetMessageInfo(const LLMessageInfo *info)
{
    assert(messageInfo == 0);
    assert(info != 0);
    assert(info->id == messageID);

    messageInfo = info;

    ResetReading();
}

void LLInMessage::ResetReading()
{
    assert(messageInfo);

    currentBlock = 0;
    currentBlockInstanceNumber = 0;
    currentVariable = 0;
    currentVariableSize = 0;
    bytesRead = 0;
    variableCountBlockNext = false;

    // If first block's type is variable, prevent the user proceeding before he has read the block instance count
    // by setting the variableCountBlockNext true.
    if (messageInfo->blocks.size())
    {
        const LLMessageBlock &firstBlock = messageInfo->blocks[currentBlock];
        if (firstBlock.type == LLBlockVariable)
            variableCountBlockNext = true;
    }

    StartReadingNextBlock();
    ReadNextVariableSize();
}

#ifdef _DEBUG
void LLInMessage::SetMessageID(LLMsgID id)
{
    messageID = id;
}
#endif

uint8_t LLInMessage::ReadU8()
{
    RequireNextVariableType(LLVarU8);

    uint8_t *data = (uint8_t*)ReadBytesUnchecked(sizeof(uint8_t));
    if (!data)
        throw Exception("LLInMessage::ReadU8 failed!");
    AdvanceToNextVariable();

    return *data;
}

uint16_t LLInMessage::ReadU16()
{
    RequireNextVariableType(LLVarU16);
    
    uint16_t *data = (uint16_t*)ReadBytesUnchecked(sizeof(uint16_t));
    if (!data)
        throw Exception("LLInMessage::ReadU16 failed!");
    AdvanceToNextVariable();

    return *data;
}

uint32_t LLInMessage::ReadU32()
{
    RequireNextVariableType(LLVarU32);

    uint32_t *data = (uint32_t*)ReadBytesUnchecked(sizeof(uint32_t));
    if (!data)
        throw Exception("LLInMessage::ReadU32 failed!");
    AdvanceToNextVariable();
    return *data;
}

uint64_t LLInMessage::ReadU64()
{
    RequireNextVariableType(LLVarU64);

    uint64_t *data = (uint64_t*)ReadBytesUnchecked(sizeof(uint64_t));
    if (!data)
        throw Exception("LLInMessage::ReadU64 failed!");
    AdvanceToNextVariable();

    return *data;
}

int8_t LLInMessage::ReadS8()
{
    RequireNextVariableType(LLVarS8);

    uint8_t *data = (uint8_t*)ReadBytesUnchecked(sizeof(int8_t));
    if (!data)
        throw Exception("LLInMessage::ReadS8 failed!");
    AdvanceToNextVariable();

    return *data;
}

int16_t LLInMessage::ReadS16()
{
    RequireNextVariableType(LLVarS16);

    int16_t *data = (int16_t*)ReadBytesUnchecked(sizeof(int16_t));
    if (!data)
        throw Exception("LLInMessage::ReadS16 failed!");
    AdvanceToNextVariable();

    return *data;
}

int32_t LLInMessage::ReadS32()
{
    RequireNextVariableType(LLVarS32);

    int32_t *data = (int32_t*)ReadBytesUnchecked(sizeof(int32_t));
    if (!data)
        throw Exception("LLInMessage::ReadS32 failed!");
    AdvanceToNextVariable();

    return *data;
}

int64_t LLInMessage::ReadS64()
{
    RequireNextVariableType(LLVarS64);

    int64_t *data = (int64_t*)ReadBytesUnchecked(sizeof(int64_t));
    if (!data)
        throw Exception("LLInMessage::ReadS64 failed!");
    AdvanceToNextVariable();

    return *data;
}

float LLInMessage::ReadF32()
{
    RequireNextVariableType(LLVarF32);

    float *data = (float*)ReadBytesUnchecked(sizeof(float));
    if (!data)
        throw Exception("LLInMessage::ReadF32 failed!");
    AdvanceToNextVariable();

    return *data;
}

double LLInMessage::ReadF64()
{
    RequireNextVariableType(LLVarF64);

    double *data = (double*)ReadBytesUnchecked(sizeof(double));
    if (!data)
        throw Exception("LLInMessage::ReadF64 failed!");
    AdvanceToNextVariable();

    return *data;
}

bool LLInMessage::ReadBool()
{
    RequireNextVariableType(LLVarBOOL);

    bool *data = (bool*)ReadBytesUnchecked(sizeof(bool));
    if (!data)
        throw Exception("LLInMessage::ReadBool failed!");
    AdvanceToNextVariable();

    return *data;
}

Vector3 LLInMessage::ReadVector3()
{
    RequireNextVariableType(LLVarVector3);

    Vector3 *data = (Vector3*)ReadBytesUnchecked(sizeof(Vector3));
    if (!data)
        throw Exception("LLInMessage::ReadVector3 failed!");
    AdvanceToNextVariable();

    return *data;
}

Vector3d LLInMessage::ReadVector3d()
{
    RequireNextVariableType(LLVarVector3d);

    Vector3d *data = (Vector3d*)ReadBytesUnchecked(sizeof(Vector3d));
    if (!data)
        throw Exception("LLInMessage::ReadVector3d failed!");
    AdvanceToNextVariable();

    return *data;
}

Vector4 LLInMessage::ReadVector4()
{
    RequireNextVariableType(LLVarVector4);

    Vector4 *data = (Vector4*)ReadBytesUnchecked(sizeof(Vector4));
    if (!data)
        throw Exception("LLInMessage::ReadVector4 failed!");
    AdvanceToNextVariable();

    return *data;
}

Quaternion LLInMessage::ReadQuaternion()
{
    RequireNextVariableType(LLVarQuaternion);

    Vector3 *data = (Vector3*)ReadBytesUnchecked(sizeof(Vector3));
    if (!data)
        throw Exception("LLInMessage::ReadQuaternion failed!");
    Quaternion quat = UnpackQuaternionFromFloat3(*data);
    AdvanceToNextVariable();

    return quat;
}

RexUUID LLInMessage::ReadUUID()
{
    RequireNextVariableType(LLVarUUID);

    RexUUID *data = (RexUUID*)ReadBytesUnchecked(sizeof(RexUUID));
    if (!data)
        throw Exception("LLInMessage::ReadUUID failed!");
    AdvanceToNextVariable();

    return *data;
}

void LLInMessage::ReadString(char *dst, size_t maxSize)
{
    // The OpenSim protocol doesn't specify variable type for strings so use "LLVarNone".
    RequireNextVariableType(LLVarNone);
    
    if (maxSize == 0)
        return;

    size_t copyLen = std::min((maxSize-1), ReadVariableSize());

    size_t read = 0;
    const uint8_t *buf = ReadBuffer(&read);
    memcpy(dst, buf, copyLen);
    dst[copyLen] = '\0';
}

std::string LLInMessage::ReadString()
{
    // The OpenSim protocol doesn't specify variable type for strings so use "LLVarNone".
    RequireNextVariableType(LLVarNone);
    
    char tmp[257];
    ReadString(tmp, 256);
    
    return std::string(tmp);
}

///\ todo Add the rest of the reading functions (IPPORT, IPADDR).

const uint8_t *LLInMessage::ReadBuffer(size_t *bytesRead)
{
    if (CheckNextVariableType() < LLVarBufferByte ||
        CheckNextVariableType() > LLVarBuffer4Bytes)
    {
        throw LLMessageException(LLMessageException::ET_VariableTypeMismatch);
    }

    RequireNextVariableType(LLVarNone);

    if (bytesRead)
        *bytesRead = currentVariableSize;

    const uint8_t *data = (const uint8_t *)ReadBytesUnchecked(currentVariableSize);
    if (!data && currentVariableSize != 0)
        throw Exception("LLInMessage::ReadBuffer failed!");
    AdvanceToNextVariable();

    return data;
}

LLVariableType LLInMessage::CheckNextVariableType() const
{
    assert(messageInfo);

    if (currentBlock >= messageInfo->blocks.size())
        return LLVarNone;

    const LLMessageBlock &block = messageInfo->blocks[currentBlock];

    if (currentVariable >= block.variables.size())
        return LLVarNone;

    const LLMessageVariable &var = block.variables[currentVariable];

    return var.type;
}

void LLInMessage::AdvanceToNextVariable()
{
    assert(messageInfo);

    if (currentBlock >= messageInfo->blocks.size()) // We're finished reading if currentBlock points past all the blocks.
        return;

    const LLMessageBlock &curBlock = messageInfo->blocks[currentBlock];

    assert(currentVariable < curBlock.variables.size());
    assert(currentBlockInstanceNumber < currentBlockInstanceCount);

    ++currentVariable;
    if (currentVariable >= curBlock.variables.size())
    {
        /// Reached the end of this block, proceed to the next block
        /// or repeat the same block if it's multiple or variable.
        currentVariable = 0;
        ++currentBlockInstanceNumber;
        if (currentBlockInstanceNumber >= currentBlockInstanceCount)
        {
            currentBlockInstanceNumber = 0;
            // When entering a new block, the very first thing we do is to read the new block count instance.
            ++currentBlock;
            StartReadingNextBlock();
        }
    }

    ReadNextVariableSize();
}

size_t LLInMessage::ReadCurrentBlockInstanceCount()
{
    assert(messageInfo);

    if (currentBlock >= messageInfo->blocks.size())
        return 0;

    const LLMessageBlock &curBlock = messageInfo->blocks[currentBlock];
    switch(curBlock.type)
    {
    case LLBlockSingle:
    case LLBlockMultiple:
        variableCountBlockNext = false;
        break;
    case LLBlockVariable:
        //if (currentBlock == currentVariableCountBlock)
            //throw LLMessageException(ET_BlockInstanceCountAlreadyRead);
        variableCountBlockNext = true;
        break;
    default:
        break;
    }

    return currentBlockInstanceCount;
}

void LLInMessage::SkipToNextVariable(bool bytesAlreadyRead)
{
    assert(messageInfo);

    // We're finished reading if currentBlock points past all the blocks.
    if (currentBlock >= messageInfo->blocks.size())
        return;

    const LLMessageBlock &curBlock = messageInfo->blocks[currentBlock];
    const LLMessageVariable &curVar = curBlock.variables[currentVariable];

    assert(currentVariable < curBlock.variables.size());
    assert(currentBlockInstanceNumber < currentBlockInstanceCount);

    if(!bytesAlreadyRead)
    {
        switch(curVar.type)
        {
        case LLVarFixed:
        case LLVarBufferByte:
        case LLVarBuffer2Bytes:
            bytesRead += currentVariableSize;
            break;
        case LLVarBuffer4Bytes:
            assert(false);
            break;
        default:
            bytesRead += LLVariableSizes[curVar.type];
            break;
        }
    }

    ++currentVariable;
    if (currentVariable >= curBlock.variables.size())
    {
        /// Reached the end of this block, proceed to the next block
        /// or repeat the same block if it's multiple or variable.
        currentVariable = 0;
        ++currentBlockInstanceNumber;
        if (currentBlockInstanceNumber >= currentBlockInstanceCount)
        {
            currentBlockInstanceNumber = 0;
            // When entering a new block, the very first thing we do is to read the new block count instance.
            ++currentBlock;
            StartReadingNextBlock();
        }
    }

    ReadNextVariableSize();
}

void LLInMessage::SkipToFirstVariableByName(const std::string &variableName)
{
    assert(messageInfo);

    /// \todo Make sure that one can't skip to inside variable-count block.

    // Check out that the variable really exists.
    bool bFound = false;
    size_t skip_count = 0;
    for(size_t block_it = currentBlock; block_it <  messageInfo->blocks.size(); ++block_it)
    {
        const LLMessageBlock &curBlock = messageInfo->blocks[block_it];
        for(size_t var_it = currentVariable; var_it <  messageInfo->blocks[block_it].variables.size(); ++var_it)
        {
            const LLMessageVariable &curVar = curBlock.variables[var_it];
            if(variableName == curVar.name)
            {
                bFound = true;
                break;
            }
            
            ++skip_count;
        }
    }

    if (!bFound)
        throw LLMessageException(LLMessageException::ET_InvalidVariableName);

    // Skip to the desired variable.
    for(size_t it = 0; it < skip_count; ++it)
        SkipToNextVariable();
}

void LLInMessage::SkipToNextInstanceStart()
{
    const LLMessageBlock &curBlock = messageInfo->blocks[currentBlock];
    const int numVariablesLeftInThisInstance = (curBlock.variables.size() - currentVariable);

    for(int i = 0; i < numVariablesLeftInThisInstance; ++i)
        SkipToNextVariable();
}

void LLInMessage::StartReadingNextBlock()
{
    assert(messageInfo);

    if (currentBlock >= messageInfo->blocks.size())
    {
        currentBlockInstanceCount = 0;
        return; // The client has read all the blocks already. Nothing left to read.
    }

    const LLMessageBlock &curBlock = messageInfo->blocks[currentBlock];
    switch(curBlock.type)
    {
    case LLBlockSingle:
        currentBlockInstanceCount = 1;  // This block occurs exactly once.
        return;
    case LLBlockMultiple:
        // Multiple block quantity is always constant, so use the repeatCount from the MessageInfo
        currentBlockInstanceCount = curBlock.repeatCount;
        assert(currentBlockInstanceCount != 0);
        return;
    case LLBlockVariable:
        // Malformity check.
        if (bytesRead >= messageData.size())
        {
            SkipToPacketEnd();
            return;
        }
        // The block is variable-length. Read how many instances of it are present.
        currentBlockInstanceCount = (size_t)messageData[bytesRead++];

        // If 0 instances present, skip over this block (tail-recursively re-enter this function to do the job.)
        if (currentBlockInstanceCount == 0)
        {
            ++currentBlock;

            // Malformity check.
            if (bytesRead >= messageData.size() || currentBlock >= messageInfo->blocks.size())
            {
                SkipToPacketEnd();
                return;
            }

            // Re-do this function, read the size of the next block.
            StartReadingNextBlock();
            return;
        }
        return;
    default:
        assert(false);
        currentBlockInstanceCount = 0;
        return;
    }
}

void LLInMessage::ReadNextVariableSize()
{
    if (currentBlock >= messageInfo->blocks.size())
    {
        currentVariableSize = 0;
        return;
    }

    const LLMessageBlock &curBlock = messageInfo->blocks[currentBlock];
    assert(currentVariable < curBlock.variables.size());
    const LLMessageVariable &curVar = curBlock.variables[currentVariable];

    switch(curVar.type)
    {
    case LLVarBufferByte:
        // Variable-sized variable, size denoted with 1 byte.
        if (bytesRead >= messageData.size())
        {
            SkipToPacketEnd();
            return;
        }
        currentVariableSize = messageData[bytesRead++];
        /*if (currentVariableSize == 0)
            ///\todo Causes issues when when skipping consecutive variable-length variables!
            AdvanceToNextVariable();*/
        return;
    case LLVarBuffer2Bytes:
        // Variable-sized variable, size denoted with 2 bytes.
        if (bytesRead + 1 >= messageData.size())
        {
            SkipToPacketEnd();
            return;
        }
        currentVariableSize = (size_t)messageData[bytesRead] + ((size_t)messageData[bytesRead + 1] << 8);
        bytesRead += 2;
        /*if (currentVariableSize == 0)
            ///\todo Causes issues when skipping consecutive variable-length variables!
            AdvanceToNextVariable();*/
        return;
    case LLVarBuffer4Bytes:
        assert(false);
        currentVariableSize = 0;
        return;
    case LLVarFixed:
        currentVariableSize = curVar.count;
        assert(currentVariableSize != 0);
        return;
    default:
        currentVariableSize = LLVariableSizes[curVar.type];
        assert(currentVariableSize != 0);
        return;
    }
}

void *LLInMessage::ReadBytesUnchecked(size_t count)
{
    if (bytesRead >= messageData.size() || count == 0)
        return 0;

    if (bytesRead + count > messageData.size())
    {
        bytesRead = messageData.size(); // Jump to the end of the whole message so that we don't after this read anything.
        std::cout << "Error: Size of the message exceeded. Can't read bytes anymore." << std::endl;
        return 0;
    }

    void *data = &messageData[bytesRead];
    bytesRead += count;

    return data;
}

void LLInMessage::SkipToPacketEnd()
{
    assert(messageInfo);

    currentBlock = messageInfo->blocks.size();
    currentBlockInstanceCount = 0;
    currentVariable = 0;
    currentVariableSize = 0;
    bytesRead = messageData.size();
}

void LLInMessage::RequireNextVariableType(LLVariableType type)
{
    assert(messageInfo);

    const LLMessageBlock &curBlock = messageInfo->blocks[currentBlock];
    
    // Current block is variable, but user hasn't called ReadCurrentBlockInstanceCount().
    if (curBlock.type == LLBlockVariable && !variableCountBlockNext)
        throw LLMessageException(LLMessageException::ET_BlockInstanceCountNotRead);
        //LLMessageException("Current block is variable: use ReadCurrentBlockInstanceCount first in order to proceed.");
        

    // In case of string or buffer just return.
    if (type == LLVarNone)
        return;
    
    // Check that the variable type matches.
    if (CheckNextVariableType() != type)
        throw LLMessageException(LLMessageException::ET_VariableTypeMismatch);
        //"Tried to read wrong variable type."
}

}
