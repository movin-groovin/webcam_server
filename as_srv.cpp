
#include "net.hpp"
#include "cam.hpp"

#include <cstdlib>

#include <unordered_map>
#include <atomic>

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
	if ((m_log_file = ExtractString(data, "[^#]log_file\\s*=\\s*([\\w\\.]+)")) == "") {
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
		PutToLog("\n\n\n\n\n === The log file has opened === ");
		
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
	
	bool WasAuth() const {
		return m_was_auth;
	}
	
	bool IsBad();
	
	unsigned long long GetCount() {
		return m_number_net_operations.load();
	}
	
	virtual ~CTcpConnection() {}
	
private:
	CTcpConnection(
		boost::asio::io_service& io_service,
		std::shared_ptr<CTcpServer> serv
		):
			m_socket(io_service),
			m_serv(serv),
			m_bad_connection(false),
			m_bad_auth(false),
			m_successfull_auth(false),
			m_was_auth(false),
			m_number_net_operations(0)
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
		boost::mutex::scoped_lock lock(m_synch_connecton_flag);
		m_bad_connection = true;
	}
	
	bool CheckAuthInformation();
	
	void OnReadHeader(
		const boost::system::error_code &error,
		size_t bytes_num_transf
	);
	
	void HandleReadOnAuth(
		const boost::system::error_code& error,
		size_t /*bytes_transferred*/
	);
	
	unsigned long long PreIncNetOperationsNumber() {
		return ++m_number_net_operations;
	}
	
	unsigned long long PreDecNetOperationsNumber() {
		return --m_number_net_operations;
	}
	
	unsigned long long GetNetOperationsNumber() {
		return m_number_net_operations.load();
	}
	
	inline bool TestConnectionAndFinishOperation();
	
private:
	tcp::socket m_socket;
	std::vector<char> m_data, auth_buf;
	std::shared_ptr<CTcpServer> m_serv;
	
	boost::mutex m_synch_connecton_flag;
	bool m_bad_connection;
	
	boost::mutex m_synch_auth_flags;
	bool m_bad_auth;
	bool m_successfull_auth;
	bool m_was_auth;
	
	std::atomic<unsigned long long> m_number_net_operations;
};
//--------------------------------------------------------------------
bool CTcpConnection::IsBad() {
	bool ret_val;
	boost::mutex::scoped_lock lock(m_synch_connecton_flag);
	ret_val = m_bad_connection;
	return ret_val;
}
//--------------------------------------------------------------------
bool CTcpConnection::CheckAuthInformation() {
	if (TestConnectionAndFinishOperation()) {
		return false;
	}
	
	auth_buf.resize(sizeof(NetThings::REQUEST_HEADER));
	
	PreIncNetOperationsNumber();
	boost::asio::async_read(
		m_socket,
		boost::asio::buffer(auth_buf),
		std::bind(
			&CTcpConnection::OnReadHeader,
			shared_from_this(),
			std::placeholders::_1,
			std::placeholders::_2
		)
	);
	
	return true;
}

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

		auto end = m_connections.end();
		for (auto it = m_connections.begin(); it != end; ) {
			CTcpConnection::Pointer p = *it;
			if (p->IsBad()) {
				m_bad_connections.push_back(p);
				m_connections.erase(it);
				it = m_connections.begin();
			} else {
				++it;
			}
		}
		
		std::for_each(
			m_connections.begin(),
			m_connections.end(),
			[&cam](CTcpConnection::Pointer p) { p->SendData(cam); }
		);
		
		return;
	}
	
	void ClearBadConnections() {
		boost::mutex::scoped_lock lock(m_conn_mut);
		m_bad_connections.clear();
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
	
	bool HaveConnections() {
		boost::mutex::scoped_lock(m_conn_mut);
		return !m_connections.empty();
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
			boost::mutex::scoped_lock lock(m_conn_mut);
			m_connections.push_front(new_connection);
		}
		StartAccept();
	}
	
private:
	tcp::acceptor m_acceptor;
	
	boost::mutex m_conn_mut;
	std::list<CTcpConnection::Pointer> m_connections;
	std::list<CTcpConnection::Pointer> m_bad_connections;
	
	int m_port;
	unsigned m_max_connections;
};
//--------------------------------------------------------------------
bool CTcpConnection::TestConnectionAndFinishOperation() {
	if (IsBad() && !GetNetOperationsNumber()) {
		return true;
	}
	return IsBad();
}
//--------------------------------------------------------------------
void CTcpConnection::OnSentHeader(
	const boost::system::error_code &error,
	size_t bytes_num_transf
)
{
	PreDecNetOperationsNumber();
	if (TestConnectionAndFinishOperation()) {
		return;
	}
	
	if (error) {
		WriteError(error);
		MarkBad();
		return;
	}
	
	if (!m_data.size()) {
		return;
	}
	
	PreIncNetOperationsNumber();
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
void CTcpConnection::OnReadHeader(
	const boost::system::error_code &error,
	size_t bytes_num_transf
)
{
	PreDecNetOperationsNumber();
	if (TestConnectionAndFinishOperation()) {
		return;
	}
	
	if (error) {
		MarkBad();
		WriteError(error);
		return;
	}
	
	NetThings::REQUEST_HEADER hdr {};
	std::copy(auth_buf.begin(), auth_buf.end(), reinterpret_cast<char*>(&hdr));
	auth_buf.resize(auth_buf.size() + hdr.u.s.size);
	
	if (hdr.u.s.command == NetThings::AuthData) {
		PreIncNetOperationsNumber();
		boost::asio::async_read(
			m_socket,
			boost::asio::buffer(&auth_buf[0] + sizeof hdr, hdr.u.s.size),
			std::bind(
				&CTcpConnection::HandleReadOnAuth,
				shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2
			)
		);
	}
	
	return;
}
//--------------------------------------------------------------------
void CTcpConnection::HandleReadOnAuth(
	const boost::system::error_code& error,
	size_t /*bytes_transferred*/)
{
	PreDecNetOperationsNumber();
	if (TestConnectionAndFinishOperation()) {
		return;
	}
	
	if (error) {
		MarkBad();
		WriteError(error);
	}
	
	boost::mutex::scoped_lock lock(m_synch_auth_flags);
	m_data.resize(0);
	NetThings::REQUEST_HEADER hdr;
	NetThings::FillHeader(hdr, m_data.size());
	
	if(m_was_auth && !m_successfull_auth) {
		std::string auth_str(auth_buf.begin() + sizeof (NetThings::REQUEST_HEADER), auth_buf.end());
		std::string::size_type colon_pos = auth_str.find(NetThings::SeparatorAuthChar);
		
		if (colon_pos == std::string::npos) {
			m_bad_auth = true;
			hdr.u.s.status = NetThings::AuthError;
			
			SendAsyncHeader(hdr);
			WriteError("Illformed auth string");
			
			return;
		}
		
		std::string name(auth_str.begin(), auth_str.begin() + colon_pos);
		std::string pass(auth_str.begin() + colon_pos + 1, auth_str.end());
		
		if (CConfig::GetConfig().CheckAuth(name, pass)) {
			hdr.u.s.status = NetThings::AuthSuccess;
			
			SendAsyncHeader(hdr);
			m_successfull_auth = true;
		} else {

			hdr.u.s.status = NetThings::AuthError;
			m_bad_auth = true;
			
			SendAsyncHeader(hdr);
			WriteError("Incorrect authentication data");
		}
	}
	
	return;
}
//--------------------------------------------------------------------
void CTcpConnection::HandleWrite(
	const boost::system::error_code& error,
	size_t /*bytes_transferred*/)
{
	PreDecNetOperationsNumber();
	if (TestConnectionAndFinishOperation()) {
		return;
	}
	
	if (error) {
		WriteError(error);
		MarkBad();
	}
	
	return;
}
//--------------------------------------------------------------------
void CTcpConnection::SendAsyncHeader(NetThings::REQUEST_HEADER &hdr) {
	if (TestConnectionAndFinishOperation()) {
		return;
	}
	
	try {
		std::vector<char> hdr_buf(sizeof hdr);
		
		std::copy(
			reinterpret_cast<char*>(&hdr),
			reinterpret_cast<char*>(&hdr) + sizeof hdr,
			hdr_buf.begin()
		);
		
		PreIncNetOperationsNumber();
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
//--------------------------------------------------------------------
void CTcpConnection::SendData(Frames::CWebcam &cam) {
	if (TestConnectionAndFinishOperation()) {
		return;
	}

	{
		m_synch_auth_flags.lock();
		if (!m_was_auth) {
			m_was_auth = true;
			m_synch_auth_flags.unlock();
			CheckAuthInformation();
			return;
		}
		if (m_bad_auth) {
			m_synch_auth_flags.unlock();
			WriteError("Bad authentication with client");
			MarkBad();
			return;
		}
		if (!m_successfull_auth) {
			m_synch_auth_flags.unlock();
			return;
		}
		m_synch_auth_flags.unlock();
	}
	
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
//--------------------------------------------------------------------
int main(int argc, char *argv[]) {
	unsigned milisec_sleep = 10;
	unsigned wait_cam_milisec = 1000;
	boost::posix_time::time_duration duration_clear(0, 0, 5);
	
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
	
		boost::posix_time::ptime new_clear_period = boost::posix_time::second_clock::local_time();
		while (true)
		{	
			if (server->HaveConnections()) {
				if (!first_cam.IsOPened())
				{
					if (!first_cam.OpenCamera())
					{
						CLogger::GetLogger().PutToLog("Can't open the camera, wait ...");
						boost::this_thread::sleep(boost::posix_time::milliseconds(wait_cam_milisec));
					}
				}
				
				first_cam.RefreshFrame();
				server->NotifyClients(first_cam);
			} else if (first_cam.IsOPened()) {
				first_cam.CloseCamera();
			}
			
			boost::this_thread::sleep(boost::posix_time::millisec(milisec_sleep));
			
			if (duration_clear < (boost::posix_time::second_clock::local_time() - new_clear_period)) {
				new_clear_period = boost::posix_time::second_clock::local_time();
				server->ClearBadConnections();
				std::cout << "Bad connections clearing\n";
			}
			//
		}
		
		std::cout << "Wating termination...\n";
		thr_grp.join_all();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	
	return 0;
}








