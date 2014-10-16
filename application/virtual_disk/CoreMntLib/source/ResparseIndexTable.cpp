#include "../include/ResparseIndexTable.h"
#include <algorithm>

inline ULONG32 CalcDataOffset(ULONG64 logicSize, ULONG32 blockSize, ULONG32 granulity)
{
    ULONG64 ungranulParam = sizeof(IndexTableHeader) + 
                           s_integer_div(logicSize, blockSize)*sizeof(ULONG32);
    return s_granulity_to(ungranulParam, granulity);
}

CResparseIndexTable::CResparseIndexTable(ULONG64 logicSize, ULONG32 blockSize, ULONG32 granulity):
    blockSize_(blockSize), 
    dataOffset_(CalcDataOffset(logicSize, blockSize, granulity))
{
    tableData_.resize(dataOffset_);
    tableHeader_ = (IndexTableHeader*)&tableData_[0];
    tableHeader_->indexTableSize = s_integer_div(logicSize, blockSize);
    tableHeader_->usedBlockCount = 0;
    tableHeader_->currentLogOffset = blockSize;
    tableHeader_->firstLogBlock = s_unusedBlokToken;
    tableHeader_->currentLogBlock = s_unusedBlokToken;

    std::fill(&tableHeader_->table[0], &tableHeader_->table[tableHeader_->indexTableSize], s_unusedBlokToken);
}
void CResparseIndexTable::getLogBlock()
{
    std::set<ULONG32> usedBlock;
    for(ULONG32 i = 0; i < tableHeader_->indexTableSize; ++i)
        if(tableHeader_->table[i] != s_unusedBlokToken)
            usedBlock.insert(tableHeader_->table[i]);

    if(tableHeader_->currentLogBlock != s_unusedBlokToken)
        for(ULONG32 i = 0; i < tableHeader_->currentLogBlock; ++i)
            if(usedBlock.find(i) == usedBlock.end())
                logBlocks_.insert(i);
}
ULONG64 CResparseIndexTable::getMessageBlocksSize()
{
    ULONG64 retOffset = 0;
    for(std::set<ULONG32>::iterator i = logBlocks_.begin();i != logBlocks_.end(); ++i)
        retOffset += blockSize_;
    if(tableHeader_->currentLogBlock != s_unusedBlokToken)
        retOffset += tableHeader_->currentLogOffset;
    return retOffset;
}
ULONG64 CResparseIndexTable::LogMessageOffset2FileOffset(ULONG64 virtualOffset)
{
    ULONG64 maxRange = getMessageBlocksSize();
    if(maxRange < virtualOffset)
        throw std::runtime_error("Offset out of range ");
    ULONG32 logBlockIndex = boost::numeric_cast<ULONG32>(virtualOffset/blockSize_);
    ULONG32 logBlockOffset = virtualOffset%blockSize_;
    ULONG32 logBlockNum = s_unusedBlokToken;
    ULONG32 index = 0;
    for(std::set<ULONG32>::iterator i = logBlocks_.begin();i != logBlocks_.end(); ++i, ++index)
        if(index == logBlockIndex)
            logBlockNum = *i;
    if(logBlockIndex == logBlocks_.size())
        logBlockNum = tableHeader_->currentLogBlock;

    return logBlockNum*blockSize_ + logBlockOffset;
}
ULONG64 CResparseIndexTable::getNextLogMessagePosition(ULONG32 messageSize)
{
    if(messageSize > blockSize_ - tableHeader_->currentLogOffset)
    {//need add new block
        tableHeader_->currentLogBlock = tableHeader_->usedBlockCount;
        tableHeader_->currentLogOffset = 0;
        ++(tableHeader_->usedBlockCount);
        if(tableHeader_->firstLogBlock == s_unusedBlokToken)
            tableHeader_->firstLogBlock = tableHeader_->currentLogBlock;
        else
            logBlocks_.insert(tableHeader_->currentLogBlock);
    }
    if(messageSize > blockSize_ - tableHeader_->currentLogOffset)
        throw std::runtime_error("Message too long.");    
    ULONG64 offset = tableHeader_->currentLogBlock * blockSize_ + tableHeader_->currentLogOffset;
    tableHeader_->currentLogOffset += messageSize;
    return offset;
}
ULONG32 CResparseIndexTable::at(ULONG32 index)
{
    if(index >= tableHeader_->indexTableSize)
        throw std::runtime_error("Index out of range.");
    return tableHeader_->table[index];
}
void CResparseIndexTable::set(ULONG32 index, ULONG32 val)
{
    if(index >= tableHeader_->indexTableSize)
        throw std::runtime_error("Index out of range.");
    if(tableHeader_->table[index] != s_unusedBlokToken || val == s_unusedBlokToken)
        throw std::runtime_error("User reset for free block.");
    tableHeader_->table[index] = val;
    ++(tableHeader_->usedBlockCount);
}
void CResparseIndexTable::reset(ULONG32 index)
{
    if(index >= tableHeader_->indexTableSize)
        throw std::runtime_error("Index out of range.");
    if(tableHeader_->table[index] == s_unusedBlokToken)
        throw std::runtime_error("User reset for free block.");
    tableHeader_->table[index] = s_unusedBlokToken;
    --(tableHeader_->usedBlockCount);
}