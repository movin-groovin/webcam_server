
#include "async.hpp"



std::string make_daytime_string() {
	using namespace std; // For time_t, time and ctime;
	time_t now = time(0);
	return ctime(&now);
}

//--------------------------------------------------------------------

class tcp_connection: public boost::enable_shared_from_this<tcp_connection> {
public:
	typedef boost::shared_ptr<tcp_connection> pointer;
	
	static pointer create(boost::asio::io_service& io_service) {
		return pointer(new tcp_connection(io_service));
	}
	
	tcp::socket& socket() {
		return socket_;
	}
	
	void start() {
		message_ = make_daytime_string();
		
		boost::asio::async_write(
			socket_,
			boost::asio::buffer(message_),
			boost::bind(
				&tcp_connection::handle_write,
				shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		);
		
		return;
	}
	
private:
	tcp_connection(boost::asio::io_service& io_service): socket_(io_service)
	{}
	
	void handle_write(
		const boost::system::error_code& /*error*/,
		size_t /*bytes_transferred*/)
	{
		return;
	}
	
private:
	tcp::socket socket_;
	std::string message_;
};
//--------------------------------------------------------------------
class tcp_server {
public:
	tcp_server(boost::asio::io_service& io_service):
		acceptor_(io_service, tcp::endpoint(tcp::v4(), 8899))
	{
		start_accept();
	}
	
private:
	void start_accept() {
		tcp_connection::pointer new_connection =
			tcp_connection::create(acceptor_.get_io_service());
			
		acceptor_.async_accept(
			new_connection->socket(),
			boost::bind(
				&tcp_server::handle_accept,
				this,
				new_connection,
				boost::asio::placeholders::error
			)
		);
		
		return;
	}
	
	void handle_accept(
		tcp_connection::pointer new_connection,
		const boost::system::error_code& error
	)
	{
		if (!error) {
			new_connection->start();
		}
		start_accept();
	}
	
private:
	tcp::acceptor acceptor_;
};

//--------------------------------------------------------------------

int main() {
	const unsigned thr_number = 5;
	
	try {
		boost::thread_group thr_grp;
		boost::asio::io_service io_service;
		tcp_server server(io_service);
		
		for (unsigned i = 0; i < thr_number; ++i) {
			thr_grp.create_thread( [&]()->void {io_service.run();} );
		}
		
		std::cout << "Wating ...\n";
		thr_grp.join_all();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}








