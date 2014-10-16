#ifndef RESPARSE_INDEX_TABLE_H_INCLUDED
#define RESPARSE_INDEX_TABLE_H_INCLUDED

#include <windows.h>

#include <vector>
#include <set>
//#include "mntCmn.h"
#include "boost/cast.hpp"

const static ULONG32 s_unusedBlokToken = 0xFFFFFFFF;

inline static ULONG32 s_integer_div(ULONG64 size, ULONG32 block)
{
    ULONG32 result = boost::numeric_cast<ULONG32>(size/block);
    if (size%block)
        ++result;
    return result;
}
inline static ULONG32 s_granulity_to(ULONG64 size, ULONG32 block)
{
    ULONG64 result = (size/block)*block;
    if (size%block)
        result += block;
    return boost::numeric_cast<ULONG32>(result);
}

#pragma pack(push, 1) 
#pragma warning(push)
#pragma warning(disable : 4200)
    struct IndexTableHeader
    {
        ULONG32 indexTableSize;
        ULONG32 usedBlockCount;
        ULONG32 firstLogBlock;
        ULONG32 currentLogBlock;
        ULONG32 currentLogOffset;
        ULONG32 table[0];
    };
#pragma warning(pop)
#pragma pack(pop) 

class CResparseIndexTable
{
    const ULONG32 dataOffset_;
    const ULONG32 blockSize_;
    std::vector<char> tableData_;
    CResparseIndexTable();
    CResparseIndexTable(const CResparseIndexTable&);
    const CResparseIndexTable& operator=(const CResparseIndexTable&);
    IndexTableHeader* tableHeader_;

    std::set<ULONG32> logBlocks_;

public:
    CResparseIndexTable(ULONG64 logicSize, ULONG32 blockSize, ULONG32 granulity);
    void getLogBlock();
    char* getBuf(){return &tableData_[0];}
    ULONG32 getSize(){return dataOffset_;}
    ULONG32 getUsedCount(){return tableHeader_->usedBlockCount;}
    ULONG64 getMessageBlocksSize();
    ULONG64 LogMessageOffset2FileOffset(ULONG64 virtualOffset);
    ULONG64 getNextLogMessagePosition(ULONG32 messageSize);
    ULONG32 at(ULONG32 index);
    void set(ULONG32 index, ULONG32 val);
    void reset(ULONG32 index);
};
#endif