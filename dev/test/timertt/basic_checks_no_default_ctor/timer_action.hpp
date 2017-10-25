#include <string>

class test_action
{
	std::string & m_dest;
	const std::string m_appendix;

public:
	test_action(
		std::string & dest,
		std::string appendix )
		:	m_dest(dest), m_appendix(std::move(appendix))
	{}

	void operator()() const
	{
		m_dest += m_appendix;
	}
};

