
#include <iostream>
#include <opencv2/opencv.hpp>
#include <gstreamer-1.0/gstreamer.h>

int main(int argc, char** argv) 
{
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the pipeline
    GstElement* pipeline = gst_pipeline_new("pipeline");

    // Create the source element
    GstElement* source = gst_element_factory_make("tcpclientsrc", "source");
    g_object_set(G_OBJECT(source), "host", "192.168.1.10", NULL);

    // Create the H.264 decoder
    GstElement* decoder = gst_element_factory_make("avdec_h264", "decoder");

    // Create the video sink
    GstElement* sink = gst_element_factory_make("appsink", "sink");
    g_object_set(G_OBJECT(sink), "sync", FALSE, NULL);

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), source, decoder, sink, NULL);

    // Link the elements
    gst_element_link_many(source, decoder, sink, NULL);

    // Start the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Create OpenCV window
    cv::namedWindow("H.264 Stream", cv::WINDOW_AUTOSIZE);

    // Continuously read frames from the pipeline and display them
    while (true) 
    {
        GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
        if (sample) 
        {
            GstBuffer* buffer = gst_sample_get_buffer(sample);
            GstMapInfo map;
            gst_buffer_map(buffer, &map, GST_MAP_READ);

            // Create OpenCV Mat from the buffer data
            cv::Mat frame(cv::Size(640, 480), CV_8UC3, map.data, cv::Mat::AUTO_STEP);

            // Display the frame
            cv::imshow("H.264 Stream", frame);

            gst_buffer_unmap(buffer, &map);
            gst_sample_unref(sample);

            // Check for key press
            int key = cv::waitKey(1);
            if (key == 27) // Escape key
                break;
        }
    }

    // Stop the pipeline and clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
