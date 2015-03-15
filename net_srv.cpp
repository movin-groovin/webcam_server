
// http://habrahabr.ru/post/192284/
// http://www.bogotobogo.com/cplusplus/Boost/boost_AsynchIO_asio_tcpip_socket_server_client_timer_bind_handler_multithreading_synchronizing_network_D.php



#include "network_header.hpp"


using boost::asio::ip::tcp;



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
		for (;;) {
			tcp::socket sock(io_srv);
			boost::array <char, 1024> buf;
			boost::system::error_code err_code;
			
			acceptor.accept(sock);
			//sock.non_blocking(false); 
			
			while (true) {
				std::string data = GetData();
				boost::asio::write (sock, boost::asio::buffer(data), err_code);
				if (!CheckError(err_code)) {
					std::cout << "Serv out 1\n";
					break;
				}
				
				size_t len = boost::asio::read (sock, boost::asio::buffer(buf), &completion_condition, err_code);
				if (!CheckError(err_code)) {
					if (len) {
						std::cout.write(buf.data(), len);
						std::cout << std::endl;
					}
					std::cout << "Serv out 2\n";
					break;
				}
				
				std::cout.write(buf.data(), len);
				std::cout << std::endl;
			}
			//
		}
	}
	catch (std::exception & exc) {
		std::cout << exc.what() << std::endl;
	}
	
	
	return 0;
}
