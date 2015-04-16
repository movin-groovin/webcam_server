
#include "net.hpp"



namespace NetThings {


bool CheckInvariantHeader(const REQUEST_HEADER &hdr) {
	return CheckCommand(hdr.u.s.command) &&
		   CheckStatus(hdr.u.s.status) &&
		   CheckExtraStatus(hdr.u.s.extra_status) &&
		   (hdr.u.s.size >= MinDataSize) &&
		   (hdr.u.s.size <= MaxDataSize) &&
		   (hdr.u.s.height >= MinFrameHeight) &&
		   (hdr.u.s.height <= MaxFrameHeight) &&
		   (hdr.u.s.width >= MinFrameWidth) &&
		   (hdr.u.s.width <= MaxFrameWidth);
}


void FillHeader(REQUEST_HEADER &hdr, unsigned size, unsigned height, unsigned width) {
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


}
