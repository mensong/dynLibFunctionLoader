#pragma once

template <	typename Return,
			typename ...Args>
class Function
{
	typedef Return(*type)(Args...);

public:
	explicit Function(type funcPtr):
		m_funcPtr( funcPtr )
	{

	}

	explicit operator bool() const
	{
		return m_funcPtr != nullptr;
	}

	Return operator()(Args... args) const
	{
		return m_funcPtr( args... );
	}
private:
	const type m_funcPtr;
};
