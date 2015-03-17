
// http://habrahabr.ru/post/192284/
// http://www.bogotobogo.com/cplusplus/Boost/boost_AsynchIO_asio_tcpip_socket_server_client_timer_bind_handler_multithreading_synchronizing_network_D.php



#include "network_header.hpp"


using boost::asio::ip::tcp;



cv::VideoCapture AdjustCamera () {
    cv::VideoCapture cap(0); // open the default camera
    if(!cap.isOpened()) {  // check if we succeeded
        throw std::runtime_error ("Cah't connect to first cmaera device");
    }
    cap.set (CV_CAP_PROP_FRAME_WIDTH, 480); // 480 640
    cap.set (CV_CAP_PROP_FRAME_HEIGHT, 320); // 320 480
    
    return cap;
}


cv::Mat GetData(cv::VideoCapture &cap) {
	cv::Mat frame;
	
	cap >> frame; // get a new frame from camera
	
	return frame;
}


int main (int argc, char **argv) {
	std::string port = "54321";
	
	
	if (argc < 2) {
		//std::cout << "Enter port number\n";
		//return 1001;
	} else {
		port = argv[1];
	}
	
	try {
		boost::asio::io_service io_srv;
		
		tcp::acceptor acceptor (io_srv, tcp::endpoint (tcp::v4(), boost::lexical_cast<int>(port)));
		cv::VideoCapture cap = AdjustCamera ();
		
		for (;;) {
			tcp::socket sock(io_srv);
			boost::array <char, 1024> buf;
			boost::system::error_code err_code;
			
			acceptor.accept(sock);
			//sock.non_blocking(false); 
			
			while (true) {
				cv::Mat frame = GetData(cap);
				cv::Mat new_frame = MakeLinear(frame);
				size_t len = WriteDataPart(sock, new_frame.data, GetMatSize(&new_frame), frame.rows, frame.cols);
				
				if (len != GetMatSize(&frame)) {
					std::cout << "Serv out\n";
					break;
				}
				
				cv::waitKey(25);
			}
			//
		}
	}
	catch (std::exception & exc) {
		std::cout << exc.what() << std::endl;
	}
	
	
	return 0;
}
