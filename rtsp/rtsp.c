#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define RTSP_SERVER_PORT "8555"
#define RTSP_SERVER_MOUNT_POINT "/test"

static gboolean on_new_sample(GstAppSink *appsink, gpointer user_data)
{
    g_print("New sample received\n");
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    if (sample != NULL)
    {
        g_print("Processing sample...\n");
        // 处理样本的代码...
        g_print("Sample processed\n");
        gst_sample_unref(sample);
    }
    else
    {
        g_print("Failed to pull sample\n");
    }
    return TRUE;
}

int main(int argc, char *argv[])
{
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;
    GstElement *appsink;

    gst_init(&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);

    // 创建一个RTSP服务器
    server = gst_rtsp_server_new();

    // 获取服务器的挂载点
    mounts = gst_rtsp_server_get_mount_points(server);

    factory = gst_rtsp_media_factory_new();
    gst_rtsp_media_factory_set_launch(factory,
                                      "( rtspsrc location=rtsp://192.168.1.40:8554/test.264 latency=0 ! rtph264depay ! h264parse  ! rtph264pay name=pay0 pt=96 )");

    // 将工厂挂载到路径"/test"上
    gst_rtsp_mount_points_add_factory(mounts, RTSP_SERVER_MOUNT_POINT, factory);

    // 放弃挂载点的引用，服务器将拥有它
    g_object_unref(mounts);

    gst_rtsp_server_set_service(server, RTSP_SERVER_PORT); // 将端口号更改为所需的值

    // 启动服务器
    gst_rtsp_server_attach(server, NULL);

    // 创建一个appsink来获取每个数据帧
    appsink = gst_element_factory_make("appsink", "sink");
    g_object_set(appsink, "emit-signals", TRUE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(on_new_sample), NULL);

    // 运行主循环
    g_main_loop_run(loop);

    // 清理
    g_main_loop_unref(loop);
    g_object_unref(server);

    return 0;
}
