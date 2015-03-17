
// g++ -std=c++11 -I/usr/include/opencv2 -lopencv_highgui -lopencv_core -lopencv_imgproc read_cam.cpp -o read_cam

#include <iostream>
#include <string>

#include <cstdio>

#include <highgui/highgui.hpp>
#include <opencv2/opencv.hpp>



cv::Mat MakeLinear (const cv::Mat & mat) {
	return mat.reshape (0, 1);
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


void PrintImageInfo(const cv::Mat & frame) {
	std::cout << "Matrix width: " << frame.size().width << std::endl;
	std::cout << "Matrix height: " << frame.size().height << std::endl;
	std::cout << "Matrix size: " << frame.rows * frame.cols << std::endl;
	std::cout << "Channels: " << frame.channels() << std::endl;
	std::cout << "Continuous: : " << static_cast<bool>(frame.flags & cv::Mat::CONTINUOUS_FLAG) << std::endl;
	std::cout << "Size: " << (frame.rows * frame.cols * frame.channels() / 1024) << " KB\n";
	std::cout << "Rows: " << frame.rows << " Cols: " << frame.cols<< '\n';
	std::cout << "==========================================================================\n";
	
	return;
}


int main(int, char**)
{
    cv::VideoCapture cap(0); // open the default camera
    if(!cap.isOpened()) {  // check if we succeeded
        return -1;
    }
    cap.set (CV_CAP_PROP_FRAME_WIDTH, 480); // 480 640
    cap.set (CV_CAP_PROP_FRAME_HEIGHT, 320); // 320 480

    cv::Mat edges;
    cv::namedWindow("edges",1);
    for(;;)
    {
        cv::Mat frame;
        
        cap >> frame; // get a new frame from camera
        
        PrintImageInfo(frame);
        cv::Mat new_frame = MakeLinear(frame);
        cv::Mat img = RestoreFromBuff(new_frame.data, frame.rows, frame.cols);
		//PrintImageInfo(new_frame);
        //PrintImageInfo(img);
        
        cv::cvtColor(img, edges, CV_BGR2GRAY);
        //PrintImageInfo(edges);
        std::cout << "\n\n";
        
        cv::GaussianBlur(edges, edges, cv::Size(7,7), 1.5, 1.5);
        cv::Canny(edges, edges, 0, 30, 3);
        //cv::imshow("edges", img);
        cv::imshow("edges", edges);
        
        // create socket (host, port)
        
        if(cv::waitKey(30) >= 0) break;
    }
    
    
    // the camera will be deinitialized automatically in VideoCapture destructor
    return 0;
}

