#pragma once

namespace stdext
{
#if 0
    template<class _Elem>
    class jcstring : public ATL::CStringT<_Elem, StrTraitMFC_DLL< char > >
    {
        typedef ATL::CStringT<_Elem, StrTraitMFC_DLL< char > >  base_class;
    public:
        typedef int size_type;

        jcstring(void) : base_class() {};
        jcstring(const _Elem * str) : base_class(str) {};
        const _Elem * c_str() const {
            return (const _Elem*)(*this);
        };

        size_type find(const _Elem* str) {
            
        }

        //size_type find_first_not_of(


    };
#else
    template<class _Elem>
    class jcstring
        : public std::basic_string<_Elem, std::char_traits<_Elem>, std::allocator<_Elem> >
    {
        typedef std::basic_string<_Elem, std::char_traits<_Elem>, std::allocator<_Elem> > base_class;
        //typedef base_class::const_pointer lpcstr;
        //typedef base_class::pointer lpstr;
        typedef const _Elem * lpcstr;
        typedef _Elem * lpstr;

    public:
        jcstring(void) : base_class() {};
        jcstring(const _Elem * str) : base_class(str) {};
        jcstring(const base_class & str) : base_class(str) {};
        ~jcstring(void) {};

    public:
        void formatv(lpcstr str_format, va_list args);
        void Format(lpcstr str_format, ...);

    };
#endif
};
