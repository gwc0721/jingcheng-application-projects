#pragma once

#include "iimage_proc.h"

bool CreatePreProcessor(const CJCStringT & src_fn, IImageProcessor * & proc);
bool CreateProcessor(const CJCStringT & proc_name, IImageProcessor * & proc);
