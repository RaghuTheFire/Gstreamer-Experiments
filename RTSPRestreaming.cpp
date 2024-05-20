
#include <opencv2/opencv.hpp>
#include <gstreamer-1.0/gstreamer.h>

int main(int argc, char** argv) 
{
    cv::VideoCapture cap(0); // Open default camera
    if (!cap.isOpened()) 
    {
        std::cerr << "Failed to open camera" << std::endl;
        return -1;
    }

    // Create GStreamer pipeline
    GstElement* pipeline = gst_pipeline_new("rtsp-stream");
    GstElement* source = gst_element_factory_make("appsrc", "source");
    GstElement* encoder = gst_element_factory_make("x264enc", "encoder");
    GstElement* payloader = gst_element_factory_make("rtph264pay", "payloader");
    GstElement* sink = gst_element_factory_make("udpsink", "sink");

    // Set up pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, encoder, payloader, sink, NULL);
    gst_element_link_many(source, encoder, payloader, sink, NULL);

    // Set RTSP stream properties
    g_object_set(G_OBJECT(sink), "host", "127.0.0.1", "port", 5000, NULL);

    // Start pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    cv::Mat frame;
    while (true) 
    {
        cap >> frame;
        if (frame.empty()) 
        {
            std::cerr << "Failed to capture frame" << std::endl;
            break;
        }

        // Push frame to GStreamer pipeline
        GstBuffer* buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY, frame.data, frame.total() * frame.elemSize(), 0, frame.total() * frame.elemSize(), NULL, NULL);
        GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(source), buffer);
        if (ret != GST_FLOW_OK) 
        {
            std::cerr << "Failed to push buffer to GStreamer pipeline" << std::endl;
            break;
        }
    }

    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    cap.release();

    return 0;
}
