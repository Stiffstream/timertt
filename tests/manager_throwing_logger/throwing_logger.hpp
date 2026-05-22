struct throwing_logger
{
	void operator()( const std::string & what )
	{
		throw std::runtime_error( "an exception logging message: " + what );
	}
};

