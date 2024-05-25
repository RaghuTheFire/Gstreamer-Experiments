#include <iostream>
#include <opencv2/opencv.hpp>
#include <gstreamer-1.0/gstreamer.h>

int main(int argc, char** argv) 
{
    // Initialize GStreamer
    gst_init(&argc, &argv);

    // Create the pipeline
    GstElement* pipeline = gst_pipeline_new("udp-h264-pipeline");

    // Create the UDP source
    GstElement* udpsrc = gst_element_factory_make("udpsrc", "udp-source");
    g_object_set(G_OBJECT(udpsrc), "uri", "udp://192.168.1.10:5000", NULL);

    // Create the H.264 decoder
    GstElement* h264dec = gst_element_factory_make("avdec_h264", "h264-decoder");

    // Create the video sink
    GstElement* appsink = gst_element_factory_make("appsink", "app-sink");

    // Add elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), udpsrc, h264dec, appsink, NULL);

    // Link the elements
    gst_element_link_many(udpsrc, h264dec, appsink, NULL);

    // Set the appsink to emit signals when new data is available
    GstAppSinkCallbacks callbacks;
    callbacks.eos = NULL;
    callbacks.new_preroll = NULL;
    callbacks.new_sample = [](GstAppSink* sink, gpointer user_data) 
	{
        GstSample* sample = gst_app_sink_pull_sample(sink);
        GstCaps* caps = gst_sample_get_caps(sample);
        GstStructure* structure = gst_caps_get_structure(caps, 0);
        gint width, height;
        gst_structure_get_int(structure, "width", &width);
        gst_structure_get_int(structure, "height", &height);

        GstBuffer* buffer = gst_sample_get_buffer(sample);
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_READ);

        // Create OpenCV Mat from the buffer data
        cv::Mat frame(cv::Size(width, height), CV_8UC3, map.data);

        // Display the frame
        cv::imshow("UDP H.264 Stream", frame);
        cv::waitKey(1);

        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
        return GST_FLOW_OK;
    };
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink), &callbacks, NULL, NULL);

    // Start the pipeline
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait for the pipeline to finish
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_EOS | GST_MESSAGE_ERROR);
    if (msg != NULL) 
	{
        gst_message_unref(msg);
    }
    gst_object_unref(bus);

    // Clean up
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
