
#include "cam.hpp"



std::string ErrorToString(int error) {
	std::vector<char> buf(4);
	
	while (strerror_r(error, &buf[0], buf.size()) && errno == ERANGE) {
		buf.resize(buf.size());
	}
	if (errno == EINVAL) {
		std::ostringstream oss;
		oss << error;
		return "The value of error: " + oss.str() + ", is not a valid error number";
	}
	std::stringstream oss;
	oss << error;
	
	return std::string(buf.begin(), buf.end()) + "; Error code: " + oss.str();
}



namespace Frames {


void CWebcam::GetData(std::vector<char> & dat) {
	if (dat.size() < m_data.size()) {
		CRWLockHolderWrite lock(m_rwlock);
		dat.resize(m_data.size());
	}
	CRWLockHolderRead lock(m_rwlock);
	std::copy(m_data.begin(), m_data.end(), dat.begin());
	
	return;
}



cv::Mat RestoreFromBuff (unsigned char *buf, int orig_rows, int orig_cols) {
	cv::Mat img = cv::Mat::zeros(orig_rows, orig_cols, CV_8UC3);
	size_t offset = 0;
	
	for (int i = 0;  i < img.rows; i++) {
		for (int j = 0; j < img.cols; j++) {
			img.at<cv::Vec3b>(i,j) = cv::Vec3b(
				buf[offset + 0],
				buf[offset + 1],
				buf[offset + 2]
			);
			offset += 3;
		}
	}
	
	return img;
}


} // Frames








