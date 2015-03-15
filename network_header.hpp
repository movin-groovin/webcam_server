
#ifndef HEADERS_FOR_NETWORK
#define HEADERS_FOR_NETWORK


#include <iostream>
#include <string>

#include <boost/smart_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <opencv2/opencv.hpp>



std::string GetData() {
	std::string buf;
	std::getline(std::cin, buf, '\n');
	return buf;
}


bool CheckError (boost::system::error_code & err_code) {
	if (err_code == boost::asio::error::eof) {
		return false;
	} else if (err_code) {
		throw boost::system::system_error(err_code);
	}
	
	return true;
}


size_t completion_condition(const boost::system::error_code &err, size_t num) {
	std::cout << err << " ------ " << num << '\n';
	
	if(num == 4 || err == boost::asio::error::eof)
		return 0;
	return 1;
}



#endif // HEADERS_FOR_NETWORK







