
#ifndef __ASYNC_HPP
#define __ASYNC_HPP



#include <ctime>
#include <cassert>

#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <fstream>
#include <unordered_map>

#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>



using namespace boost::asio::ip;
using namespace boost::asio;



class CTcpServer;


namespace NetThings {


struct REQUEST_HEADER {
	union {
		struct {
			size_t size;
			unsigned height; // rows
			unsigned width;  // cols
		} s;
		size_t padd[8];
	} u;
} __attribute__((packed)); // just for example


void FillHeader(REQUEST_HEADER &hdr, unsigned size, unsigned height = 0, unsigned width = 0) {
	std::fill_n(
		reinterpret_cast<size_t*>(&hdr),
		sizeof(REQUEST_HEADER) / sizeof(size_t),
		0
	);
	hdr.u.s.size = size;
	hdr.u.s.height = height;
	hdr.u.s.width = width;
	
	return;
}


} // NetThings


#endif // __ASYNC_HPP
