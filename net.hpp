
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


enum Commands {
	LowCommand = 0
	Frame,
	Service,
	NewCommand,
	HighCommand
};

inline bool CheckCommand(unsigned cmd) {
	return cmd >= LowCommand && cmd <= HighCommand;
}


enum Statuses {
	LowStatus = 0,
	Success,
	Error,
	NewStatus,
	HighStatus
};

inline bool CheckStatus(unsigned cmd) {
	return cmd >= LowStatus && cmd <= HighStatus;
}


enum ExtraStatus {
	LowExtraStatus = 0,
	NewExtraStatus,
	HighExtraStatus
}

inline bool CheckExtraStatus(unsigned cmd) {
	return cmd >= LowExtraStatus && cmd <= HighExtraStatus;
}


const unsigned MinDataSize = 0;
const unsigned MaxDataSize = 1024*1024;
const unsigned MinFrameHeight = 0;
const unsigned MaxFrameHeight = 2000;
const unsigned MinFrameWidth = 0;
const unsigned MaxFrameWidth = 2000;



struct REQUEST_HEADER {
	union {
		struct {
			size_t size;
			unsigned height; // rows
			unsigned width;  // cols
			unsigned command;
			unsigned status;
			unsigned extra_status;
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
