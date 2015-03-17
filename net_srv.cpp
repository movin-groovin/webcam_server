
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
				size_t len = WriteDataPart(sock, &std::vector<char>(data.begin(), data.end())[0], data.size());
				if (len != data.size()) {
					std::cout << "Serv out 1\n";
					break;
				}
				
				
				boost::asio::mutable_buffers_1 buf (ReadDataPart(sock));
				char *mem = boost::asio::buffer_cast<char*> (buf);
				len = boost::asio::buffer_size(buf);
				if (!len) {
					delete [] mem;
					std::cout << "Serv out 2\n";
					break;
				}
				mem[len] = '\0';
				std::cout << mem << std::endl;
				delete [] mem;
			}
			//
		}
	}
	catch (std::exception & exc) {
		std::cout << exc.what() << std::endl;
	}
	
	
	return 0;
}
