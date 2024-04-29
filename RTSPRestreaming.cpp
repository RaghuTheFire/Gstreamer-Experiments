#include <opencv2/opencv.hpp>
#include <gst/gst.h>

int main(int argc, char * argv[]) 
{
  // Initialize OpenCV video capture
  cv::VideoCapture cap(0); // Open default camera

  // Initialize GStreamer
  gst_init( & argc, & argv);

  // Create GStreamer pipeline
  GstElement * pipeline = gst_pipeline_new("rtsp-restreamer");
  GstElement * source = gst_element_factory_make("appsrc", "source");
  GstElement * encoder = gst_element_factory_make("x264enc", "encoder");
  GstElement * payloader = gst_element_factory_make("rtph264pay", "payloader");
  GstElement * sink = gst_element_factory_make("udpsink", "sink");

  // Set up pipeline elements
  g_object_set(G_OBJECT(source), "is-live", TRUE, "format", GST_FORMAT_TIME, NULL);
  g_object_set(G_OBJECT(sink), "host", "127.0.0.1", "port", 5000, NULL);

  // Link pipeline elements
  gst_bin_add_many(GST_BIN(pipeline), source, encoder, payloader, sink, NULL);
  gst_element_link_many(source, encoder, payloader, sink, NULL);

  // Start pipeline
  gst_element_set_state(pipeline, GST_STATE_PLAYING);

  // Capture and stream video frames
  cv::Mat frame;
  while (true) 
  {
    cap >> frame;
    if (frame.empty()) 
    {
      break;
    }

    // Push frame to GStreamer pipeline
    GstBuffer * buffer = gst_buffer_new_wrapped_full(GST_MEMORY_FLAG_READONLY,frame.data, frame.total() * frame.elemSize(),0, frame.total() * frame.elemSize(), NULL, NULL);
    GstFlowReturn ret = gst_app_src_push_buffer(GST_APP_SRC(source), buffer);
    if (ret != GST_FLOW_OK) 
    {
      // Handle error
      break;
    }
  }

  // Clean up
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);
  cap.release();

  return 0;
}
