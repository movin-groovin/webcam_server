
#include "net.hpp"
#include "cam.hpp"



class CLogger: boost::noncopyable {
public:
	void OpenLog(const std::string &path) {
		m_path = path;
		
		if (!m_ofs.is_open()) {
			m_ofs.open(m_path, std::ios_base::out | std::ios_base::app);
			if (!m_ofs.is_open())
				throw std::runtime_error("Can't open log file: " + m_path);
		}
		
		return;
	}

	virtual void PutToLog(const std::string &msg) {
		assert(!m_path.empty());
		assert(m_ofs.is_open());
		
		m_ofs << msg << "\n";
		
		return;
	}
	
	static CLogger& GetLogger() {
		static CLogger logger;
		return logger;
	}
	
private:
	CLogger(){}
	
	std::string m_path;
	std::ofstream m_ofs;
};



class CTcpConnection: public boost::enable_shared_from_this<CTcpConnection> {
public:
	typedef boost::shared_ptr<CTcpConnection> Pointer;
	
	static Pointer Create(boost::asio::io_service& io_service)
	{
		return Pointer(new CTcpConnection(io_service));
	}
	
	tcp::socket& Socket() {
		return m_socket;
	}
	
	void WriteError(const boost::system::error_code &error) {
		std::ostringstream oss;
		
		oss << error.value();
		std::string msg = "Boost asio error code: " + oss.str() + "; message: " + error.message();
		CLogger::GetLogger().PutToLog(msg);
		
		return;
	}
	

	void SendData(Frames::CWebcam &cam) {
		try {
			cam.GetData(m_data);
		} catch (std::exception &exc) {
			WriteError(exc.what());
			MarkBad();
			return;
		}
		
		NetThings::REQUEST_HEADER hdr;
		NetThings::FillHeader(hdr, m_data.size(), cam.GetHeight(), cam.GetWidth());
		
		SendAsyncHeader(hdr);
		
		return;
	}
	
	void WriteError(const std::string &msg) {
		CLogger::GetLogger().PutToLog(msg);
		return;
	}
	
	bool IsBad() {
		boost::mutex::scoped_lock lock(m_synch);
		return m_bad_connection;
	}
	
	virtual ~CTcpConnection() {}
	
private:
	CTcpConnection(
		boost::asio::io_service& io_service
		):
			m_socket(io_service),
			m_bad_connection(false)
		{}
	
	void SendAsyncHeader(NetThings::REQUEST_HEADER &hdr) {
		try {
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
		} catch (std::exception &exc)
		{
			WriteError(exc.what());
			MarkBad();
			
			return;
		}
		
		return;
	}
	
	void OnSentHeader(
		const boost::system::error_code &error,
		size_t bytes_num_transf
	)
	{
		if (error) {
			WriteError(error);
			MarkBad();
			return;
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

	void HandleWrite(
		const boost::system::error_code& error,
		size_t /*bytes_transferred*/)
	{
		if (error) {
			WriteError(error);
			MarkBad();
		}
		
		return;
	}
	
	void MarkBad() {
		boost::mutex::scoped_lock lock(m_synch);
		m_bad_connection = true;
	}
	
private:
	tcp::socket m_socket;
	std::vector<char> m_data;
	boost::mutex m_synch;
	bool m_bad_connection;
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
			[&cam](CTcpConnection::Pointer p) { if (!p->IsBad()) p->SendData(cam); }
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
			CTcpConnection::Create(m_acceptor.get_io_service());
			
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
	
	void ClearBadConnections() {
		auto end = m_connections.end();
		for (auto i = m_connections.begin(); i != end; ++i) {
			if ((*i)->IsBad())
				m_connections.remove(*i);
		}
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



int main() {
	unsigned thr_number = 5;
	unsigned milisec_sleep = 10;
	const int port = 8899;
	const unsigned max_connections = 100;
	const std::string log = "async_log.txt";
	std::string config = "conf.txt";
	std::unordered_map <std::string, std::string> user_data;
	user_data["one"] = "two";
	
	try {
		CLogger::GetLogger().OpenLog(log);
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
			server->ClearBadConnections();
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








