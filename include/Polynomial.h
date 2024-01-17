#pragma once

#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include <cppstd.h>

namespace VS3CODEFACTORY::CORE
{
	typedef struct Term
	{
		int32_t m_nExponential;
		double_t m_dCoefficient;
		Term(double_t coef, int32_t exp)
		{
			m_nExponential = exp;
			m_dCoefficient = coef;
		}
		Term(Term* item)
		{
			m_nExponential = item->m_nExponential;
			m_dCoefficient = item->m_dCoefficient;
		}
	} Term_t;

	template<typename T, typename U>
	struct CompareByMember
	{
		U T::* field;
		CompareByMember(U T::* f)
			: field(f) {}
		bool operator()(const T& lhs, const T& rhs)
		{
			return lhs.*field < rhs.*field;
		}
	};

	class Polynomial
	{
	public:
		std::vector<Term_t*> m_vTerms;

		Polynomial();
		virtual ~Polynomial();

		void AddTerm(const double_t coef, const int32_t exp);
		void Clear();


		Polynomial& operator+(Polynomial other);
		Polynomial& operator-(Polynomial other);
		Polynomial& operator*(Polynomial other);
		Polynomial& operator+=(Polynomial other);
		Polynomial& operator-=(Polynomial other);
		Polynomial& operator*=(Polynomial other);
	};
}

#endif // POLYNOMIAL_H