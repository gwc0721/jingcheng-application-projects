// CSvtApplication 同时继承 Application和IPlugin两个Interface。
// 此源文件主要实现IPlugin部分，包括一些内部命令及其相应的成员变量的管理
//  主要包括：
//  1) 设备管理 ScanDev, DummyDev, ...
//  2) 变量管理 Assign, Show, Save, ...
//  3) 命令管理 Help, ...
//  4) 流程管理 Exit, ...

#include "stdafx.h"
#include <stdext.h>

LOCAL_LOGGER_ENABLE(_T("default_pi"), LOGGER_LEVEL_ERROR);
#include "application.h"

#define __CLASS_NAME__	CSvtApplication

const CCmdManager CSvtApplication::m_cmd_man(CCmdManager::RULE()
	COMMAND_DEFINE(_T("exit"), Exit, 0, 0, _T("Exit SmiVenderTest."))

/*
	COMMAND_DEFINE(_T("scandev"), ScanDevice, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME), 
		_T("Scan SMI device."),
		(_T("list"),		_T('l'),	jcparam::VT_STRING, _T("Give a scan list, like C~Z0~9.") )
		// 起始字符必须制定，结尾可以省略（单字符或者到最后）如:A~C, 6~, AB2~, C~0~
		(_T("driver"),		_T('d'),	jcparam::VT_STRING, _T("Force using specified driver.") )
		(_T("controller"),	_T('c'),	jcparam::VT_STRING, _T("Force using specified controller.") )
		// TODO: dummy smi device
		// TODO: dummy physical device
	)
*/

	COMMAND_DEFINE(_T("assign"), Assign, 0, 0, _T("<var_name> Assign value to variable."))

	COMMAND_DEFINE(_T("show"), ShowVariable, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Show content of variable in test format."),
		PARDEF(_T("format"),		m, VT_STRING,	_T("Foramt to show or save.") )
	)

	COMMAND_DEFINE(_T("save"), SaveVariable, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Save content of variable."),
		PARDEF(_T("filename"),	f, VT_STRING,	_T("File name for save.") )
		PARDEF(_T("format"),		m, VT_STRING,	_T("Foramt to show or save.") )
		PARDEF(_T("append"),		a, VT_BOOL,		_T("Append data to existing file.") )
	)

	COMMAND_DEFINE(_T("loadbin"),		LoadBinaryData, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Load binary data from file"),
		PARDEF(_T("filename"),	f, VT_STRING,	_T("file name.") )
		PARDEF(FN_SECTORS,		n, VT_STRING,	_T("Data size in sectors, default is file size.") )
	)

	COMMAND_DEFINE(_T("mkvector"),	MakeVector, 0, PARAM_PROP(PROP_MATCH_PARAM_NAME)
		, _T("Create a vector (one column table)."),
		(_T("name"),	_T('m'), jcparam::VT_INT, _T("Column name."))
		(_T("from"),	_T('f'), jcparam::VT_INT, _T("Start value."))
		(_T("step"),	_T('s'), jcparam::VT_INT, _T("Step, default is 1."))
		(_T("to"),		_T('t'), jcparam::VT_INT, _T("End value."))
		(_T("size"),	_T('n'), jcparam::VT_INT, _T("Element number in the vector."))
	)

	//( CCmdDef<CSvtApplication>(_T("list"),	&CSvtApplication::Assign,		0, _T("List out all variable") 
	//) )
	//( CCmdDef<CSvtApplication>(_T("remove"),	&CSvtApplication::Assign,		0, _T("Remove variable") 
	//) )
	
	// TODO: list all variables
	// TODO: delete variable
	);
bool CSvtApplication::Exit(jcparam::CArguSet &, jcparam::IValue *, jcparam::IValue * &)
{
	_tprintf( _T("Bye bye!\r\n") );
	throw CExitException();
	return true;
}

enum CHAR_TYPE
{
	CT_UNKNOW = 0,
	CT_WORD =		0x00000006,
	CT_NUMBER =		0x00000004,
	CT_ALPHABET =	0x00000002,
	CT_HYPHEN =		0x00000010,
};

static CHAR_TYPE CharType(TCHAR &ch)
{
	static const TCHAR LOWERCASE = _T('a') - _T('A');
	CHAR_TYPE type = CT_UNKNOW;
	if ( _T('a') <= ch &&  ch <= _T('z') )		ch -= LOWERCASE, type = CT_ALPHABET;
	else if ( _T('A') <= ch && ch <= _T('Z') )	type = CT_ALPHABET;
	else if ( _T('0') <= ch && ch <= _T('9') )	type = CT_NUMBER;
	else if ( _T('-') == ch || _T('~') == ch)	type = CT_HYPHEN;
	else THROW_ERROR(ERR_PARAMETER, _T("Unknow character %c"), ch);
	return type;
}

static void AddDeviceName(std::vector<CJCStringT> & list, TCHAR start, TCHAR end, CHAR_TYPE type)
{
	TCHAR device_name[32];
	JCASSERT(type & CT_WORD);
	JCASSERT(start);
	//if ( end < start) THROW_ERROR(ERR_PARAMETER, _T("Uncorrect letter range from %c to %c"), start, end);
	if (type & CT_ALPHABET)
	{
		if (0 == end ) end = _T('Z');
		JCASSERT( end - start < 26)
		if ( end < start) THROW_ERROR(ERR_PARAMETER, _T("Uncorrect letter range from %c to %c"), start, end);
		for (TCHAR cc = start; cc <= end; ++cc)
		{
			_stprintf_s(device_name, _T("\\\\.\\%c:"), cc);
			list.push_back(CJCStringT(device_name));
		}
	}
	else
	{
		if (0 == end ) end = _T('9');
		JCASSERT(type & CT_NUMBER);
		JCASSERT( end - start < 10);
		if ( end < start) THROW_ERROR(ERR_PARAMETER, _T("Uncorrect letter range from %c to %c"), start, end);
		for (TCHAR cc = start; cc<= end ; ++cc)
		{
			_stprintf_s(device_name, _T("\\\\.\\PhysicalDrive%c"), cc);
			list.push_back(CJCStringT(device_name));
		}	
	}

}

bool CSvtApplication::ScanDevice(jcparam::CArguSet & argu, jcparam::IValue *, jcparam::IValue * &)
{
	LOG_STACK_TRACE();

	std::vector<CJCStringT>	try_list;
	bool found = false;
	
	//ClearDevice();

	CJCStringT	str_list(_T("0~9")), force_storage, force_device;
	argu.GetValT(_T("list"), str_list);
	argu.GetValT(_T("driver"), force_storage);
	argu.GetValT(_T("controller"), force_device);

	if ( force_storage == _T("DUMMY") ) return true;

	enum STATUS
	{
		ST_START, ST_STARTED, ST_WAITING_END,
	};
	// Create a scan list
	LPCTSTR list = str_list.c_str();
	TCHAR start = 0, end = 0;
	CHAR_TYPE start_type = CT_UNKNOW;
	STATUS status = ST_START;
	while ( *list )
	{
		TCHAR ch = *list;
		CHAR_TYPE type = CharType(ch);
		switch (status)
		{
		case ST_START:
			if ( type & CT_WORD ) 
				status = ST_STARTED, end = ch, start = ch, start_type = type;
			else THROW_ERROR(ERR_PARAMETER, _T("Missing start character"));
			break;

		case ST_STARTED:
			if ( type & CT_WORD)
			{
				// process current letter
				AddDeviceName(try_list, start, start, start_type);
				end = ch;
				start = ch;
				start_type = type;
			}
			else if ( type & CT_HYPHEN)		status = ST_WAITING_END, end = 0;
			break;
		case ST_WAITING_END:
			if ( type & CT_WORD)
			{
				JCASSERT(start);
				JCASSERT(start_type);
				if ( type != start_type)
				{
					AddDeviceName(try_list, start, 0, start_type);
					start = ch;
					start_type = type;
					status = ST_STARTED;
				}
				else
				{
					AddDeviceName(try_list, start, ch, start_type);
					start = 0;
					status = ST_START;
				}
			}
			else	THROW_ERROR(ERR_PARAMETER, _T("Unexpected hyphen"));
		}
		list ++;
	}
	if (ST_START != status) 		AddDeviceName(try_list, start, end, start_type);


	std::vector<CJCStringT>::iterator it, endit=try_list.end();

	for (it = try_list.begin(); it != endit; ++it)
	{
		LOG_DEBUG(_T("Trying device %s"), it->c_str() )
		ISmiDevice * smi_dev = NULL;
		bool br = CSmiRecognizer::RecognizeDevice(it->c_str(), smi_dev, force_storage, force_device);
		if (br && smi_dev)
		{
			const CJCStringT & name (*it);
			SetDevice(name, smi_dev);
			smi_dev->Release();
			_tprintf(_T("Found device %s.\r\n"), name.c_str() );
			found = true;

			LOG_DEBUG(_T("Found device %s"), name.c_str() );
			break;
		}
	}

	if ( ! found) _tprintf(_T("Cannot find SMI device!\r\n"));

	return true;
}

bool CSvtApplication::Assign(jcparam::CArguSet & argu, jcparam::IValue * var_in, jcparam::IValue * &var_out)
{
	JCASSERT(var_out == NULL);
	// behavior: 如果变量var_name存在，并且var_in != NULL，将var_in赋值给var_name, var_out = var_in
	//			如果变量var_name存在，并且var_in == NULL, 取var_name的值赋给var_out
	//			如果var_name不存在，且var_in != NULL, 添加变量var_name
	//			如果var_name不存在，且var_in == NULL, 错误
	// trim var_name
	static const jcparam::CCmdLineParser cmd_parser;
	
	CJCStringT str_name;
	argu.GetCommand(0, str_name);

	if (NULL == var_in)
	{
		_tprintf(_T("No input value.\r\n"));
		return false;
	}

	var_out = var_in;
	var_out->AddRef();

	if (str_name.empty()) 
	{
		_tprintf(_T("missing variable name.\r\n"));
		return false;
	}

	m_plugin_manager.GetVariableSet()->SetSubValue(str_name.c_str(), var_in);

	//if ( var_in )
	//{
	//	m_variables->SetSubValue(str_name.c_str(), var_in);
		//var_out = var_in;
		//var_out->AddRef();
	//}
	//else
	//{
	//	m_variables->GetSubValue(str_name.c_str(), var_out);
	//	if ( !var_out ) THROW_ERROR(ERR_PARAMETER, _T("variable %s is not exist"), str_name.c_str());
	//}

	return true;
}

bool CSvtApplication::OutputVariable(FILE * file, jcparam::IValue * var_in, LPCTSTR str_type)
{
	LOG_STACK_TRACE();
	stdext::auto_cif<jcparam::IValueFormat>	format;
	var_in->QueryInterface(jcparam::IF_NAME_VALUE_FORMAT, format);
	if (format)
	{
		format->Format(file, str_type);
	}
	else
	{
		LOG_ERROR(_T("Error! Format interface not support."));
		_tprintf(_T("Error! Format interface not support.\r\n"));
	}
	return true;
}

bool CSvtApplication::ShowVariable(jcparam::CArguSet & argu, jcparam::IValue * var_in, jcparam::IValue * &var_out)
{
	if (!var_in)
	{
		_tprintf(_T("No variable.\r\n"));
		return true;
	}
	CJCStringT str_format(_T("text"));
	argu.GetValT(_T("format"), str_format);
	OutputVariable(stdout, var_in, str_format.c_str() );
	stdext::jc_fprintf(stdout, _T("\n"));

	return true;
}

bool CSvtApplication::SaveVariable(jcparam::CArguSet & argu, jcparam::IValue * var_in, jcparam::IValue * &var_out)
{
	LOG_STACK_TRACE();
	if (!var_in)
	{
		_tprintf(_T("No variable.\r\n"));
		return false;
	}

	var_out = var_in;
	var_out->AddRef();

	CJCStringT str_filename,  str_format;
	argu.GetValT(_T("filename"), str_filename);
	if ( str_filename.empty() )
	{
		_tprintf(_T("Missing file name.\r\n"));
		return false;
	}
	argu.GetValT(_T("format"), str_format);

	bool append = argu.Exist(_T("append"));

	LOG_TRACE(_T("Start saving file ..."));
	FILE *file = NULL;
	if (append) 	_tfopen_s(&file, str_filename.c_str(), _T("a+b"));
	else			_tfopen_s(&file, str_filename.c_str(), _T("w+b"));
	//if (append) fseek(file, 0, SEEK_END);

	OutputVariable(file, var_in, str_format.c_str() );
	
	fclose(file);
	return true;
}

bool CSvtApplication::LoadBinaryData(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	JCASSERT(NULL == varout);

	CJCStringT filename;
	argu.GetValT(_T("filename"), filename);
	if ( filename.empty() ) THROW_ERROR(ERR_PARAMETER, _T("Missing file name.") );

	FILE * file = NULL;
	jcerrno err = stdext::jc_fopen(&file, filename.c_str(), _T("r"));
	if ( 0 != err || !file) THROW_ERROR(ERR_PARAMETER, _T("Failure on openning file %s"), filename.c_str() );
	// Get file size
	if ( 0 != fseek(file, 0, SEEK_END) ) THROW_ERROR(ERR_PARAMETER, _T("Failure on getting file size") );
	JCSIZE file_size  = ftell(file);
	fseek(file, 0, SEEK_SET);

	JCSIZE secs = 0;
	argu.GetValT(FN_SECTORS, secs);
	JCSIZE data_size = secs * SECTOR_SIZE;
	if ( 0 == data_size ) data_size = file_size;

	stdext::auto_interface<CBinaryBuffer> buf;
	CBinaryBuffer::Create(data_size, buf);
	BYTE * _buf = buf->Lock();
	memset(_buf, 0, data_size);

	fread(_buf, 1, data_size, file);

	buf->Unlock();
	fclose(file);

	buf.detach(varout);
	return true;
}

bool CSvtApplication::MakeVector(jcparam::CArguSet & argu, jcparam::IValue * varin, jcparam::IValue * & varout)
{
	//CJCStringT name;
	//argu.GetValT(_T("name"), name);

	int val_start = 0;
	argu.GetValT(_T("from"), val_start);

	int val_step = 1;
	argu.GetValT(_T("step"), val_step);
	if (val_step == 0)	THROW_ERROR(ERR_PARAMETER, _T("Step should not be zero."));

	// 考虑到处理step >0和 <0两种情况，统一计算size更方便。
	int size = 0;
	int val_stop = 1;
	if ( argu.GetValT(_T("to"), val_stop) )
	{
		// if defined stop valule, then calculate size
		// step <0时可能有误差。
		size = (val_stop - val_start - 1) / val_step + 1;
	}
	else
	{
		if ( !argu.GetValT(_T("size"), size) )	THROW_ERROR(ERR_PARAMETER, _T("Missing stop or size."));
	}
	if (size < 0) THROW_ERROR(ERR_PARAMETER, _T("size should be not less than 0."));

	stdext::auto_interface<jcparam::CTypedColumn<int> > col;
	jcparam::CTypedColumn<int>::Create(size, col);

	int ii =0, val = 0;

	// consider stop < start
	for (ii = 0, val = val_start; ii < size; ++ii, val += val_step)
	{
		col->push_back(val);
	}

	col.detach(varout);
	return true;
}