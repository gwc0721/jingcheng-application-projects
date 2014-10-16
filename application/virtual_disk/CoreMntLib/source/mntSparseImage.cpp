#include "../include/mntSparseImage.h"
#include <assert.h>

SparseImage::SparseImage(std::auto_ptr<IImage> impl, ULONG32 headerOffset, 
                               ULONG64 logicSize, ULONG32 blockSize, ULONG32 granulity, bool fCreate):
    impl_(impl),
    headerOffset_(headerOffset),
    logicSize_(logicSize), blockSize_(blockSize),
    indexTable_(logicSize, blockSize, granulity)
{
    assert(headerOffset%granulity == 0);
    assert(blockSize%granulity == 0);
    if(fCreate)
        WriteTable();
    else
        ReadTable();
}
void SparseImage::WriteTable()
{
    impl_->Write(indexTable_.getBuf(), headerOffset_, indexTable_.getSize());
}
void SparseImage::ReadTable()
{
    impl_->Read(indexTable_.getBuf(), headerOffset_, indexTable_.getSize());
    indexTable_.getLogBlock();
}
void SparseImage::ReadFromBlock(char* pBuffer, ULONG32 blockIndex, ULONG32 offset, ULONG32 size)
{
    ULONG32 blockToken = indexTable_.at(blockIndex);
    assert(blockToken != s_unusedBlokToken);
    ULONG64 read_offset = headerOffset_ + indexTable_.getSize() + blockToken * blockSize_ + offset;
    impl_->Read(pBuffer, read_offset, size);
}
void SparseImage::WriteToBlock(const char* pBuffer, ULONG32 blockIndex, ULONG32 offset, ULONG32 size)
{
    ULONG32 blockToken = indexTable_.at(blockIndex);
    assert(blockToken != s_unusedBlokToken);
    ULONG64 write_offset = headerOffset_ + indexTable_.getSize() + blockToken * blockSize_ + offset;
    impl_->Write(pBuffer, write_offset, size);
}
void SparseImage::AddNewBlock(ULONG32 index)
{
    ULONG32 usedCount = indexTable_.getUsedCount();
    indexTable_.set(index, usedCount);
    try
    {
        WriteTable();
    }
    catch(...)
    {
        indexTable_.reset(index);
        throw;
    }
}
bool SparseImage::Read(char*  pBuffer,
                          ULONG64 offset,
                          ULONG32 length)
{
    ULONG32 firstBlockIndex = boost::numeric_cast<ULONG32>(offset / blockSize_);
    ULONG32 lastBlockIndex = boost::numeric_cast<ULONG32>((offset + length) / blockSize_);
    ULONG32 firstBlockOffset = offset % blockSize_;
    ULONG32 lastBlockOffset = (offset + length) % blockSize_;

    ULONG32 bytesWriten = 0;
    for(ULONG32 currentBlockIndex = firstBlockIndex;
        currentBlockIndex <= lastBlockIndex && bytesWriten < length;
        ++currentBlockIndex)
    {
        if(currentBlockIndex == lastBlockIndex && lastBlockOffset == 0)
            break;
        ULONG32 currentBlockToken = indexTable_.at(currentBlockIndex);
        if(currentBlockToken != s_unusedBlokToken)
        {
            ULONG32 begin = 0;
            ULONG32 end = blockSize_;
            if(currentBlockIndex == firstBlockIndex)
                begin = firstBlockOffset;
            if(currentBlockIndex == lastBlockIndex)
                end = lastBlockOffset;
            assert(end >= begin);
            ReadFromBlock(pBuffer, currentBlockIndex, begin, end - begin);

            pBuffer += (end - begin);
            bytesWriten += (end - begin);
        }
    }
	return true;
}

bool SparseImage::Write(const char*   pBuffer,
                           ULONG64 offset,
                           ULONG32 length)
{
    ULONG32 firstBlockIndex = boost::numeric_cast<ULONG32>(offset / blockSize_);
    ULONG32 lastBlockIndex = boost::numeric_cast<ULONG32>((offset + length) / blockSize_);
    ULONG32 firstBlockOffset = offset % blockSize_;
    ULONG32 lastBlockOffset = (offset + length) % blockSize_;

    ULONG32 bytesWriten = 0;
    for(ULONG32 currentBlockIndex = firstBlockIndex;
        currentBlockIndex <= lastBlockIndex && bytesWriten < length;
        ++currentBlockIndex)
    {
        ULONG32 currentBlockToken = indexTable_.at(currentBlockIndex);
        if(currentBlockToken == s_unusedBlokToken)
            AddNewBlock(currentBlockIndex);

        ULONG32 begin = 0;
        ULONG32 end = blockSize_;
        if(currentBlockIndex == firstBlockIndex)
            begin = firstBlockOffset;
        if(currentBlockIndex == lastBlockIndex)
            end = lastBlockOffset;
        assert(end >= begin);
        WriteToBlock(pBuffer, currentBlockIndex, begin, end - begin);

        pBuffer += (end - begin);
        bytesWriten += (end - begin);
    }
	return true;
}
