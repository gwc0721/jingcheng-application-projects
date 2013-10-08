#include "stdafx.h"

#include <boost/spirit/include/qi.hpp>     
#include <boost/spirit/include/phoenix.hpp>     
#include <boost/bind.hpp>

#include "filead_csv.h"

LOCAL_LOGGER_ENABLE(_T("op_bus_doctor"), LOGGER_LEVEL_DEBUGINFO);

using namespace jcparam;


///////////////////////////////////////////////////////////////////////////////
//-- csv row

CCsvRow::CCsvRow(void)
{
}

CCsvRow::~CCsvRow(void)
{
}

void CCsvRow::GetSubValue(LPCTSTR name, IValue * & val)
{
}

void CCsvRow::SetSubValue(LPCTSTR name, IValue * val)
{
}

JCSIZE CCsvRow::GetRowID(void) const
{
	return 0;
}

int CCsvRow::GetColumnSize() const
{
	return 0;
}

void CCsvRow::GetColumnData(int field, jcparam::IValue * &)	const
{
}

const jcparam::CColInfoBase * CCsvRow::GetColumnInfo(int field) const
{
	return NULL;
}

// 从row的类型创建一个表格
bool CCsvRow::CreateTable(jcparam::ITable * & tab)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////
//-- csv table
CCsvTable::CCsvTable(void)
{
	m_total_length = 0;
}

CCsvTable::~CCsvTable(void)
{
	row_iterator it, endit = m_rows.end();
	for ( it = m_rows.begin(); it != endit; ++ it)
	{
		CCsvRow * row = *it;
		JCASSERT(row);
		row->Release();
	}
}

void CCsvTable::GetSubValue(LPCTSTR name, IValue * & val)
{

}

void CCsvTable::SetSubValue(LPCTSTR name, IValue * val)
{
	JCASSERT(0);
	THROW_ERROR(ERR_UNSUPPORT, _T("unsupport"));
}

bool CCsvTable::Create(CCsvTable * & table)
{
	JCASSERT(NULL == table);
	table = new CCsvTable;
	return true;
}

JCSIZE CCsvTable::GetRowSize() const
{
	return m_rows.size();
}

void CCsvTable::GetRow(JCSIZE index, IValue * & row)
{
	JCASSERT(NULL == row);
	JCASSERT(index < m_rows.size());
	CCsvRow * csv_row = m_rows.at(index);
	row = static_cast<IValue *>(csv_row);
	row -> AddRef();
}

//virtual void NextRow(IValue * & row) = 0;

JCSIZE CCsvTable::GetColumnSize() const
{
	return m_cols_info.GetSize();
}

const jcparam::CColInfoBase * CCsvTable::GetColInfo(JCSIZE col) const
{
	return m_cols_info.GetItem(col);
}

void CCsvTable::Append(IValue * source)
{
}

void CCsvTable::AddRow(ITableRow * row)
{
	CCsvRow * csv_row = dynamic_cast<CCsvRow*>(row);
	if (!row ) THROW_ERROR(ERR_PARAMETER, _T("row type does not match") );
	m_rows.push_back(csv_row);
}

void CCsvTable::AddColumn(const CJCStringT & name, jcparam::VALUE_TYPE type, JCSIZE length)
{
	if ( name.empty() ) return;

	switch (type)
	{
	case VT_STRING:
		if (length == 0) length = 64;	// default length for string
		break;

	case jcparam::VT_INT:
		length = 4;
		break;

	case VT_HEX:
		length = 4;
		break;

	case VT_FLOAT:
		if (length > 4) length = 8;
		else			length = 4;
		break;

	};

	jcparam::CColInfoBase * col_info = static_cast<CColInfoBase*>(
		new CCsvColInfo(name, type, length, m_total_length) );

	m_cols_info.AddItem(col_info);
	m_total_length += length;
}

///////////////////////////////////////////////////////////////////////////////
//-- CCsvReaderOp
CCsvReaderOp::CCsvReaderOp(const CJCStringT & file_name)
	: m_src_fn(file_name)
	, m_src_file(NULL)
	, m_table(NULL)
	, m_line_gm(NULL)
{
}


CCsvReaderOp::	~CCsvReaderOp(void)
{
	if (m_table) m_table->Release();
	if (m_src_file)	fclose(m_src_file);
	delete m_line_gm;
}


bool CCsvReaderOp::GetResult(jcparam::IValue * & val)
{
	return false;
}

bool CCsvReaderOp::Invoke(void)
{
	Init();
	for (; InvokeOnce() == true; );
	return true;
}


void CCsvReaderOp::SetSource(UINT src_id, IAtomOperate * op)
{
	JCASSERT(0);
}

void CCsvReaderOp::GetProgress(JCSIZE cur_prog, JCSIZE total_prog) const
{

}


namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

template <typename ITERATOR> 
class csv_header_gm : public qi::grammar<ITERATOR, unsigned(), ascii::space_type>
{
public:
	csv_header_gm() : csv_header_gm::base_type(rule_header)
		, col_type(VT_STRING)
		, col_length(0)
		, table(NULL)
	{
		using qi::lit;
		using ascii::char_;
		using qi::_1;
		using qi::string;
		using qi::int_;
		using qi::double_;
		using qi::float_;
		using qi::alpha;
		using qi::hex;
		using qi::lexeme;

		using namespace qi::labels;
		using boost::phoenix::ref;

		rule_type.add("string", jcparam::VT_STRING)("hex", jcparam::VT_HEX)("decimal", jcparam::VT_INT)
			("float", VT_FLOAT);

		rule_header = (rule_col [boost::bind(&csv_header_gm::save, this)])
			>> *(lit(',') >> (rule_col [boost::bind(&csv_header_gm::save, this)]) );

		rule_name %= lexeme[+(char_ - (lit(':') | lit(',') ) )];

		rule_col = ( rule_name [ref(col_name) = _1])
			>> -(lit(':') >> (rule_type [ref(col_type) = _1])
			>> -(lit(':') >> (int_ [ref(col_length) = _1]) ) );
	};

	qi::rule<ITERATOR, unsigned(), ascii::space_type> 
		rule_header, rule_col;
	qi::rule<ITERATOR, CJCStringT(), ascii::space_type> rule_name;

	void clear(void)
	{
		col_type = VT_STRING;	// default
		col_length = 0;		// default
		table = NULL;
	}

	void save(void)
	{
		JCASSERT(table);
		table->AddColumn(col_name, col_type, col_length);
		col_type = VT_STRING;	// default
		col_length = 0;		// default
	};

	qi::symbols<char, VALUE_TYPE> rule_type;
	CJCStringT col_name;
	VALUE_TYPE col_type;
	JCSIZE col_length;
	CCsvTable * table;
};

static	csv_header_gm<CJCStringA::const_iterator> gm_csv; 

template <typename ITERATOR> 
class csv_line_gm : public qi::grammar<ITERATOR, unsigned(), ascii::space_type>
{
public:
	void setcol_str(JCSIZE col, const CJCStringT & v)
	{
		LOG_DEBUG(_T("col %d"), col);
	}

	void setcol_int(JCSIZE col, INT64 v)
	{
		LOG_DEBUG(_T("col %d"), col);
		lba= v;
	}

	void setcol_float(JCSIZE col, double v)
	{
		LOG_DEBUG(_T("col %d"), col);
		time_stamp = v;
	}

	csv_line_gm() : csv_line_gm::base_type(rule_line)
		//, col_type(VT_STRING)
		//, col_length(0)
		//, table(NULL)
	{
		using qi::lit;
		using ascii::char_;
		using qi::_1;
		using qi::string;
		using qi::int_;
		using qi::double_;
		using qi::float_;
		using qi::alpha;
		using qi::hex;
		using qi::lexeme;

		using namespace qi::labels;
		using boost::phoenix::ref;


		JCSIZE ii = 0;

		//rule_line = 
		//	(col_int [boost::bind(&csv_line_gm::setcol_int, this, (JCSIZE)(0), _1)])
		//	>> (float_ [boost::bind(&csv_line_gm::setcol_float, this, (JCSIZE)(1), _1)])
		//	>> (float_ [boost::bind(&csv_line_gm::setcol_float, this, (JCSIZE)(2), _1)])
		//	>> (hex [boost::bind(&csv_line_gm::setcol_int, this, (JCSIZE)(3), _1)])
		//	>> (hex [boost::bind(&csv_line_gm::setcol_int, this, (JCSIZE)(4), _1)])
		//	>> (col_int [boost::bind(&csv_line_gm::setcol_int, this, (JCSIZE)(5), _1)])
		//	;

		//rule_line = 
		//	(col_int [ref(store) = _1]) >> ","
		//	>> (float_ [ref(time_stamp) = _1]) >> ","
		//	>> (float_ [ref(busy_time) = _1]) >> ","
		//	>> (hex [ref(cmd_code) = _1]) >> ","
		//	>> (hex [ref(lba) = _1]) >> ","
		//	>> (col_int [ref(sec) = _1])
		//	;
		add_int(ii++);
		add_float(ii++);
		add_float(ii++);
		add_hex(ii++);
		add_hex(ii++);
		add_int(ii++);

		col_string %= lexeme[+(char_ - lit(',') )];

		col_int %= lexeme[int_ | (lit("0x") >> hex)];
	};


	void add_string(JCSIZE col)
	{
		//rule_line = ((col_string [boost::bind(&csv_line_gm::setcol_str, this, col, _1)]) >> ",");
	}
	void add_int(JCSIZE col)
	{
		rule_line = (rule_line.copy() >> (col_int [boost::bind(&csv_line_gm::setcol_int, this, col, _1)]) >> ",");
	}

	void add_float(JCSIZE col)
	{
		using qi::float_;
		rule_line =  rule_line.copy() >> ((float_ [boost::bind(&csv_line_gm::setcol_float, this, col, _1)]) >> ",");
	}

	void add_hex(JCSIZE col)
	{
		using qi::hex;
		rule_line =  rule_line.copy() >> ((hex [boost::bind(&csv_line_gm::setcol_int, this, col, _1)]) >> ",");
	}


	qi::rule<ITERATOR, unsigned(), ascii::space_type> rule_line;
	qi::rule<ITERATOR, CJCStringT()> col_string;
	qi::rule<ITERATOR, INT64(), ascii::space_type> col_int;

	int store;
	float time_stamp;
	float busy_time;
	int cmd_code;
	int lba;
	int sec;
};


	
void CCsvReaderOp::Init(void)
{
	// Open file
	_tfopen_s(&m_src_file, m_src_fn.c_str(), _T("r"));
	if (NULL == m_src_file) THROW_ERROR(ERR_PARAMETER, _T("failure on openning file %s"), m_src_fn.c_str() );

	// 解析文件头，创建表格
	// 表头格式:
	//		header -> column_def , *
	//		column_def -> name[:type[:length]]
	//		type -> string | decimal | float | hex

	// 创建表格
	CCsvTable * m_table = NULL;
	CCsvTable::Create(m_table);

	// 读取文件头
#if 1
	CJCStringA str_line;
	char _line_buf[MAX_LINE_BUF];
	memset(_line_buf, 0, MAX_LINE_BUF);
	fgets(_line_buf, MAX_LINE_BUF, m_src_file);

	//char line_buf[MAX_LINE_BUF];
	stdext::UnicodeToUtf8(str_line, (const wchar_t*)(_line_buf) );
	//csv_header_gm<CJCStringA::const_iterator> gm;
#else
	TCHAR line_buf[MAX_LINE_BUF];
	_fgetts(line_buf, MAX_LINE_BUF, m_src_file);
	CJCStringT str_line(line_buf);
	csv_header_gm<CJCStringT::const_iterator> gm;
#endif

	gm_csv.clear();
	gm_csv.table = m_table;
	qi::phrase_parse(str_line.begin(), str_line.end(), gm_csv, ascii::space);

	m_line_gm = new csv_line_gm<CJCStringA::const_iterator>;
	//JCSIZE col_num = m_table->GetColumnSize();
	//for (JCSIZE ii=0; ii < col_num; ii ++)
	//{
	//	const CCsvColInfo * info = dynamic_cast<const CCsvColInfo *>
	//		(m_table->GetColInfo(ii) );
	//	JCASSERT(info);
	//	switch (info->m_type)
	//	{
	//	case VT_STRING:			m_line_gm->add_string(ii); break;
	//	case jcparam::VT_INT:	m_line_gm->add_int(ii); break;
	//	case VT_HEX:			m_line_gm->add_hex(ii); break;
	//	case VT_FLOAT:			m_line_gm->add_float(ii); break;
	//	}
	//}
}


bool CCsvReaderOp::InvokeOnce(void)
{
	CJCStringA str_line;
	char _line_buf[MAX_LINE_BUF];
	memset(_line_buf, 0, MAX_LINE_BUF);
	if ( !fgets(_line_buf, MAX_LINE_BUF, m_src_file) ) return false;
	stdext::UnicodeToUtf8(str_line, (const wchar_t*)(_line_buf +1) );

	qi::phrase_parse(str_line.begin(), str_line.end(), *m_line_gm, ascii::space);

	return true;
}

void CCsvReaderOp::ParseLine(const CJCStringA & line, JCSIZE length)
{
}

#ifdef _DEBUG
void CCsvReaderOp::DebugOutput(LPCTSTR indentation, FILE * outfile)
{
	stdext::jc_fprintf(outfile, indentation);
	stdext::jc_fprintf(outfile, _T("csv_parser, [%08X], <%08X>\n"),
		(UINT)(static_cast<IAtomOperate*>(this)), (UINT)(0) );
}
#endif
