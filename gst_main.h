#ifndef _GST_MAIN_HEADER_
#define _GST_MAIN_HEADER_

typedef enum {
    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE
} TutorialCount;

typedef struct _gst_main_element {
    GstElement *pipeline;
    GstElement *uri_source;
    GstElement *v_source;
    GstElement *v_sink;
    GstElement *a_source;
    GstElement *a_sink;
    GstElement *aconvert;
    GstElement *vconvert;
    GstElement *resample;
    GstBus *bus;
    GstMessage *msg;
    int c_ret;
} gst_main_element;

/* Handler for the pad-added signal */
//static void pad_added_handler (GstElement *src, GstPad *pad, gst_main_element *data);

#endif
