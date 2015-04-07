
#include "net.hpp"
#include "cam.hpp"

#include <unordered_map>

#include <boost/regex.hpp>



class CConfig {
public:
	static CConfig& GetConfig() {
		static CConfig obj;
		return obj;
	}
	
	void Read (const std::string &path) {
		m_path = path;
		CaptureLock();
		try {
			DoRead();
		} catch (...) {
			ReleaseLock();
			throw;
		}
		ReleaseLock();
	}
	
	unsigned GetPort () const {
		return m_port;
	}
	unsigned GetMaxThreads () const {
		return m_max_threads;
	}
	unsigned GetMaxConnections () const {
		return m_max_connections;
	}
	std::string GetLogPath() const {
		return m_log_file;
	}
	bool CheckAuth(const std::string &name, const std::string &pass) const {
		std::unordered_map<std::string, std::string>::const_iterator it = m_auth_info.find(name);
		
		if (it == m_auth_info.end()) {
			return false;
		}
		
		return it->second == pass;
	}
	
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
		if (ret = pthread_rwlock_rdlock(&m_synch))
		{
			throw std::logic_error(ErrorToString(ret));
		}
#else
		pthread_rwlock_rdlock(&m_synch);
#endif
	}
	
	void ReleaseLock() {
		pthread_rwlock_unlock(&m_synch);
	}
	
	void DoRead();
	
	unsigned ExtractValue(const std::string &data, const std::string &pattern);
	
	std::string ExtractString(const std::string &data, const std::string &pattern);
	
	void ExtraxtAuthData(const std::string &data);
	
	unsigned m_max_connections;
	unsigned m_port;
	unsigned m_max_threads;
	std::string m_log_file;
	std::unordered_map<std::string, std::string> m_auth_info;
	
	std::string m_path;
	bool m_read;
	
	pthread_rwlock_t m_synch;
};


unsigned CConfig::ExtractValue(const std::string &data, const std::string &pattern) {
	boost::match_results<std::string::const_iterator> reg_res;
	unsigned value;
//std::cout << data << "\n\n\n";
	if (!boost::regex_search(data, reg_res, boost::regex(pattern))) {
		return -1;
	}
	else {
		std::stringstream ioss;
		ioss << std::string (reg_res[1].first, reg_res[1].second);
		ioss >> value;
	}
	
	return value;
}


std::string CConfig::ExtractString(const std::string &data, const std::string &pattern) {
	boost::match_results<std::string::const_iterator> reg_res;
	
	if (!boost::regex_search(data, reg_res, boost::regex(pattern))) {
		return "";
	}
	else {
		return std::string (reg_res[1].first, reg_res[1].second);
	}
}


void CConfig::ExtraxtAuthData(const std::string &data) {
	boost::match_results<std::string::const_iterator> reg_res;
	std::string pattern = "[^#]auth\\s*=\\s*([\\w\\-]+)[:]([\\w\\-]+)";
	boost::regex reg_val(pattern);
	size_t num = 0;
	
	std::string::const_iterator it = data.begin(), end = data.end();
	while (boost::regex_search(it, end, reg_res, reg_val)) {
		m_auth_info.insert(
			std::make_pair (
				std::string(reg_res[1].first, reg_res[1].second),
				std::string(reg_res[2].first, reg_res[2].second)
			)
		);
		
		it = reg_res[0].second;
		++num;
	}
	if (!num)
		throw std::runtime_error("Not found auth information");
	
	return;
}


void CConfig::DoRead() {
	std::ifstream ifs(m_path);
	std::string text, data;
	
	while(std::getline(ifs, text)) {
		data.append(text + '\n');
	}
	
	if ((m_max_connections = ExtractValue(data, "[^#]max_connections\\s*=\\s*(\\d+)")) == static_cast<unsigned>(-1)) {
		throw std::runtime_error("Can't find max_connections parameter in config file: " + m_path);
	}
	if ((m_max_threads = ExtractValue(data, "[^#]worker_threads_number\\s*=\\s*(\\d+)")) == static_cast<unsigned>(-1)) {
		throw std::runtime_error("Can't find worker_threads_number parameter in config file: " + m_path);
	}
	if ((m_port = ExtractValue(data, "[^#]port\\s*=\\s*(\\d+)")) == static_cast<unsigned>(-1)) {
		throw std::runtime_error("Can't find port parameter in config file: " + m_path);
	}
	if ((m_log_file = ExtractString(data, "[^#]log_file\\s*=\\s*(\\s+)")) == "") {
		throw std::runtime_error("Can't find port parameter in config file: " + m_path);
	}
	ExtraxtAuthData(data);
	
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
	static std::shared_ptr<CTcpServer> MakeTcpServer(
		boost::asio::io_service& io_service,
		int port,
		unsigned max_connections
	) {
		return std::shared_ptr<CTcpServer> (new CTcpServer (io_service, port, max_connections));
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
	CTcpServer(boost::asio::io_service& io_service, int port, unsigned max_connections):
		m_acceptor(io_service, tcp::endpoint(tcp::v4(), port)),
		m_port(port),
		m_max_connections(max_connections)
	{
		return;
	}
	
	void HandleAccept(
		CTcpConnection::Pointer new_connection,
		const boost::system::error_code& error
	)
	{
		if (!error) {
			if (CheckAuthInformation(new_connection)) {
				boost::mutex::scoped_lock lock(m_conn_mut);
				m_connections.push_front(new_connection);
			}
		}
		StartAccept();
	}
	
	bool CheckAuthInformation(CTcpConnection::Pointer connection);
	
private:
	tcp::acceptor m_acceptor;
	
	boost::mutex m_conn_mut;
	std::list<CTcpConnection::Pointer> m_connections;
	
	int m_port;
	unsigned m_max_connections;
};


bool CTcpServer::CheckAuthInformation(CTcpConnection::Pointer connection) {
	
	
	//
	
	
	return true;
}

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

int main(int argc, char *argv[]) {
	unsigned milisec_sleep = 10;
	
	if (argc < 2) {
		std::cout << "Enter a path to config file\n";
		return 1001;
	}
	
	try {
		CConfig::GetConfig().Read(argv[1]);
		CLogger::GetLogger().OpenLog(CConfig::GetConfig().GetLogPath());
		
		boost::thread_group thr_grp;
		boost::asio::io_service io_service;
	
		std::shared_ptr<CTcpServer> server(CTcpServer::MakeTcpServer(
			io_service,
			CConfig::GetConfig().GetPort(),
			CConfig::GetConfig().GetMaxConnections())
		);	
		server->StartAccept();
		
		for (unsigned i = 0; i < CConfig::GetConfig().GetMaxThreads(); ++i) {
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








