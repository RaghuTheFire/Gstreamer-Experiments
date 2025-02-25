/*********************************************************************************************************************************
This implementation creates a RTSP server that can re-stream an existing RTSP camera feed using GStreamer's RTSP server module.

Key components of the code:
    Uses GstRTSPServer to create a RTSP server
    Creates a media factory that defines the GStreamer pipeline
    Handles proper initialization and cleanup of GStreamer resources
    Implements a clean C++ class interface around the C-based GStreamer API

Build requirements:
    sudo apt-get install libgstrtspserver-1.0-dev libgstreamer1.0-dev

Compilation command:
    g++ RestreamRTSPCameraFeed.cpp -o RestreamRTSPCameraFeed `pkg-config --cflags --libs gstreamer-1.0 gstreamer-rtsp-server-1.0`
The pipeline used in this implementation:
    rtspsrc: Sources the input RTSP stream
    rtph264depay: Extracts H264 video from RTP packets
    h264parse: Parses H264 video
    rtph264pay: Repackages H264 video into RTP packets
Usage example:
./RestreamRTSPCameraFeed rtsp://original-camera-url:554/stream
Then connect to the re-streamed feed at: rtsp://127.0.0.1:8554/test
Note: This implementation assumes H264 video codec. For different codecs, you'll need to modify the pipeline description accordingly.
*********************************************************************************************************************************/

#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <memory>
#include <string>

class RTSPRestreamer 
{
public:
    RTSPRestreamer(const std::string& input_url, int port = 8554, 
                   const std::string& mount_point = "/test") 
        : input_url_(input_url), port_(port), mount_point_(mount_point) 
        {
        }

    bool initialize() 
    {
        gst_init(nullptr, nullptr);
        
        server_ = gst_rtsp_server_new();
        if (!server_) 
        {
            std::cerr << "Failed to create RTSP server" << std::endl;
            return false;
        }

        gst_rtsp_server_set_service(server_, std::to_string(port_).c_str());

        mounts_ = gst_rtsp_server_get_mount_points(server_);
        if (!mounts_) 
        {
            std::cerr << "Failed to get mount points" << std::endl;
            return false;
        }

        factory_ = gst_rtsp_media_factory_new();
        if (!factory_) 
        {
            std::cerr << "Failed to create media factory" << std::endl;
            return false;
        }

        std::string pipeline = create_pipeline_description();
        gst_rtsp_media_factory_set_launch(factory_, pipeline.c_str());
        gst_rtsp_media_factory_set_shared(factory_, TRUE);

        gst_rtsp_mount_points_add_factory(mounts_, mount_point_.c_str(), factory_);
        g_object_unref(mounts_);

        return true;
    }

    void run() 
    {
        gst_rtsp_server_attach(server_, nullptr);
        g_main_loop_run(g_main_loop_new(nullptr, FALSE));
    }

    ~RTSPRestreamer() 
    {
        if (server_) 
        {
            g_object_unref(server_);
        }
    }

private:
    std::string create_pipeline_description() 
    {
        return "( rtspsrc location=" + input_url_ + 
               " ! rtph264depay ! h264parse ! rtph264pay name=pay0 pt=96 )";
    }

    std::string input_url_;
    int port_;
    std::string mount_point_;
    GstRTSPServer* server_ = nullptr;
    GstRTSPMountPoints* mounts_ = nullptr;
    GstRTSPMediaFactory* factory_ = nullptr;
};

int main(int argc, char* argv[]) 
{
    if (argc != 2) 
    {
        std::cerr << "Usage: " << argv[0] << " <rtsp-url>" << std::endl;
        return 1;
    }

    RTSPRestreamer restreamer(argv[1]);
    
    if (!restreamer.initialize()) 
    {
        std::cerr << "Failed to initialize RTSP restreamer" << std::endl;
        return 1;
    }

    std::cout << "RTSP server running at rtsp://127.0.0.1:8554/test" << std::endl;
    restreamer.run();

    return 0;
}



