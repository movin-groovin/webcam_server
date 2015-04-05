
#include "net.hpp"
#include "cam.hpp"



void ReadData(tcp::socket &sock, std::vector<unsigned char> &data, int &rows, int &cols) {
	boost::system::error_code error;
	std::vector<char> hdr_buf(sizeof (NetThings::REQUEST_HEADER));
	NetThings::REQUEST_HEADER msg_hdr;
	
	
	boost::asio::read(sock, boost::asio::buffer(hdr_buf), error);
	if (error) {
		std::cout << "Have caught an error at header reading: " << error.message() << std::endl;
		throw boost::system::system_error(error);
	}
	std::copy(hdr_buf.begin(), hdr_buf.end(), reinterpret_cast<char*> (&msg_hdr));
	
	if (!Commands::CheckInvariantHeader(msg_hdr)) {
		std::string msg = "Msg header doesn't comply with invariant";
		std::cout << msg + "\n";
		throw std::runtime_error(msg);
	}
	
	if (msg_hdr.u.s.size > data.size()) {
		data.resize(msg_hdr.u.s.size);
	}
	boost::asio::read(sock, boost::asio::buffer(data), error);
	if (error) {
		std::cout << "Have caught an error at data reading: " << error.message() << std::endl;
		throw boost::system::system_error(error);
	}
	
	rows = msg_hdr.u.s.height;
	cols = msg_hdr.u.s.width;
	
	
	return;
}


int main(int argc, char* argv[]) {
	try {
		if (argc != 2) {
			std::cerr << "Usage: client <host>" << std::endl;
			return 1001;
		}
		
		io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::socket socket(io_service);
		socket.connect(tcp::endpoint (address::from_string("127.0.0.1"), 8899));
		cv::namedWindow("edges", 1);
		
		std::vector<unsigned char> data;
		int rows, cols;
		for (;;) {
			ReadData(socket, data, rows, cols);
			cv::Mat img = Frames::RestoreFromBuff (&data[0], rows, cols);
			
			cv::imshow("edges", img);
			if ('q' == cv::waitKey(1)) {
				std::cout << "Bye bye\n";
				break;
			}
		}
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}
