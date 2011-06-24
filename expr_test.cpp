/****************************************************************************************************************
*	
*	Author:			Korcan Hussein
*	User name:		snk_kid
*	Date:			23/10/05
*
*	Description:	This example demenstrates a possible, alternative solution to the problem of
*					intermediate results of expression templates, to be more clear of the problem
*					here is a quote taken from:
*
*						C++ Template Metaprogramming : Concepts, Tools, and Techniques from Boost and Beyond
*							by David Abrahams, Aleksey Gurtovoy
*
*					[quote]
*						... One drawback of expression templates is that they tend to encourage writing 
*						large, complicated expressions, because evaluation is only delayed until the 
*						assignment operator is invoked. If a programmer wants to reuse some 
*						intermediate result without evaluating it early, she may be forced to 
*						declare a complicated type like:
*
*						Expression<
*							Expression<Array,plus,Array>,
*							plus,
*							Expression<Array,minus,Array>
*                        > intermediate = a + b + (c - d);
*
*                       (or worse). Notice how this type not only exactly and redundantly reflects 
*						the structure of the computationand so would need to be maintained as the 
*						formula changes but also overwhelms it? This is a long-standing problem for 
*						C++ DSELs. The usual workaround is to capture the expression using type 
*						erasure, but in that case one pays for dynamic dispatching.
*
*						There has been much discussion recently, spearheaded by Bjarne Stroustrup 
*						himself, about reusing the vestigial auto keyword to get type deduction in 
*						variable declarations, so that the above could be rewritten as:
*
*						auto intermediate = a + b + (c - d);
*
*						This feature would be a huge advantage to C++ DSEL authors and users alike...
*					[/qoute]
*
*					Type erasure refers to a technique which takes advantage of type deducation of template
*					function arguments. Basically you keep richful type information until the very last moment
*					and then "erase" the static type into some polymorhpic type, Boost.Any is a typical example
*					of this.
*
*					My solution takes alook at the problem from a different angle, instead of "erasing type",
*					take the intermediate results and "convert" (not literally) the compile-time parse tree into
*					a *semi*-runtime equivalent using recursive variant types and not the traditional mechanisms of
*					OO abuse using the composite pattern and sub-type polymorhism.
*
*					The advantage of this is it gets us close to type deduction of variable declarations
*					(until of course changes made to auto), while not paying for dynamic (runtime) dispatch and
*					casting syndrome. Most of which can be made automated, there is also the possbilty of mixing compile-time & semi-runtime parse trees,
*					adding new nodes etc.
*
*					As of current this is not a library, this is simply an example of the technique, this needs further
*					generalization, testing, discussion etc.
*					
*					Hence i'm placing this in the valut for dicussion on boost.devel mailing list.
*					further details visit: http://lists.boost.org/Archives/boost/2005/10/95793.php
*
*/
#include <boost/type_traits/integral_constant.hpp>
#include <boost/variant.hpp>
#include <boost/call_traits.hpp>
#include <cstddef>

template < typename, typename,	typename, typename ReturnType = float >
class Expression;
struct addf_tag;
struct minusf_tag;

template < typename OperandLhs, typename OperandRhs >
// take note of explicit constant reference here, explained in the definition of Expression
inline Expression<const OperandLhs&, addf_tag, const OperandRhs&>
	operator+(const OperandLhs& lhs, const OperandRhs& rhs) {
        return Expression<const OperandLhs&, addf_tag, const OperandRhs&>(lhs, rhs);
}

template < typename OperandLhs, typename OperandRhs >
// take note of explicit constant reference here, explained in the definition of Expression
inline Expression<const OperandLhs&, minusf_tag, const OperandRhs&>
	operator-(const OperandLhs& lhs, const OperandRhs& rhs) {
        return Expression<const OperandLhs&, minusf_tag, const OperandRhs&>(lhs, rhs);
}

template < typename OperandLhs, typename Operation,	typename OperandRhs, typename ReturnType >
class Expression {

	typedef boost::call_traits<OperandLhs> call_traits1;
	typedef boost::call_traits<OperandRhs> call_traits2;

public:

	typedef OperandLhs		lhs_type;
	typedef OperandRhs		rhs_type;
	typedef ReturnType		result_type;
	typedef Operation		operation_type;
	typedef std::size_t		size_type;

	typedef typename call_traits1::param_type lhs_param_type;
	typedef typename call_traits2::param_type rhs_param_type;

	typedef typename call_traits1::const_reference const_ref_lhs;
	typedef typename call_traits2::const_reference const_ref_rhs;

private:

	lhs_type lhs;	// notice members are not of constant reference type here,
	rhs_type rhs;	// this is due to the fact of reusing Expression
					// in recursive variants (see var) as the variant type itself must be held by value
					// and not constant reference, to avoid redundant copies
					// we get round this by making one of the variant's types a constant reference.
public:

	Expression(lhs_param_type lhs, rhs_param_type rhs): lhs(lhs), rhs(rhs) {}

	const_ref_lhs get_lhs() const { return lhs; }
	const_ref_rhs get_rhs() const { return rhs; }

	result_type operator[](size_type index) const {
		return Operation::apply(lhs[index], rhs[index]);
	}
    
};

struct addf_tag {
	static float apply(float a, float b) {
		return a + b;
	}
};

struct minusf_tag {

	static float apply(float a, float b) {
		return a - b;
	}
};

template < typename Tp >
struct var {

	typedef typename boost::make_recursive_variant<
        const Tp&,
        Expression<boost::recursive_variant_, addf_tag, boost::recursive_variant_>,
		Expression<boost::recursive_variant_, minusf_tag, boost::recursive_variant_>
    >::type expr;

	typedef Expression<expr, addf_tag, expr> add_expr;
	typedef Expression<expr, minusf_tag, expr> minus_expr;

	typedef std::size_t size_type;

private:

	expr root_expr;

	template < typename Tq >
	struct is_intern : boost::false_type {
		typedef is_intern<Tq> type;
	};

	template < typename OperandLhs, typename Operation, typename OperandRhs, typename RET >
	struct is_intern< Expression<OperandLhs, Operation, OperandRhs, RET> > : boost::true_type {
		typedef is_intern< Expression<OperandLhs, Operation, OperandRhs, RET> > type;
	};

	template < typename Exp >
	static expr make_tree_impl(const Exp& e, boost::true_type) {
		return Expression<expr, typename Exp::operation_type, expr>
			(make_tree(e.get_lhs()), make_tree(e.get_rhs()));
	}

	template < typename Exp >
	static inline expr make_tree_impl(const Exp& e, boost::false_type) {
		return e;
	}
	
	template < typename Exp >
	static inline expr make_tree(const Exp& e) {
		return make_tree_impl(e, is_intern<Exp>());
	
	}

	struct basic_eval : boost::static_visitor<float> {
		
		size_type curr_indx;

        basic_eval(size_type i = 0): curr_indx(i) {}

        float operator()(const Tp& val) const {
            return val[curr_indx];
        }

        template < typename Expression >
        float operator()(const Expression& exp) const {
            using boost::apply_visitor;
            typedef typename Expression::operation_type Oper;

            return	Oper::apply(
                apply_visitor(*this, exp.get_lhs()),
                apply_visitor(*this, exp.get_rhs()));
		}
	};

public:

	var(): root_expr() {}

    var(const var<Tp>& v): root_expr(v.root_expr) {}

	template < typename Exp >
	var(const Exp& e): root_expr(make_tree(e)) {}

	void swap(var<Tp>& v) {
		root_expr.swap(v.root_expr);
	}	

	var<Tp>& operator=(const var<Tp>& v) {
		var(v).swap(*this);
		return *this;
	}

	template < typename Exp >
	var<Tp>& operator=(const Exp& e) {
		var(e).swap(*this);
		return *this;
	}

	const expr& root_of_expr() const {
		return root_expr;
	}

	float operator[](size_type index) const {
		return boost::apply_visitor(basic_eval(index), root_of_expr());
	}
};

#include <algorithm>

namespace std {
	template < typename Tp >
	void swap(var<Tp>& lhs, var<Tp>& rhs) {
		lhs.swap(rhs);
	}
};

/////////////////////////////////////////////////// EXAMPLE //////////////////////////////////////////////////////////
#include <vector>

typedef std::vector<float> vecn;

template < typename Exp >
vecn& operator+=(vecn& v, const Exp& e) {		// typical lazy evalution of expression templates

	for(vecn::size_type i = 0, n = v.size(); i < n; ++i)
		v[i] = e[i];

	return v;

}

#include <cmath>
#include <iostream>

int main() {

	typedef var<vecn> var;

	const vecn::size_type N = 10;

	vecn a(N, 0.0f), b(N, 0.0f), c(N, 0.0f), d(N, 0.0f), result(N, 0.0f);

	std::generate(a.begin(), a.end(), std::rand);
	std::generate(b.begin(), b.end(), std::rand);
	std::generate(c.begin(), c.end(), std::rand);
	std::generate(d.begin(), d.end(), std::rand);

	// some complicated expression, force on us when strictly using expression templates
	// result += a + b - c + d + a + b + c - d + c;

	// now we can split it into as many pieces as we like:
	var temp = a + b - c + d + a;
    result += temp  + b + c - d + c; // half compile - half run-time parse tree!?!?

	std::cout << "\n\nresults: ";
	std::copy(result.begin(), result.end(), std::ostream_iterator<float>(std::cout, ", "));
}