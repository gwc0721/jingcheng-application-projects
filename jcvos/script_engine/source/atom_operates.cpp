﻿#include "stdafx.h"
#include "../include/syntax_parser.h"
#include "atom_operates.h"

// LOG 输出定义：
//  WARNING:	运行错误
//	NOTICE:		script运行trace
//  TRACE:		调用堆栈

LOCAL_LOGGER_ENABLE(_T("atom_operates"), LOGGER_LEVEL_WARNING);

using namespace jcscript;

///////////////////////////////////////////////////////////////////////////////
//-- CAssignOp
LOG_CLASS_SIZE(CAssignOp)

const TCHAR CAssignOp::m_name[] = _T("assign");

CAssignOp::CAssignOp(IAtomOperate * dst_op, const CJCStringT & dst_name)
	: m_dst_op(dst_op)
	, m_dst_name(dst_name)
{
	LOG_STACK_TRACE();
	JCASSERT(m_dst_op);
	m_dst_op->AddRef();
	//strcpy_s(_class_name, "Assign");
}

CAssignOp::~CAssignOp(void)
{
	LOG_STACK_TRACE();
	if (m_dst_op) m_dst_op->Release();
}

bool CAssignOp::GetResult(jcparam::IValue * & val)
{
	// 应当不会被调用
	JCASSERT(0);
	return false;
}

bool CAssignOp::Invoke(void)
{
	LOG_STACK_TRACE();
	JCASSERT(m_src[0]);
	JCASSERT(m_dst_op);

	jcparam::IValue *val = NULL;
	m_src[0]->GetResult(val);

	if (val)
	{
#if 1
		// 赋值的实现1: m_dst_op基于IAtomOperate的界面
		jcparam::IValue * val_dst = NULL;
		m_dst_op->GetResult(val_dst);
		val_dst->SetSubValue(m_dst_name.c_str(), val);
		val_dst->Release();
#else
		// 赋值的实现2：m_dst_op基于IVariableSet / IFeature的界面
		m_dst_op->SetVariable(m_dst_name, val);
		// or
		m_dst_op->PushParameter(m_dst_name, val);
#endif
		val->Release();
	}

	return true;
}

void CAssignOp::DebugInfo(FILE * outfile)
{
	stdext::jc_fprintf(outfile, _T("dst=<%08X>, var=%s"),
		(UINT)(m_dst_op), m_dst_name.c_str());
}

///////////////////////////////////////////////////////////////////////////////
//-- CPushParamOp
LOG_CLASS_SIZE(CPushParamOp)
const TCHAR CPushParamOp::m_name[] = _T("push_param");

CPushParamOp::CPushParamOp(IFeature * dst_op, const CJCStringT & dst_name)
	: m_function(dst_op)
	, m_param_name(dst_name)
{
	LOG_STACK_TRACE();
	JCASSERT(m_function);
	m_function->AddRef();
}

CPushParamOp::~CPushParamOp(void)
{
	LOG_STACK_TRACE();
	if (m_function) m_function->Release();
}

bool CPushParamOp::GetResult(jcparam::IValue * & val)
{
	// 应当不会被调用
	JCASSERT(0);
	return false;
}

bool CPushParamOp::Invoke(void)
{
	JCASSERT(m_src[0]);
	JCASSERT(m_function);

	LOG_SCRIPT(_T("<%08X> -> <%08X>:%s"), 
		(UINT)(m_src[0]), (UINT)(m_function), m_param_name.c_str() );

	jcparam::IValue *val = NULL;
	m_src[0]->GetResult(val);

	m_function->PushParameter(m_param_name, val);
	val->Release();
	return true;
}

void CPushParamOp::DebugInfo(FILE * outfile)
{
	stdext::jc_fprintf(outfile, _T("func=<%08X>, param=%s"),
		(UINT)(m_function), m_param_name.c_str());
}

///////////////////////////////////////////////////////////////////////////////
//-- CSaveToFileOp
LOG_CLASS_SIZE(CSaveToFileOp)
const TCHAR CSaveToFileOp::m_name[] = _T("save_file");

CSaveToFileOp::CSaveToFileOp(const CJCStringT & filename)
	: m_file_name(filename)
	, m_file(NULL)
{
}
		
CSaveToFileOp::~CSaveToFileOp(void)
{
	if (m_file)		fclose(m_file);
}

bool CSaveToFileOp::Invoke(void)
{
	LOG_SCRIPT(_T(""));
	// 支持逐次运行
	JCASSERT(m_src[0]);

	// 行写入
	stdext::auto_interface<jcparam::IValue> val;
	m_src[0]->GetResult(val);
	if ( !val) 
	{	// warnint
		return true;
	}
	stdext::auto_cif<jcparam::IValueFormat>	format;
	val->QueryInterface(jcparam::IF_NAME_VALUE_FORMAT, format);
	if ( ! format)
	{	// !! warning
		return true;
	}

	if (NULL == m_file)
	{	// 初始化
		_tfopen_s(&m_file, m_file_name.c_str(), _T("w+b"));
		if ( !m_file ) THROW_ERROR(ERR_PARAMETER, 
			_T("failure on openning file %s"), m_file_name.c_str());
		format->WriteHeader(m_file);
	}

	format->Format(m_file, _T("") );
	return true;
}

void CSaveToFileOp::DebugInfo(FILE * outfile)
{
	stdext::jc_fprintf(outfile, _T("fn=%s"), m_file_name.c_str());
}

///////////////////////////////////////////////////////////////////////////////
//-- CExitOp
LOG_CLASS_SIZE(CExitOp)

bool CExitOp::Invoke(void)
{
	_tprintf( _T("Bye bye!\r\n") );
	throw CExitException();
	return false;
}

const TCHAR CExitOp::name[] = _T("exit");

///////////////////////////////////////////////////////////////////////////////
//-- feature wrapper
LOG_CLASS_SIZE(CFeatureWrapper)
const TCHAR CFeatureWrapper::m_name[] = _T("feature");

CFeatureWrapper::CFeatureWrapper(IFeature * feature)
	: m_feature(feature)
	, m_outport(NULL)
{
	JCASSERT(m_feature);
	m_feature->AddRef();
}

CFeatureWrapper::~CFeatureWrapper(void)
{
	JCASSERT(m_feature);
	m_feature->Release();
}

bool CFeatureWrapper::GetResult(jcparam::IValue * & val)
{
	return false;
}

bool CFeatureWrapper::Invoke(void)
{
	LOG_SCRIPT(_T("<%08X>"), (UINT)(m_feature) )
	stdext::auto_interface<jcparam::IValue> val;
	if (m_src[0])	m_src[0]->GetResult(val);
	return m_feature->Invoke(val, m_outport);
}

void CFeatureWrapper::SetOutPort(IOutPort * outport)
{
	JCASSERT(NULL == m_outport);
	m_outport = outport;
}

void CFeatureWrapper::DebugInfo(FILE * outfile)
{
	stdext::jc_fprintf(outfile, _T(": %s, <%08X>"), m_feature->GetFeatureName(), (UINT)(m_feature) );
}

///////////////////////////////////////////////////////////////////////////////
//-- filter statement
LOG_CLASS_SIZE(CFilterSt);
const TCHAR CFilterSt::m_name[] = _T("filter");

CFilterSt::CFilterSt(IOutPort * out_port)
	: m_init(false)
	, m_outport(out_port)
{
	// m_outport指向filter的上层，一般为single_st。不能引用计数，否则会引起release的死锁。
	JCASSERT(m_outport);
}
		
CFilterSt::~CFilterSt(void)
{
}

bool CFilterSt::Invoke(void)
{
	JCASSERT(m_outport);
	// src0: the source of bool expression.
	// src1: the source of table

	stdext::auto_interface<jcparam::IValue> val;
	stdext::auto_interface<jcparam::IValue> _row;
	bool br = m_src[SRC_EXP]->GetResult(val);
	if ( br )
	{
		m_src[SRC_TAB]->GetResult(_row);
		jcparam::ITableRow * row = _row.d_cast<jcparam::ITableRow*>();
		if ( !row ) THROW_ERROR(ERR_USER, _T("The input of filter should be a row"));
		m_outport->PushResult(row);
	}
	LOG_SCRIPT(_T(": %s"), br?_T("true"):_T("false"));
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//-- CInPort
LOG_CLASS_SIZE(CInPort);

const TCHAR CInPort::m_name[] = _T("inport");

CInPort::CInPort(void)
	: m_row(NULL)
{
}

CInPort::~CInPort(void)
{
	if (m_row) m_row->Release();
}

bool CInPort::GetResult(jcparam::IValue * & val)
{
	JCASSERT(NULL == val);
	LOG_SCRIPT(_T(""));
	if (m_row)
	{
		val = static_cast<jcparam::IValue*>(m_row);
		val->AddRef();
		return true;
	}
	else	return false;

}

bool CInPort::Invoke(void)
{
	LOG_SCRIPT(_T(""));
	if (m_row)	m_row->Release(), m_row=NULL;
	bool br = m_src[0]->PopupResult(m_row);
	return br;
}

const TCHAR CHelpProxy::name[] = _T("help");