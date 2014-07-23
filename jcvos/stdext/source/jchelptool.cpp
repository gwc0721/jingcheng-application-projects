#include "../include/comm_define.h"
#include "../include/jchelptool.h"


using namespace stdext;

#define DIR_SEP_CHAR  '\\'

#define DIR_SEP_STR		"\\"

//static const char * EMPTY = "";


//int CSubBlockBase::ToRelativePath(LPCTSTR absolute_path, CString & relative_path)
/*
int CJCHelpTool::PathAbs2Rel(LPCTSTR cur_path, LPCTSTR abs_path, CJCStringT & rel_path)
{
	// 从绝对路径解析得到相对于文档的路径，返回doc_path需要回朔的层数，如果无法取得相对路径，则返回-1

	// 取得文档的路径
//	CString _doc_path;
//	CUIDevice * device = dynamic_cast<CUIDevice *>(this);		ASSERT(device);
//	CVSContainer * cont = device->GetContainer();				ASSERT(cont);
//	cont->GetPathName(_doc_path);
//	_doc_path.MakeLower();
//	LPCTSTR cur_path = _doc_path;

	int cur_path_len = (int)_tcsclen(cur_path), abs_path_len = (int)_tcsclen(abs_path);
	if (cur_path[0] != abs_path[0])
	{
		rel_path = abs_path;
		return -1;	// 驱动器盘符不同, 取绝对路径
	}

	int path_len = std::min(cur_path_len, abs_path_len);
	int diff = 0;		// 第一次出现不同的路径的位置
	for (int i=0; (i<path_len) && (cur_path[i] == abs_path[i]) ; i++)
		if (cur_path[i] == _T( DIR_SEP_CHAR)) diff = i;
	
	int back_layer = 0;
	for (int i = diff+1; i< cur_path_len; i++)
		if (cur_path[i] == _T( DIR_SEP_CHAR)) back_layer ++;

	rel_path.clear();
	for (int i=0; i<back_layer; i++)	rel_path += _T("..")_T(DIR_SEP_STR);

	// 第一个字符是 '\'
	rel_path += abs_path + diff + 1;
	
	return back_layer;
}
*/
int CJCHelpTool::PathRel2Abs(LPCTSTR cur_path, LPCTSTR rel_path, CJCStringT & abs_path)
{
	/*
	// 从相对路径得到绝对路径

	// 取得文档的路径
	//CUIDevice * device = dynamic_cast<CUIDevice *>(this);		ASSERT(device);
	//CVSContainer * cont = device->GetContainer();				ASSERT(cont);
	//cont->GetPathName(abs_path);

	// 判断rel_path是否是绝对路径
	int i = abs_path.ReverseFind(_T( DIR_SEP_CHAR));
	if ( (rel_path[1] == _T(':')) || (i<0) )
	{
		abs_path = rel_path;
		return;
	}
	
	LPCTSTR rpath = rel_path;
	abs_path = abs_path.Left(i);
	do
	{
		if ( _tcsncicmp(rpath, _T("..\\"), 3) == 0)
		{
			// 处理一层相对路径
			rpath += 3;
			i = abs_path.ReverseFind(_T( DIR_SEP_CHAR));
			if (i<0) break;
			abs_path = abs_path.Left(i);
		}
		else break;
	}
	while (rpath[0] != 0);
	abs_path += _T("\\");
	abs_path += rpath;
*/
	return 0;
}


void stdext::UnicodeToUtf8(CJCStringA & dest, const wchar_t *szSrc, JCSIZE nSrcLen)
{
	JCSIZE src_len = (nSrcLen > 0)? nSrcLen : (JCSIZE)wcslen(szSrc);
	JCSIZE dest_len = src_len * 3 + 1;

	dest.resize(dest_len);
	UnicodeToUtf8((char*)dest.data(), dest_len, szSrc, src_len);
}

#define BYTE_1_REP          0x80    /* if <, will be represented in 1 byte */
#define BYTE_2_REP          0x800   /* if <, will be represented in 2 bytes */

/* If the unicode value falls on or between these values, it will be
   represented as 4 bytes
*/
#define SURROGATE_MIN       0xd800
#define SURROGATE_MAX       0xdfff

int stdext::UnicodeToUtf8(char *strDest, JCSIZE nDestLen, const wchar_t *szSrc, JCSIZE nSrcLen)
{
    JCSIZE i = 0;
    JCSIZE i_cur_output = 0;
    char ch_tmp_byte;

    // We cannot append terminate 0 at this case.
    if (nDestLen == 0)		return 0; /* ERROR_INSUFFICIENT_BUFFER */

    for (i = 0; i < nSrcLen; i++)
    {
        if (BYTE_1_REP > szSrc[i]) /* 1 byte utf8 representation */
        {
            if (i_cur_output + 1 < nDestLen)
            {
                strDest[i_cur_output++] = (char)szSrc[i];
            }
            else
            {
                strDest[i_cur_output] = '\0'; /* Terminate string */
                return 0; /* ERROR_INSUFFICIENT_BUFFER */
            }
        }
        else if (BYTE_2_REP > szSrc[i]) /* 2 byte utf8 representation */
        {
            if (i_cur_output + 2 < nDestLen)
            {
                strDest[i_cur_output++] = (char)(szSrc[i] >> 6 | 0xc0);
                strDest[i_cur_output++] = (char)((szSrc[i] & 0x3f) | 0x80);
            }
            else
            {
                strDest[i_cur_output] = '\0'; /* Terminate string */
                return 0; /* ERROR_INSUFFICIENT_BUFFER */
            }
        }
        else if (SURROGATE_MAX > szSrc[i] && SURROGATE_MIN < szSrc[i])
        {        /* 4 byte surrogate pair representation */
            if (i_cur_output + 4 < nDestLen)
            {
                ch_tmp_byte = (char)(((szSrc[i] & 0x3c0) >> 6) + 1);
                strDest[i_cur_output++] = (char)(ch_tmp_byte >> 2 | 0xf0);
                strDest[i_cur_output++] = (char)(((ch_tmp_byte & 0x03) | 0x80) | (szSrc[i] & 0x3e) >> 2);
                /* @todo Handle surrogate pairs */
            }
            else
            {
                strDest[i_cur_output] = '\0'; /* Terminate string */
                return 0; /* ERROR_INSUFFICIENT_BUFFER */
            }
        }
        else /* 3 byte utf8 representation */
        {
            if (i_cur_output + 3 < nDestLen)
            {
                strDest[i_cur_output++] = (char)(szSrc[i] >> 12 | 0xe0);
                strDest[i_cur_output++] = (char)(((szSrc[i] >> 6)  & 0x3f) | 0x80);
                strDest[i_cur_output++] = (char)((szSrc[i] & 0x3f) | 0x80);
            }
            else
            {
                strDest[i_cur_output] = '\0'; /* Terminate string */
                return 0; /* ERROR_INSUFFICIENT_BUFFER */
            }
        }
    }

    strDest[i_cur_output] = '\0'; /* Terminate string */

    return i_cur_output; /* This value is in bytes */
}

JCSIZE stdext::Utf8ToUnicode(wchar_t *strDest, JCSIZE nDestLen, const char *szSrc, JCSIZE nSrcLen)
{
#ifdef WIN32
	return MultiByteToWideChar(CP_UTF8, 0, szSrc, nSrcLen, strDest, nDestLen); 
#else
	return 0;
#endif
}



INT64 stdext::str2hex(LPCTSTR str, JCSIZE dig)
{
	LPTSTR str_end = NULL;
	return _tcstoi64(str, &str_end, 16);
}

void stdext::itohex(LPTSTR str, JCSIZE dig, UINT d)
{
	str[dig] = 0;
	JCSIZE ii = dig;
	do
	{
		--ii;
		str[ii] = hex2char(d & 0xF);
		d >>= 4;
	}
	while (ii > 0);
}

