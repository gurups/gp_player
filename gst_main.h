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
    
    gboolean playing;      /* Are we in the PLAYING state? */
    gboolean terminate;    /* Should we terminate execution? */
    gboolean seek_enabled; /* Is seeking enabled for this media? */
    gboolean seek_done;    /* Have we performed the seek already? */
    gint64 duration;       /* How long does this media last, in nanoseconds */
} gst_main_element;

/* Handler for the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *pad, gst_main_element *data);

/* Forward definition of the message processing function */
static void handle_message (gst_main_element *data, GstMessage *msg);

#endif
