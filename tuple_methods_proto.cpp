//Purpose:
//  Demonstrate composition of methods
//  (using CRTP: http://www.boost.org/libs/iterator/doc/index.html#cop95)
//  as well as data to create a tuple with methods, 
//  i.e. a "full fledged class" as mentioned
//  in the following post to boost devel list:
/*
Author: Weapon Liu
Date: 2006-12-14 06:36 -600
To: boost
Subject: [boost] If boost::fusion is the hammer, then what's the nail?

http://archives.free.net.ph/message/20061214.123637.986df9c1.en.html
 */
 
/*
 * $RCSfile: tuple_methods_proto.cpp,v $
 * $Revision: 1.19 $
 * $Date: 2006/12/16 16:29:17 $
 */
  enum
field_names
{ f_0
, f_1
};
 
  template
  < field_names FN
  >
  struct
field_types
{
};

#include <iostream>
#include <iomanip>
  template
  < field_names FN
  >
  std::ostream&
operator<<
  ( std::ostream& sout
  , field_types<FN>const &field
  )
{
    sout<<"field_types<"<<FN<<">\n";
    return sout;
}            
 
  enum
method_names
{ m_0
, m_1
, m_2
};

  template
  < method_names MN
  >
  struct
wrap_method_name
//Allow use as arg to placeholder expresson
//in order to satisfy item 2 in "Expression Requirements"
//of http://www.boost.org/libs/mpl/doc/refmanual/placeholder-expression.html
{};  

  template
  < class HeadMethodName
  , class TailMethods
  , class FFC
  >
  struct
method_types
;

#define METHOD_TYPES_META
//The macro is only used to demonstrate that the
//  method_types<,,>::type
//nested typedef is required to avoid a compile-time error like:
/*
tuple_methods_proto.cpp:254: error: 'struct ffc' has no member named 'm0'
 */
//This is despite the "Expression requirements" on
//  http://www.boost.org/libs/mpl/doc/refmanual/placeholder-expression.html
//which make no mention of the need for such a nested typedef.

  template
  < class TailMethods
  , class FFC
  >
  struct
method_types
  < wrap_method_name<m_0>
  , TailMethods
  , FFC
  >
: TailMethods //in this case, there are no methods, only data fields.
{
  #ifdef METHOD_TYPES_META
        typedef
      method_types
    type
    ;
  #endif
      void
    m0(void)const
    {
        std::cout<<"fields={\n";
        method_types const&me=*this;
        field_types<f_0>const& f0=me;
        std::cout<<f0;
        field_types<f_1>const& f1=me;
        std::cout<<f1;
        std::cout<<"}=fields\n";
    }
};

  template
  < class TailMethods
  , class FFC
  >
  struct
method_types
  < wrap_method_name<m_1>
  , TailMethods
  , FFC
  >
: TailMethods
{
  #ifdef METHOD_TYPES_META
        typedef
      method_types
    type
    ;
  #endif
      void
    m1(unsigned n)const
    {
        std::cout<<std::setw(2*n)<<""<<"{m1:n="<<n<<"\n";
        this->m0();
        if(n>0)
        {
            static_cast<FFC const*>(this)->m2(n-1)
            //FFC(actually the struct ffc below) is derived from 
            //an inherit_linearly<,,> which folded
            //m2 into the inheritance heirarchy after m1
            //Consequently, there's no TailMethods::m2.  
            //Hence, the above static_cast is needed.
            ;
        }
        std::cout<<std::setw(2*n)<<""<<"}m1\n";
    }
};

  template
  < class TailMethods
  , class FFC
  >
  struct
method_types
  < wrap_method_name<m_2>
  , TailMethods
  , FFC
  >
: TailMethods
{
  #ifdef METHOD_TYPES_META
        typedef
      method_types
    type
    ;
  #endif
      void
    m2(unsigned n)const
    {
        std::cout<<std::setw(2*n)<<""<<"{m2:n="<<n<<"\n";
        this->m0();
        if(n>0)
        {
            this->m1(n-1);
        }
        std::cout<<std::setw(2*n)<<""<<"}m2\n";
    }
};

#include <boost/mpl/vector.hpp>

    typedef
  boost::mpl::vector
  < field_types<f_0>
  , field_types<f_1>
  >
fields_seq
;
      
#include <boost/mpl/inherit.hpp>
#include <boost/mpl/inherit_linearly.hpp>
#include <boost/mpl/arg.hpp>

  template
  < class FieldsSeq
  >
  struct
ffc_fields
: boost::mpl::inherit_linearly
  < FieldsSeq
  , boost::mpl::inherit
    < boost::mpl::arg<2>
    , boost::mpl::arg<1>
    >
  >::type
{
};  

    typedef
  boost::mpl::vector
  < wrap_method_name<m_0>
  , wrap_method_name<m_1>
  , wrap_method_name<m_2>
  >
methods_seq
;
  struct
ffc
: boost::mpl::inherit_linearly
  < methods_seq
  , method_types
    < boost::mpl::arg<2>
    , boost::mpl::arg<1>
    , ffc 
      //this ffc, and the static_cast in:
      //  method_types< wrap_method_name<m_1>,...>::m1
      //qualifies this as an example of CRTP.
    >
  , ffc_fields
    < fields_seq
    >
  >::type
{
};

  int
main(void)
{
    typedef ffc_fields<fields_seq> ffc_fields_empty_base;
  #if 1
    {//demonstrate that nested method_types<,,>::type
     //is required for a placeholder method_types<,,> to
     //work.
     //To show the error, #undef METHODS_TYPES_META.
        typedef boost::mpl::arg<1> _1;
        typedef boost::mpl::arg<2> _2;
        typedef method_types<_2,_1,ffc_fields_empty_base> mt_phe
        //should be, according to:
        //  http://www.boost.org/libs/mpl/doc/refmanual/placeholder-expression.html 
        //a valid placeholder expression.
        ;
        typedef boost::mpl::apply2<mt_phe,ffc_fields_empty_base,wrap_method_name<m_0> >::type
        applied_phe;
        applied_phe a_m0;
        a_m0.m0();//however, this produces compile-time error when #undef METHOD_TYPES_META:
        /*
tuple_methods_proto.cpp:261: error: 'struct main()::applied_phe' has no member named 'm0'
         */
    }
  #endif
  #ifdef METHOD_TYPES_META
    {//check  ffc_fields:
        ffc_fields_empty_base a_fields;
        field_types<f_0>const& a_f0=a_fields;
        std::cout<<a_f0;
    }
    {//check method_types:
        method_types<wrap_method_name<m_0>,ffc_fields_empty_base,ffc_fields_empty_base> a_m0;
        a_m0.m0();
    }
    {//check ffc mutual recursion method calls:
        ffc a_ffc;
        a_ffc.m0();
        a_ffc.m1(3);
        a_ffc.m2(3);
    }
  #endif
    return 0;
}      
