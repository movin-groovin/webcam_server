
#ifndef HEADERS_FOR_NETWORK
#define HEADERS_FOR_NETWORK


#include <iostream>
#include <string>
#include <algorithm>

#include <boost/smart_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/array.hpp>
#include <boost/asio.hpp>

#include <opencv2/opencv.hpp>



struct REQUEST_HEADER {
	union {
		struct {
			size_t size;
		};
		size_t padd[8];
	};
} __attribute__((packed)); // just for example


void FillHeader(REQUEST_HEADER &hdr, unsigned size) {
	std::fill_n(
		reinterpret_cast<size_t*>(&hdr),
		sizeof(REQUEST_HEADER) / sizeof(size_t),
		0
	);
	hdr.size = size;
	
	return;
}


bool CheckError (boost::system::error_code & err_code) {
	if (err_code) {
		return false;
	}
	
	return true;
}


template <typename SockType>
boost::asio::mutable_buffer ReadDataPart (SockType &sock) {
	size_t num;
	REQUEST_HEADER hdr;
	boost::system::error_code err_code;
	
	FillHeader(hdr, 0);
	boost::asio::read(sock, boost::asio::buffer(&hdr, sizeof (hdr)), err_code);
	if (!CheckError(err_code)) {
		return boost::asio::mutable_buffer();
	}
	
	boost::asio::mutable_buffers_1 data_buf =
			boost::asio::buffer(new unsigned char [hdr.size + sizeof (size_t)], hdr.size);
	boost::asio::read(sock, data_buf, err_code);
	if (!CheckError(err_code)) {
		delete [] boost::asio::buffer_cast<char*>(data_buf);
		return boost::asio::mutable_buffer();
	}
	
	return data_buf;
}


template <typename SockType>
size_t WriteDataPart (SockType &sock, const void *data, size_t n) {
	size_t num;
	REQUEST_HEADER hdr;
	boost::system::error_code err_code;
	
	FillHeader(hdr, n);
	boost::asio::write(sock, boost::asio::buffer(&hdr, sizeof (hdr)), err_code);
	if (!CheckError(err_code)) {
		return 0;
	}
	
	num = boost::asio::write(sock, boost::asio::buffer (data, n), err_code);
	if (!CheckError(err_code)) {
		return num;
	}
	
	return num;
}


std::string GetData() {
	std::string buf;
	std::getline(std::cin, buf, '\n');
	return buf;
}


size_t completion_condition(const boost::system::error_code &err, size_t num) {
	std::cout << err << " ------ " << num << '\n';
	
	if(num == 4 || err == boost::asio::error::eof)
		return 0;
	return 1;
}



#endif // HEADERS_FOR_NETWORK







