#ifndef MNT_SPARSE_IMAGE_H_INCLUDED
#define MNT_SPARSE_IMAGE_H_INCLUDED
#include <vector>
#include <set>
#include "mntImage.h"
#include "ResparseIndexTable.h"
class SparseImage: public IImage
{
    std::auto_ptr<IImage> impl_;

    const ULONG64 logicSize_;
    const ULONG32 blockSize_;
    const ULONG32 headerOffset_;

    CResparseIndexTable indexTable_;

    void WriteTable();
    void ReadTable();
    void AddNewBlock(ULONG32 index);
    void ReadFromBlock(char* pBuffer, ULONG32 blockIndex, ULONG32 offset, ULONG32 size);
    void WriteToBlock(const char* pBuffer, ULONG32 blockIndex, ULONG32 offset, ULONG32 size);

    SparseImage();
    SparseImage(const SparseImage&);
    SparseImage operator=(const SparseImage&);
public:
    SparseImage::SparseImage(std::auto_ptr<IImage> impl, ULONG32 headerOffset, 
        ULONG64 logicSize, ULONG32 blockSize, ULONG32 granulity, bool fCreate);
    bool Read(char* out_buf, ULONG64 offset, ULONG32 bytesToRead);
    bool Write(const char*   pBuffer,ULONG64 offset,ULONG32 length);

    ULONG64 GetSize() const
    {
        return logicSize_;
    }
};

#endif