#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <opencv2/opencv.hpp>

static GstPipeline * pipeline;
static GstAppSink * appsink;
static cv::Mat frame;

static gboolean bus_call(GstBus * bus, GstMessage * msg, gpointer data) 
{
  GMainLoop * loop = (GMainLoop * ) data;

  switch (GST_MESSAGE_TYPE(msg)) 
  {
  case GST_MESSAGE_EOS:
    g_print("End of stream\n");
    g_main_loop_quit(loop);
    break;
  case GST_MESSAGE_ERROR: 
  {
    gchar * debug;
    GError * error;

    gst_message_parse_error(msg, & error, & debug);
    g_free(debug);

    g_printerr("Error: %s\n", error -> message);
    g_error_free(error);

    g_main_loop_quit(loop);
    break;
  }
  default:
    break;
  }

  return TRUE;
}

static GstFlowReturn new_sample(GstAppSink * sink, gpointer user_data) 
{
  GstSample * sample = gst_app_sink_pull_sample(sink);
  GstCaps * caps = gst_sample_get_caps(sample);
  GstStructure * structure = gst_caps_get_structure(caps, 0);
  gint width, height;

  gst_structure_get_int(structure, "width", & width);
  gst_structure_get_int(structure, "height", & height);

  GstMapInfo map;
  gst_sample_map(sample, & map, GST_MAP_READ);

  frame = cv::Mat(cv::Size(width, height), CV_8UC3, map.data, cv::Mat::AUTO_STEP);

  cv::imshow("Video", frame);
  cv::waitKey(1);

  gst_sample_unmap(sample);
  gst_sample_unref(sample);

  return GST_FLOW_OK;
}

int main(int argc, char * argv[]) 
{
  gst_init( & argc, & argv);

  GMainLoop * loop = g_main_loop_new(NULL, FALSE);

  GstElement * source = gst_element_factory_make("rtspsrc", "source");
  g_object_set(G_OBJECT(source), "location", "rtmp://your-rtmp-url", NULL);

  GstElement * depay = gst_element_factory_make("rtpmp4vdepay", "depay");
  GstElement * decode = gst_element_factory_make("avdec_h264", "decode");
  GstElement * convert = gst_element_factory_make("videoconvert", "convert");
  appsink = GST_APP_SINK(gst_element_factory_make("appsink", "sink"));

  pipeline = GST_PIPELINE(gst_pipeline_new("pipeline"));

  gst_bin_add_many(GST_BIN(pipeline), source, depay, decode, convert, appsink, NULL);
  gst_element_link_many(source, depay, decode, convert, appsink, NULL);

  GstAppSinkCallbacks callbacks = {
    NULL,
    NULL,
    new_sample,
    NULL
  };
  gst_app_sink_set_callbacks(appsink, & callbacks, NULL, NULL);

  GstBus * bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);

  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);

  g_main_loop_run(loop);

  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
  gst_object_unref(GST_OBJECT(pipeline));

  return 0;
}
