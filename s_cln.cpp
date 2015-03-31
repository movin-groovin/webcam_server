
#include "async.hpp"



int main(int argc, char* argv[]) {
	try {
		if (argc != 2) {
			std::cerr << "Usage: client <host>" << std::endl;
			return 1;
		}
		
		io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::socket socket(io_service);
		socket.connect(tcp::endpoint (address::from_string("127.0.0.1"), 8899));
		
		for (;;) {
			boost::array<char, 128> buf;
			boost::system::error_code error;
			size_t len = socket.read_some(boost::asio::buffer(buf), error);
			
			if (error == boost::asio::error::eof)
				break; // Connection closed cleanly by peer.
			else if (error)
				throw boost::system::system_error(error); // Some other error.
			
			std::cout.write(buf.data(), len);
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}
