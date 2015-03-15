
// http://habrahabr.ru/post/192284/
// http://www.bogotobogo.com/cplusplus/Boost/boost_AsynchIO_asio_tcpip_socket_server_client_timer_bind_handler_multithreading_synchronizing_network_D.php



#include "network_header.hpp"


using boost::asio::ip::tcp;



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
		//sock.non_blocking(false); 
		
		for (;;) {
			size_t len = read(sock, boost::asio::buffer(buf), &completion_condition, err_code);
			
			if (!CheckError(err_code)) {
				if (len) {
					std::cout.write(buf.data(), len);
					std::cout << std::endl;
				}
				std::cout << "Cln out 1\n";
				break;
			}
			
			std::cout.write(buf.data(), len);
			std::cout << std::endl;
			
			std::string data = GetData();
			boost::asio::write (sock, boost::asio::buffer(data), err_code);
			if (!CheckError(err_code)) {
				std::cout << "Cln out 2\n";
				break;
			}
		}
	}
	catch (std::exception &e) {
		std::cout << e.what() << std::endl;
	}
	
	
	return 0;
}
