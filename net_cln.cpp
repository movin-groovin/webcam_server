
// http://habrahabr.ru/post/192284/
// http://www.bogotobogo.com/cplusplus/Boost/boost_AsynchIO_asio_tcpip_socket_server_client_timer_bind_handler_multithreading_synchronizing_network_D.php



#include "network_header.hpp"


using boost::asio::ip::tcp;



cv::Mat RestoreFromBuff (unsigned char *buf, int orig_rows, int orig_cols) {
	cv::Mat img = cv::Mat::zeros(orig_rows, orig_cols, CV_8UC3);
	size_t offset = 0;
	
	for (int i = 0;  i < img.rows; i++) {
		for (int j = 0; j < img.cols; j++) {
			img.at<cv::Vec3b>(i,j) = cv::Vec3b(
				buf[offset + 0],
				buf[offset + 1],
				buf[offset + 2]
			);
			offset += 3;
		}
	}
	
	return img;
}


int main (int argc, char **argv) {
	std::string host = "127.0.0.1";
	std::string port = "54321";
	
	
	if (argc < 3) {
		//std::cout << "Enter host name and port\n";
		//return 1001;
	} else {
		host = argv[1];
		port = argv[2];
	}
	
	try {
		boost::asio::io_service io_srv;
		tcp::resolver resolver(io_srv);
		tcp::resolver::query query(host, port);
		tcp::resolver::iterator iter(resolver.resolve(query));
		tcp::socket sock(io_srv);
		boost::array <char, 1024> buf;
		boost::system::error_code err_code;
		
		boost::asio::connect(sock, iter);
		cv::namedWindow("edges", 1);
		
		for (;;) {
			unsigned height, width;
			boost::asio::mutable_buffers_1 buf (ReadDataPart(sock, height, width));
			char *mem = boost::asio::buffer_cast<char*> (buf);
			size_t len = boost::asio::buffer_size(buf);	
			
			if (!len) {
				delete [] mem;
				std::cout << "Cln out\n";
				break;
			}
			cv::Mat img = RestoreFromBuff(reinterpret_cast<unsigned char*>(mem), height, width);
			delete [] mem;
			
			cv::imshow("edges", img);
			cv::waitKey(25);
		}
	}
	catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	}
	
	
	return 0;
}
