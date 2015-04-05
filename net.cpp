
#include "net.hpp"



namespace Commands {


bool CheckInvariantHeader(const REQUEST_HEADER &hdr) {
	return CheckCommand(hdr.u.s.command) &&
		   CheckStatus(hdr.u.s.status) &&
		   CheckExtraStatus(hdr.u.s.extra_status) &&
		   (hdr.u.s.size >= MinDataSize) &&
		   (hdr.u.s.size <= MaxDataSize) &&
		   (hdr.u.s.height >= MinFrameHeight) &&
		   (hdr.u.s.height <= MaxFrameHeight) &&
		   (hdr.u.s.width >= MinFrameWidth) &&
		   (hdr.u.s.width <= MaxFrameWidth)
}


}
