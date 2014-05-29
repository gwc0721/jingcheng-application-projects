#pragma once

// 通用的String Table的实现。
// String Table主要实用于以String为关键词索引的表格。表格的内容是基于指针的，因此不要求每个项目的内容都一致。
//	支持三种形态的初始化时静态创建。
//	1，固定排序方式：基于折半查找的索引算法。关键词的排序在源代码实现。
//		初始化时必须按照顺序插入，不支持初始化以后的动态插入。由于折半查找算法的时间常系数很小，
//		查找效率很高。同时排序在源代码中完成，初始化时间也很短。
//	2，静态排序方式：基于折半查找的索引算法，源代码中不要求对关键字排序。表格再添加项目是排序或者初始化时排序。
//		优点是折半查找的高速化，并且不要求源代码中关键字排序，使用方便。但是初始化以及动态插入效率很低，
//		基本上不支持动态插入。
//	3，动态排序方式：基于RB树查找的索引算法。有点是源代码不要求排序，并且支持动态插入。但是查找速度相对较慢。
//
//	所有方式都支持表格式的初始化方法。因此必须有Rule作为描述的辅助类。

#include <vector>

namespace jcparam
{
	template <class P_ITEM_TYPE>
	class NO_INDEX
	{
	public:
		NO_INDEX<P_ITEM_TYPE>(void) {};
		void push_back(P_ITEM_TYPE) {};
		void reserve(JCSIZE) {};
		P_ITEM_TYPE & operator [] (JCSIZE) 
		{
			THORW_ERROR(ERR_APP, _T("Do not support sub index.") );
			P_ITEM_TYPE dummy = NULL;
			return dummy;
		}
		static NO_INDEX * Create(void) {return NULL;}
	};

	// BASE_ITEM_TYPE 表格中各项的基本类型。
	// SECOND_INDEX 表格的主要索引是ITEMT_TYPE::name(), SUB_INDEX为第二索引
	//	例如：初始化表格的顺序，id的顺序等。缺省为NO_INDEX，既不提供SUB_INDEX功能


	template <class BASE_ITEM_TYPE, class SECOND_INDEX = jcparam::NO_INDEX<const typename BASE_ITEM_TYPE*> >
	class CStringTable
	{
	public:	
		typedef std::map<CJCStringT, const BASE_ITEM_TYPE*>		ITEM_MAP;
		typedef std::pair<CJCStringT, const BASE_ITEM_TYPE*>		PAIR;
		typedef typename ITEM_MAP::const_iterator			CONST_ITERATOR;

	public:
		class RULE
		{
		public:
			RULE() : m_item_map(NULL), m_sub_index(NULL)
			{
				m_item_map = new ITEM_MAP;
				m_sub_index = new SECOND_INDEX;
			}

			template <class ITEM_CLASS>
			RULE & operator () (const ITEM_CLASS * item)
			{
				JCASSERT(m_item_map);
				JCASSERT(m_sub_index);

				const BASE_ITEM_TYPE * pp = item;
				std::pair<ITEM_MAP::iterator, bool> rs = m_item_map->insert( PAIR(pp->name(), pp) );
				if (!rs.second) THROW_ERROR(ERR_PARAMETER, _T("Item %s redefined "), pp->name() );
				m_sub_index->push_back(pp);
				return * this;
			};

		public:
			ITEM_MAP		* m_item_map;
			SECOND_INDEX	* m_sub_index;
		};

	public:
		CStringTable(const RULE & rules)
			: m_item_map(rules.m_item_map)
			, m_sub_index(rules.m_sub_index)
		{
			//LOG_STACK_TRACE();
			JCASSERT(m_item_map);
			JCASSERT(m_sub_index);
		}

		CStringTable(void)
			: m_item_map(NULL)
			, m_sub_index(NULL)
		{
			//LOG_STACK_TRACE();
			m_item_map = new ITEM_MAP;
			m_sub_index = new SECOND_INDEX;
		}

		~CStringTable()
		{
			JCASSERT(m_item_map);
			JCASSERT(m_sub_index);
			ITEM_MAP::const_iterator it = m_item_map->begin();
			ITEM_MAP::const_iterator endit = m_item_map->end();
			for ( ; it != endit; ++it)	delete (it->second);
			delete m_item_map;
			delete m_sub_index;
		}

		void AddItem(const BASE_ITEM_TYPE * item)
		{
			JCASSERT(m_item_map);
			JCASSERT(m_sub_index);
			std::pair<ITEM_MAP::iterator, bool> rs = m_item_map->insert( PAIR(item->name(), item) );
			if (!rs.second) THROW_ERROR(ERR_PARAMETER, _T("Item %s redefined "), item->name() );
			m_sub_index->push_back(item);
		}

		const JCSIZE GetSize() const
		{
			JCASSERT(m_item_map);
			return (JCSIZE) m_item_map->size();
		}

		const BASE_ITEM_TYPE * GetItem(const CJCStringT & key) const
		{
			JCASSERT(m_item_map);
			ITEM_MAP::const_iterator it = m_item_map->find(key);
			if ( it == m_item_map->end() ) return NULL;
			return it->second;
		};

		// Sub Index
		const BASE_ITEM_TYPE * GetItem(JCSIZE index) const
		{
			JCASSERT(m_sub_index);
			JCASSERT(index < m_sub_index->size());
			return (*m_sub_index)[index];
		}

		void Enumerate()
		{
		};

		CONST_ITERATOR Begin() const 
		{
			JCASSERT(m_item_map);
			return m_item_map->begin(); 
		}
		
		CONST_ITERATOR End() const 
		{
			JCASSERT(m_item_map);
			return m_item_map->end(); 
		}
		

	protected:
		ITEM_MAP	* m_item_map;
		typename SECOND_INDEX	* m_sub_index;
	};
};