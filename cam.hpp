
#ifndef _CAM_HPP
#define _CAM_HPP



#include <cerrno>
#include <cstring>

#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>
#include <functional>
#include <algorithm>

#include <highgui/highgui.hpp>
#include <opencv2/opencv.hpp>

#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

#include <pthread.h>



std::string ErrorToString(int error);



namespace Frames {


cv::Mat RestoreFromBuff (unsigned char *buf, int orig_rows, int orig_cols);


class CWebcam: boost::noncopyable
{
private:
	cv::Mat MakeLinear (const cv::Mat & mat) {
		return mat.reshape (0, 1);
	}

	size_t GetMatSize(cv::Mat *frame) {
		return frame->rows * frame->cols * frame->channels();
	}
	
	cv::VideoCapture AdjustCamera (size_t height, size_t width, size_t channels, int cam_number) {
		cv::VideoCapture cap(cam_number);
		if(!cap.isOpened()) {
			std::ostringstream oss;
			oss << cam_number;
			throw std::runtime_error ("Can't connect to " + oss.str() + " camera device");
		}
		cap.set (CV_CAP_PROP_FRAME_WIDTH, width);
		cap.set (CV_CAP_PROP_FRAME_HEIGHT, height);
		m_height_frame = cap.get (CV_CAP_PROP_FRAME_HEIGHT);
		m_width_frame = cap.get (CV_CAP_PROP_FRAME_WIDTH);
		m_data.resize(height * width * channels);
		
		return cap;
	}
	
	struct CRWLockHolderRead: private boost::noncopyable {
		CRWLockHolderRead(pthread_rwlock_t & ref): ref_rwlock(ref) {
#ifdef MY_OWN_DEBUG_1
			int ret;
			if (ret = pthread_rwlock_rdlock(&ref_rwlock);)
			{
				throw std::logic_error(ErrorToString(ret));
			}
#else
			pthread_rwlock_rdlock(&ref_rwlock);
#endif
		}
		~CRWLockHolderRead() {
			pthread_rwlock_unlock(&ref_rwlock);
		}
		private:
			pthread_rwlock_t & ref_rwlock;
	};
	
	struct CRWLockHolderWrite: private boost::noncopyable {
		CRWLockHolderWrite(pthread_rwlock_t & ref): ref_rwlock(ref) {
#ifdef MY_OWN_DEBUG_1
			int ret;
			if (ret = pthread_rwlock_wrlock(&ref_rwlock);)
			{
				throw std::logic_error(ErrorToString(ret));
			}
#else
			pthread_rwlock_wrlock(&ref_rwlock);
#endif
		}
		~CRWLockHolderWrite() {
			pthread_rwlock_unlock(&ref_rwlock);
		}
		private:
			pthread_rwlock_t & ref_rwlock;
	};
	
public:
	virtual ~CWebcam() {
		pthread_rwlock_destroy(&m_rwlock);
	}
	
	CWebcam(size_t cam_number = 0, size_t height = 320, size_t width = 480, size_t channels = 3):
		m_data(height * width * channels),
		m_channels(channels),
		m_cam_number(cam_number),
		m_cap(AdjustCamera(height, width, channels, cam_number))
	{
		int ret;
		if ((ret = pthread_rwlock_init(&m_rwlock, nullptr))) {
			throw std::runtime_error(ErrorToString(ret));
		}
		
		return;
	}
	
	bool CloseCamera() {
		CRWLockHolderWrite lock(m_rwlock);
		m_cap.release();
		return true;
	}
	
	bool IsOPened() const {
		return m_cap.isOpened();
	}
	
	bool OpenCamera () {
		CRWLockHolderWrite lock(m_rwlock);
		try {
			m_cap = AdjustCamera(m_height_frame, m_width_frame, m_channels, m_cam_number);
		} catch (std::exception & Exc) {
			// WriteToLog("Can't open camera, " + Exc.what());
			return false;
		}
		return true;
	}

	void RefreshFrame() {
		CRWLockHolderWrite lock(m_rwlock);
		cv::Mat frame;
		
		m_cap >> frame; // get a new frame from camera
		cv::Mat new_frame (MakeLinear(frame));
		std::copy_n(new_frame.data, GetMatSize(&new_frame), m_data.begin());
		/*auto iter = m_data.begin();
		for (int i = 0; i < GetMatSize(&new_frame); ++i, ++iter) {
			*iter = new_frame.data[i];
		}*/
		
		return;
	}
	
	size_t GetWidth() const {return m_width_frame;}
	size_t GetHeight() const {return m_height_frame;}
	
	void GetData(std::vector<char> & dat);
	
private:
	std::vector<char> m_data;
	
	mutable pthread_rwlock_t m_rwlock;
	
	size_t m_height_frame;
	size_t m_width_frame;
	size_t m_channels;
	size_t m_cam_number;
	cv::VideoCapture m_cap;
};


} // Frames



#endif // _CAM_HPP
