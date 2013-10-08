//#include "StdAfx.h"
#include "../include/local_header.h"

#include "../include/jcoption.h"

COptionSet::COptionSet(void)
{
}


COptionSet::COptionSet(const CJCStringT & name, COptionBase * parent)
    : m_name(name)
    , m_parent(parent)
{
}

COptionSet::~COptionSet(void)
{
    OptionMap::iterator it = m_sub_options.begin();
    OptionMap::iterator end_it = m_sub_options.end();
    for (; it != end_it; it++)
    {
        it->second->Delete();
    }
}

void COptionSet::Delete(void)
{
    delete this;
}

COptionBase * COptionSet::QuerySubItemByName(const CJCStringT & name)
{
    COptionBase * option_item = NULL;
    CJCStringT parent_name;
    CJCStringT::size_type pos = name.find(NAME_SEPERATOR);
    parent_name = name.substr(0, pos);

    OptionMap::iterator it = m_sub_options.find(parent_name);
    if (it != m_sub_options.end() ) option_item = it->second;

    if ( option_item )
    {
        // option item is found
        if (pos != CJCStringT::npos)
        {
            CJCStringT local_name = name.substr(pos+1);
            // the option_item is not the target, query the sub item
            COptionSet * sub_set = dynamic_cast<COptionSet*>(option_item);
            if (!sub_set) option_item = NULL;
            else option_item = sub_set->QuerySubItemByName(local_name);
        }
    }
    return option_item;
}

bool COptionSet::AddSubItem(COptionBase * option)
{
    LPCTSTR option_name = option->GetOptionName();
    std::pair<OptionMap::iterator, bool> rc;
    rc = m_sub_options.insert( OptionPare(option_name, option) );
    return rc.second;
}

void COptionSet::EnumSubOptions(IOptionEnumObserver * observer, bool recursive)
{
    JCASSERT(observer);
    // enum this
    //observer->HandleEnumOption( static_cast<COptionBase*>(this) );
    // enum sub options
    OptionMap::iterator it = m_sub_options.begin();
    OptionMap::iterator end_it = m_sub_options.end();
    if (recursive)
    {
        for (; it != end_it; it++)
        {
            COptionBase * sub_option = it->second;
            observer->HandleEnumOption( sub_option);
            sub_option->EnumSubOptions(observer, recursive);
        }
    }
    else
    {
        for (; it != end_it; it++)
        {
            COptionBase * sub_option = it->second;
            observer->HandleEnumOption(sub_option);
        }
    }
}
//
//void COptionSet::UpdateData(void)
//{
//    IOptionPage * page = GetPageGui();
//    if (!page) return;
//    
//}

void COptionSet::SetItemValue(IParameter * param)
{
    COptionBase * option = QuerySubItemByName(param->GetParamName());
    COptionItemBase * option_item = dynamic_cast<COptionItemBase*>(option);
    if (option_item) 
    {
        CJCStringT str;
        param->GetValueText( str );
        option_item->SetValue( str.c_str() );
    }

}
