#include "iostream"
#include <gst/gst.h>
#include "gst_main.h"
using namespace std;

int get_source_type(){
    int opt = 1;
    g_print("Guru select source\n");
    g_print("1. Test Pattern\n");
    g_print("2. Uri\n");
    scanf("%d",&opt);
    return opt;
}

/* Handler for the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *pad, gst_main_element *data);

int main(int argc, char *argv[]){
    gst_main_element g_main;
    
    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Build the pipeline */
    g_main.pipeline = gst_pipeline_new("gst_app_pipeline");
    int s_type = get_source_type();
    if(s_type == 1){
        g_main.v_source = gst_element_factory_make("videotestsrc","vsrc");
        g_main.v_sink = gst_element_factory_make("autovideosink","vsink");

        g_main.a_source = gst_element_factory_make("audiotestsrc","asrc");
        g_main.a_sink = gst_element_factory_make("autoaudiosink","asink");

        if(!g_main.pipeline || !g_main.v_source || !g_main.v_sink){
            g_error("GURU: pipeline create failed");

            return -1;
        }

         if(!g_main.a_source || !g_main.a_sink){
            g_error("GURU: pipeline create failed");
            return -1;
        }
        
        gst_bin_add_many(GST_BIN (g_main.pipeline), g_main.v_source, g_main.v_sink, g_main.a_source,  g_main.a_sink, NULL);
        if(gst_element_link(g_main.v_source, g_main.v_sink) != TRUE){
            g_error("GURU:Failed to link elements");
            gst_object_unref(g_main.pipeline);
            return -1;
        }

        if(gst_element_link(g_main.a_source, g_main.a_sink) != TRUE){
            g_error("GURU:Failed to link elements");
            gst_object_unref(g_main.pipeline);
            return -1;
        }
        
        g_object_set(g_main.v_source, "pattern", 0, NULL);
        g_object_set(g_main.a_source, "pattern", 0, NULL);

    }else if(s_type == 2){
        gchar uri_source[512];
        g_print("Enter Uri:");
        scanf("%s",uri_source);
        g_print("Entered Uri:%s\n",uri_source);
        
        //Prepare/make elements
        g_main.uri_source = gst_element_factory_make("uridecodebin","uri_source"); 
        g_main.aconvert = gst_element_factory_make("audioconvert","a_convert");
        g_main.resample = gst_element_factory_make("audioresample","resample");
        g_main.a_sink = gst_element_factory_make("autoaudiosink", "asink");
        
        g_main.vconvert = gst_element_factory_make("videoconvert","v_convert");
        g_main.v_sink = gst_element_factory_make("autovideosink", "vsink");
        
        gst_bin_add_many(GST_BIN(g_main.pipeline), g_main.uri_source, g_main.vconvert, g_main.v_sink, g_main.aconvert, g_main.resample, g_main.a_sink, NULL);
        if(!gst_element_link_many(g_main.aconvert, g_main.resample, g_main.a_sink, NULL)){
            g_error("GURU: link many failed. elements can't be linked");
            gst_object_unref(g_main.pipeline);
            return -1;
        }
        if(!gst_element_link_many(g_main.vconvert, g_main.v_sink, NULL)){
            g_error("GURU: link many failed. elements can't be linked");
            gst_object_unref(g_main.pipeline);
            return -1;
        }
        g_object_set(g_main.uri_source, "uri", uri_source, NULL);
        
        g_signal_connect(g_main.uri_source , "pad-added", G_CALLBACK(pad_added_handler), &g_main);
    }else{
        g_error("GURU: input not supported");
    }
        
    /* Start playing */
    gst_element_set_state (g_main.pipeline, GST_STATE_PLAYING);

    /* Wait until error or EOS */
    g_main.bus = gst_element_get_bus (g_main.pipeline);
    g_main.msg = gst_bus_timed_pop_filtered (g_main.bus, GST_CLOCK_TIME_NONE,
     (GstMessageType) (GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    /* See next tutorial for proper error message handling/parsing */
    if(g_main.msg != NULL){
        GError *err;
        gchar *debug_info;
        switch(GST_MESSAGE_TYPE (g_main.msg)) {
            case GST_MESSAGE_ERROR:
            gst_message_parse_error(g_main.msg, &err, &debug_info);
            
            g_printerr ("GURU: An error occurred! src=%s, err=%s",
                                GST_OBJECT_NAME(g_main.msg->src), err->message);
            g_printerr("GURU: Debug info: %s", 
                        debug_info ? debug_info:"none");
            break;
            case GST_MESSAGE_EOS:
                g_print("GURU: Reached end of stream");
            break;
            default:
                g_error("GURU: unknown error");
            break;
        }
    }
    /* Free resources */
    gst_message_unref (g_main.msg);
    gst_object_unref (g_main.bus);
    gst_element_set_state (g_main.pipeline, GST_STATE_NULL);
    gst_object_unref (g_main.pipeline);
    return 0;
}


/* This function will be called by the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *new_pad, gst_main_element *data) {
  GstPadLinkReturn ret;
  GstCaps *new_pad_caps = NULL;
  GstStructure *new_pad_struct = NULL;
  const gchar *new_pad_type = NULL;

  g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));


  /* Check the new pad's type */
  new_pad_caps = gst_pad_get_current_caps (new_pad);
  new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
  new_pad_type = gst_structure_get_name (new_pad_struct);
  if (g_str_has_prefix (new_pad_type, "video/x-raw")) {
    GstPad *sink_pad = gst_element_get_static_pad (data->vconvert, "sink");
    
    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (sink_pad)) {
      g_print ("We are already linked. Ignoring.\n");
      goto exit;
    }
    /* Attempt the link */
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } else {
      g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }
    /* Unreference the sink pad */
    gst_object_unref (sink_pad);
  }else if (g_str_has_prefix (new_pad_type, "audio/x-raw")){
    GstPad *sink_pad = gst_element_get_static_pad (data->aconvert, "sink");
    
    /* If our converter is already linked, we have nothing to do here */
    if (gst_pad_is_linked (sink_pad)) {
      g_print ("We are already linked. Ignoring.\n");
      goto exit;
    }
    /* Attempt the link */
    ret = gst_pad_link (new_pad, sink_pad);
    if (GST_PAD_LINK_FAILED (ret)) {
      g_print ("Type is '%s' but link failed.\n", new_pad_type);
    } else {
      g_print ("Link succeeded (type '%s').\n", new_pad_type);
    }
    
    /* Unreference the sink pad */
    gst_object_unref (sink_pad);
  }else{
    g_print ("Received unhandled pad\n");
  }

exit:
  /* Unreference the new pad's caps, if we got them */
  if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);

}
