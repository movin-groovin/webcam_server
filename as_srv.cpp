
#include "net.hpp"
#include "cam.hpp"



class CConfig {
public:
	static CConfig& GetConfig() {
		static CConfig obj;
		return obj;
	}
	
	unsigned m_max_connections;
	unsigned m_port;
	unsigned m_max_threads;
	std::unordered_map<std::string, std::string> m_auth_info;
	
private:
	CConfig():
		m_max_connections(0),
		m_port(-1),
		m_max_threads(0),
		m_auth_info(1024),
		m_read(false)
	{
		int ret;
		if ((ret = pthread_rwlock_init(&m_synch, nullptr))) {
			throw std::runtime_error(ErrorToString(ret));
		}
		
		return;
	}
	
	virtual ~ CConfig() {
		pthread_rwlock_destroy(&m_synch);
	}
	
	void CaptureLock() {
#ifdef MY_OWN_DEBUG_1
		int ret;
		if (ret = pthread_rwlock_rdlock(&m_rwlock))
		{
			throw std::logic_error(ErrorToString(ret));
		}
#else
		pthread_rwlock_rdlock(&m_rwlock);
#endif
		
		if (dat.size() < m_data.size()) {
			try {
				dat.resize(m_data.size());
			} catch (...) {
				pthread_rwlock_unlock(&m_rwlock);
				throw;
			}
		}
		std::copy(m_data.begin(), m_data.end(), dat.begin());
		
		pthread_rwlock_unlock(&m_rwlock);
	}
	
	void ReleaseLock {
		pthread_rwlock_unlock(&m_rwlock);
	}
	
	void Read () {
#ifdef MY_OWN_DEBUG_1
		int ret;
		if (ret = pthread_rwlock_wrlock(&m_rwlock))
		{
			throw std::logic_error(ErrorToString(ret));
		}
#else
		pthread_rwlock_wrlock(&m_rwlock);
#endif
		try {
			DoRead();
		} catch (...) {
			pthread_rwlock_unlock(&m_rwlock);
			throw;
		}
		pthread_rwlock_unlock(&m_rwlock);
	}
	
	unsigned m_max_connections;
	unsigned m_port;
	unsigned m_max_threads;
	std::unordered_map<std::string, std::string> m_auth_info;
	
	std::string m_path;
	bool m_read;
	
	pthread_rwlock_t m_synch;
};

void CConfig::DoRead() {
	return;
}



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
		m_ofs.flush();
		
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
	
	static Pointer Create(
		boost::asio::io_service& io_service,
		std::shared_ptr<CTcpServer> serv
	)
	{
		return Pointer(new CTcpConnection(io_service, serv));
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

	void SendData(Frames::CWebcam &cam);
	
	void WriteError(const std::string &msg) {
		CLogger::GetLogger().PutToLog(msg);
		return;
	}
	
	bool IsBad() {
		bool ret_val;
		boost::mutex::scoped_lock lock(m_synch);
		ret_val = m_bad_connection;
		return ret_val;
	}
	
	virtual ~CTcpConnection() {}
	
private:
	CTcpConnection(
		boost::asio::io_service& io_service,
		std::shared_ptr<CTcpServer> serv
		):
			m_socket(io_service),
			m_serv(serv),
			m_bad_connection(false)
		{}
	
	void SendAsyncHeader(NetThings::REQUEST_HEADER &hdr);
	
	void OnSentHeader(
		const boost::system::error_code &error,
		size_t bytes_num_transf
	);

	void HandleWrite(
		const boost::system::error_code& error,
		size_t /*bytes_transferred*/
	);
	
	void MarkBad() {
		boost::mutex::scoped_lock lock(m_synch);
		m_bad_connection = true;
	}
	
private:
	tcp::socket m_socket;
	std::vector<char> m_data;
	std::shared_ptr<CTcpServer> m_serv;
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
void CTcpConnection::SendAsyncHeader(NetThings::REQUEST_HEADER &hdr) {
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
		m_serv->RemoveConnection(shared_from_this());
		return;
	}
	
	return;
}
//--------------------------------------------------------------------
void CTcpConnection::SendData(Frames::CWebcam &cam) {
	try {
		cam.GetData(m_data);
	} catch (std::exception &exc) {
		WriteError(exc.what());
		m_serv->RemoveConnection(shared_from_this());
		return;
	}
	
	NetThings::REQUEST_HEADER hdr;
	NetThings::FillHeader(hdr, m_data.size(), cam.GetHeight(), cam.GetWidth());
	
	SendAsyncHeader(hdr);
	
	return;
}

//--------------------------------------------------------------------

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








