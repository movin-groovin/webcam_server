
#include "net.hpp"
#include "cam.hpp"

#include <boost/lexical_cast.hpp>
//#include <boost/regex.hpp>



int ReadData(tcp::socket &sock, std::vector<unsigned char> &data, int &rows, int &cols) {
	boost::system::error_code error;
	std::vector<char> hdr_buf(sizeof (NetThings::REQUEST_HEADER));
	NetThings::REQUEST_HEADER msg_hdr;
	
	
	boost::asio::read(sock, boost::asio::buffer(hdr_buf), error);
	if (error) {
		std::cout << "Have caught an error at header reading: " << error.message() << std::endl;
		throw boost::system::system_error(error);
	}
	std::copy(hdr_buf.begin(), hdr_buf.end(), reinterpret_cast<char*> (&msg_hdr));
	
	if (!NetThings::CheckInvariantHeader(msg_hdr)) {
		std::string msg = "Msg header doesn't comply with invariant";
		std::cout << msg + "\n";
		//throw std::runtime_error(msg);
		return 1;
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
	
	
	return 0;
}


template <typename T>
boost::system::error_code SendData(
	tcp::socket &sock,
	const NetThings::REQUEST_HEADER & msg_hdr,
	const T & user_data
) {
	boost::system::error_code error;
	std::vector<unsigned char> data(sizeof msg_hdr + user_data.size());
	
	std::copy(
		reinterpret_cast <const char*> (&msg_hdr),
		reinterpret_cast <const char*> (&msg_hdr) + sizeof msg_hdr,
		data.begin()
	);
	std::copy(user_data.begin(), user_data.end(), data.begin() + sizeof msg_hdr);
	
	boost::asio::write(sock, boost::asio::buffer(data), error);
	
	return error;
}


bool SendAuthData(boost::asio::ip::tcp::socket &socket, const std::string &auth_data) {
	NetThings::REQUEST_HEADER msg_hdr;
	boost::system::error_code error;
	
	NetThings::FillHeader(msg_hdr, auth_data.size(), 0, 0);
	msg_hdr.u.s.command = NetThings::AuthData;
	
	if ((error = SendData(socket, msg_hdr, auth_data))) {
		std::cout << error.message() << "\n";
		return false;
	}
	
	return true;
}


bool RecvAuthData (boost::asio::ip::tcp::socket &sock, std::vector<char> extra_dat) {
	boost::system::error_code error;
	std::vector<char> buf(sizeof (NetThings::REQUEST_HEADER));
	NetThings::REQUEST_HEADER msg_hdr;
	
	boost::asio::read(sock, boost::asio::buffer(buf), error);
	if (error) {
		std::cout << "Have caught an error at header reading: " << error.message() << std::endl;
		throw boost::system::system_error(error);
	}
	std::copy(buf.begin(), buf.end(), reinterpret_cast<char*> (&msg_hdr));
	
	if (!NetThings::CheckInvariantHeader(msg_hdr)) {
		std::string msg = "Msg header doesn't comply with invariant";
		std::cout << msg + "\n";
		throw std::runtime_error(msg);
	}
	
	if (msg_hdr.u.s.status == NetThings::AuthSuccess) {
		if (msg_hdr.u.s.size > 0)
		{
			buf.resize(msg_hdr.u.s.size);
			extra_dat.resize(msg_hdr.u.s.size);
			boost::asio::read(sock, boost::asio::buffer(buf), error);
			if (error) {
				std::cout << "Have caught an error at header reading: " << error.message() << std::endl;
				throw boost::system::system_error(error);
			}
			
			std::copy(buf.begin(), buf.end(), extra_dat.begin());
		}
		
		return true;
	}
	
	return false;
}


int main(int argc, char* argv[]) {
	try {
		if (argc < 4) {
			std::cerr << "Usage: client <host> <port> <username:password>\n" << std::endl;
			return 1001;
		}
		
		io_service io_service;
		tcp::resolver resolver(io_service);
		tcp::socket socket(io_service);
		socket.connect(tcp::endpoint (address::from_string(argv[1]), boost::lexical_cast<int>(argv[2])));
		cv::namedWindow("edges", 1);
		
		if (!SendAuthData(socket, argv[3])) {
			std::cout << "Not passed authentication 1\n";
			return 1002;
		} else {
			std::vector<char> data;
			if (!RecvAuthData(socket, data)) {
				std::cout << "Not passed authentication 2\n";
				return 1003;
			}
		}
		
		std::vector<unsigned char> data;
		int rows, cols;
		for (;;) {
			if (1 == ReadData(socket, data, rows, cols)) {
				continue;
			}
			cv::Mat img = Frames::RestoreFromBuff (&data[0], rows, cols);
			
			try {
				cv::imshow("edges", img);
			} catch (...) {
				continue;
			}
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
