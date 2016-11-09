#ifndef ERRSTREAM_H
#define ERRSTREAM_H

#include <string>
#include <sstream>

#define af_err_stream_f(message) \
	(aifil::ErrStream() << "Error in file: " << __FILE__ << ", function: " << __func__ \
	<< ", line: " << __LINE__ << ", with message: " << message)

#define af_err_stream(message) (aifil::ErrStream() << message)


/**
 * Implementation of text stream object for formatting exception messages.
 * \code{.cpp}
 * throw std::runtime_error(ErrStream() << "Invalid value: " << value);
 * \endcode
 */

namespace aifil
{

class ErrStream
{
	public:
		template<class T>
		
		ErrStream& operator<< (const T& _arg)
		{
			stream << _arg;
			return *this;
		}
		
		operator std::string() const
		{
			return stream.str();
		}
	
	private:
		std::stringstream stream;
};

}
#endif //ERRSTREAM_H
