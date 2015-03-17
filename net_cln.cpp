
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
			boost::asio::mutable_buffers_1 buf (ReadDataPart(sock));
			char *mem = boost::asio::buffer_cast<char*> (buf);
			size_t len = boost::asio::buffer_size(buf);		
			if (!len) {
				delete [] mem;
				std::cout << "Cln out 1\n";
				break;
			}
			mem[len] = '\0';
			std::cout << mem << std::endl;
			delete [] mem;
			
			
			std::string data = GetData();
			len = WriteDataPart(sock, &std::vector<char>(data.begin(), data.end())[0], data.size());
			if (len != data.size()) {
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
