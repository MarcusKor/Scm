#include <Polynomial.h>

using namespace VS3CODEFACTORY::CORE;

Polynomial::Polynomial() {}

Polynomial::~Polynomial()
{
    Clear();
}

void Polynomial::AddTerm(const double_t coef, const int32_t exp)
{
    m_vTerms.push_back(new Term_t(coef, exp));
}

void Polynomial::Clear()
{
    for (auto itr = m_vTerms.begin(); itr != m_vTerms.end(); itr++)
        delete* itr;

    m_vTerms.clear();
}

Polynomial& Polynomial::operator+(Polynomial other)
{
    auto a_itr = m_vTerms.begin();
    auto b_itr = other.m_vTerms.begin();
    std::vector<Term_t*> result;
    std::sort(m_vTerms.begin(), m_vTerms.end(), CompareByMember<Term, int32_t>(&Term::m_nExponential));
    std::sort(other.m_vTerms.begin(), other.m_vTerms.end(), CompareByMember<Term, int32_t>(&Term::m_nExponential));

    while (a_itr != m_vTerms.end() && b_itr != other.m_vTerms.end())
    {
        if ((*a_itr)->m_nExponential == (*b_itr)->m_nExponential)
        {
            result.push_back(new Term((*a_itr)->m_dCoefficient + (*b_itr)->m_dCoefficient, (*a_itr)->m_nExponential));
            a_itr++;
            b_itr++;
        }
        else if ((*a_itr)->m_nExponential > (*b_itr)->m_nExponential)
        {
            result.push_back(new Term(*a_itr));
            ++a_itr;
        }
        else
        {
            result.push_back(new Term(*b_itr));
            ++b_itr;
        }
    }

    for (; a_itr != m_vTerms.end(); a_itr++) result.push_back(*a_itr);
    for (; b_itr != other.m_vTerms.end(); b_itr++) result.push_back(*b_itr);
    Clear();
    m_vTerms.assign(result.begin(), result.end());
    return *this;
}

Polynomial& Polynomial::operator-(Polynomial other)
{
    auto a_itr = m_vTerms.begin();
    auto b_itr = other.m_vTerms.begin();
    std::vector<Term_t*> result;
    std::sort(m_vTerms.begin(), m_vTerms.end(), CompareByMember<Term, int32_t>(&Term::m_nExponential));
    std::sort(other.m_vTerms.begin(), other.m_vTerms.end(), CompareByMember<Term, int32_t>(&Term::m_nExponential));

    while (a_itr != m_vTerms.end() && b_itr != other.m_vTerms.end())
    {
        if ((*a_itr)->m_nExponential == (*b_itr)->m_nExponential)
        {
            result.push_back(new Term((*a_itr)->m_dCoefficient - (*b_itr)->m_dCoefficient, (*a_itr)->m_nExponential));
            a_itr++;
            b_itr++;
        }
        else if ((*a_itr)->m_nExponential > (*b_itr)->m_nExponential)
        {
            result.push_back(new Term(*a_itr));
            ++a_itr;
        }        
        else
        {
            result.push_back(new Term(*b_itr));
            ++b_itr;
        }
    }

    for (; a_itr != m_vTerms.end(); a_itr++) result.push_back(*a_itr);
    for (; b_itr != other.m_vTerms.end(); b_itr++) result.push_back(*b_itr);
    Clear();
    m_vTerms.assign(result.begin(), result.end());
    return *this;
}

Polynomial& Polynomial::operator*(Polynomial other)
{
    Polynomial result;
    std::sort(m_vTerms.begin(), m_vTerms.end(), CompareByMember<Term, int32_t>(&Term::m_nExponential));
    std::sort(other.m_vTerms.begin(), other.m_vTerms.end(), CompareByMember<Term, int32_t>(&Term::m_nExponential));

    for (auto a_itr = m_vTerms.begin(); a_itr != m_vTerms.end(); a_itr++)
    {
        Polynomial temp;

        for (auto b_itr = other.m_vTerms.begin(); b_itr < other.m_vTerms.end(); b_itr++)
            temp.m_vTerms.push_back(new Term((*a_itr)->m_dCoefficient * (*b_itr)->m_dCoefficient, (*a_itr)->m_nExponential + (*b_itr)->m_nExponential));

        result += temp;
    }

    Clear();
    m_vTerms.assign(result.m_vTerms.begin(), result.m_vTerms.end());
    return *this;
}

Polynomial& Polynomial::operator+=(Polynomial other)
{
    return  (*this) + other;
}

Polynomial& Polynomial::operator-=(Polynomial other)
{
    return  (*this) - other;
}

Polynomial& Polynomial::operator*=(Polynomial other)
{
    return  (*this) * other;
}