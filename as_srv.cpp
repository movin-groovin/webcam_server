
#include "net.hpp"
#include "cam.hpp"



class CTcpConnection: public boost::enable_shared_from_this<CTcpConnection> {
public:
	typedef boost::shared_ptr<CTcpConnection> Pointer;
	
	static Pointer Create(
		boost::asio::io_service& io_service,
		std::shared_ptr <CTcpServer> serv
	)
	{
		return Pointer(new CTcpConnection(io_service, serv));
	}
	
	tcp::socket& Socket() {
		return m_socket;
	}
	
	void SendData(Frames::CWebcam &cam) {
		cam.GetData(m_data);
		
		NetThings::REQUEST_HEADER hdr;
		NetThings::FillHeader(hdr, m_data.size(), cam.GetHeight(), cam.GetWidth());
		
		SendAsyncHeader(hdr);
		
		return;
	}
	
	virtual void WriteError(const boost::system::error_code &error) {
		std::cout << "Boost asio error code: " << error << std::endl;
	}
	
	virtual ~CTcpConnection() {}
	
private:
	CTcpConnection(
		boost::asio::io_service& io_service,
		std::shared_ptr <CTcpServer> serv
		):
			m_socket(io_service),
			m_serv(serv)
		{}
	
	void SendAsyncHeader(NetThings::REQUEST_HEADER &hdr) {
		std::vector<char> hdr_buf(sizeof hdr);
		std::copy(
			reinterpret_cast<char*>(&hdr),
			reinterpret_cast<char*>(&hdr) + sizeof hdr,
			hdr_buf.begin()
		);
		boost::asio::async_write(
			m_socket,
			boost::asio::buffer(hdr_buf),
			std::bind(
				&CTcpConnection::OnSentHeader,
				shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2
			)
		);
		
		return;
	}
	
	void HandleWrite(const boost::system::error_code& error, size_t /*bytes_transferred*/);
	
	void OnSentHeader(
		const boost::system::error_code &error,
		size_t bytes_num_transf
	);
	
private:
	tcp::socket m_socket;
	std::vector<char> m_data;
	std::shared_ptr <CTcpServer> m_serv;
};

//--------------------------------------------------------------------

class CTcpServer: public std::enable_shared_from_this<CTcpServer> {
public:
	static std::shared_ptr<CTcpServer> MakeTcpServer(boost::asio::io_service& io_service, int port) {
		return std::shared_ptr<CTcpServer> (new CTcpServer (io_service, port));
	}
	
	void NotifyClients(Frames::CWebcam &cam) {
		boost::mutex::scoped_lock lock(m_conn_mut);
		
		std::for_each(
			m_connections.begin(),
			m_connections.end(),
			[&cam](CTcpConnection::Pointer p) { p->SendData(cam); }
		);
		
		return;
	}
	
	void RemoveConnection(CTcpConnection::Pointer p) {
		boost::mutex::scoped_lock lock(m_conn_mut);
		m_connections.remove(p);
	}
	
	void StartAccept()
	{
		CTcpConnection::Pointer new_connection =
			CTcpConnection::Create(m_acceptor.get_io_service(), shared_from_this());
			
		m_acceptor.async_accept(
			new_connection->Socket(),
			boost::bind(
				&CTcpServer::HandleAccept,
				shared_from_this(),
				new_connection,
				boost::asio::placeholders::error
			)
		);
		
		return;
	}
	
private:
	CTcpServer(boost::asio::io_service& io_service, int port):
		m_acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
		m_port(port)
	{
		return;
	}
	
	void HandleAccept(
		CTcpConnection::Pointer new_connection,
		const boost::system::error_code& error
	)
	{
		if (!error) {
			boost::mutex::scoped_lock lock(m_conn_mut);
			m_connections.push_front(new_connection);
		}
		StartAccept();
	}
	
private:
	tcp::acceptor m_acceptor;
	boost::mutex m_conn_mut;
	std::list<CTcpConnection::Pointer> m_connections;
	int m_port;
};

//--------------------------------------------------------------------

void CTcpConnection::OnSentHeader(
	const boost::system::error_code &error,
	size_t bytes_num_transf
)
{
	if (error) {
		WriteError(error);
		m_serv->RemoveConnection(shared_from_this());
	}
	
	boost::asio::async_write(
		m_socket,
		boost::asio::buffer(m_data),
		std::bind(
			&CTcpConnection::HandleWrite,
			shared_from_this(),
			std::placeholders::_1,
			std::placeholders::_2
		)
	);
	
	return;
}

//--------------------------------------------------------------------

void CTcpConnection::HandleWrite(
	const boost::system::error_code& error,
	size_t /*bytes_transferred*/)
{
	if (error) {
		WriteError(error);
		m_serv->RemoveConnection(shared_from_this());
	}
	
	return;
}

//--------------------------------------------------------------------

int main() {
	const unsigned thr_number = 5;
	const unsigned milisec_sleep = 10;
	const int port = 8899;
	
	try {
		boost::thread_group thr_grp;
		boost::asio::io_service io_service;
		std::shared_ptr<CTcpServer> server(CTcpServer::MakeTcpServer(io_service, port));
		
		server->StartAccept();
		
		for (unsigned i = 0; i < thr_number; ++i) {
			thr_grp.create_thread( [&]()->void {io_service.run();} );
		}
		
		Frames::CWebcam first_cam;
		
		while (true)
		{
			first_cam.RefreshFrame();
			server->NotifyClients(first_cam);
			boost::this_thread::sleep(boost::posix_time::millisec(milisec_sleep));
		}
		
		std::cout << "Wating termination...\n";
		thr_grp.join_all();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}








