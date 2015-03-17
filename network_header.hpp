
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
			unsigned height; // rows
			unsigned width;  // cols
		};
		size_t padd[8];
	};
} __attribute__((packed)); // just for example


void FillHeader(REQUEST_HEADER &hdr, unsigned size, unsigned height = 0, unsigned width = 0) {
	std::fill_n(
		reinterpret_cast<size_t*>(&hdr),
		sizeof(REQUEST_HEADER) / sizeof(size_t),
		0
	);
	hdr.size = size;
	hdr.height = height;
	hdr.width = width;
	
	return;
}


bool CheckError (boost::system::error_code & err_code) {
	if (err_code) {
		return false;
	}
	
	return true;
}


template <typename SockType>
boost::asio::mutable_buffer ReadDataPart (SockType &sock, unsigned &height, unsigned &width) {
	size_t num;
	REQUEST_HEADER hdr;
	boost::system::error_code err_code;
	
	FillHeader(hdr, 0);
	boost::asio::read(sock, boost::asio::buffer(&hdr, sizeof (hdr)), err_code);
	if (!CheckError(err_code)) {
		return boost::asio::mutable_buffer();
	}
	height = hdr.height;
	width = hdr.width;
	
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
size_t WriteDataPart (SockType &sock, const void *data, size_t n, unsigned height, unsigned width) {
	size_t num;
	REQUEST_HEADER hdr;
	boost::system::error_code err_code;
	
	FillHeader(hdr, n, height, width);
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


size_t completion_condition(const boost::system::error_code &err, size_t num) {
	std::cout << err << " ------ " << num << '\n';
	
	if(num == 4 || err == boost::asio::error::eof)
		return 0;
	return 1;
}

// ===============================================================
// ======================== OPENCV FUNCS =========================
// ===============================================================
cv::Mat MakeLinear (const cv::Mat & mat) {
	return mat.reshape (0, 1);
}


size_t GetMatSize(cv::Mat *frame) {
	return frame->rows * frame->cols * frame->channels();
}


#endif // HEADERS_FOR_NETWORK







