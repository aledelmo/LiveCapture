#include <opencv2/opencv.hpp>
#include "unistd.h"
#include "zmq.hpp"

using namespace cv;

int main (int argc, char** argv ) {
    void *context = zmq_ctx_new();
    void *publisher = zmq_socket(context, ZMQ_REQ);
    zmq_bind(publisher, "tcp://*:5005");

    Mat image;
    std::string path = "../data/lena.jpg";
    image = imread(path, 1 );

    std::vector <uchar> buffer;
    cv::imencode(".png", image, buffer);
    char request [1];
    while (true) {
        zmq_send(publisher, buffer.data(), buffer.size(), ZMQ_NOBLOCK);
        std::cout<<"new image sent"<<std::endl;
        zmq_recv(publisher, request, 1, ZMQ_NOBLOCK);
        usleep(1000000);
    }
    return 0;
}