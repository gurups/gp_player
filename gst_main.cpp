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

int main(int argc, char *argv[]){
    gst_main_element g_main;
    
    g_main.playing = FALSE;
    g_main.terminate = FALSE;
    g_main.seek_enabled = FALSE;
    g_main.seek_done = FALSE;
    g_main.duration = GST_CLOCK_TIME_NONE;

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
    do{
        g_main.msg = gst_bus_timed_pop_filtered (g_main.bus, 100 * GST_MSECOND,
         (GstMessageType)(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_DURATION));
        /* See next tutorial for proper error message handling/parsing */
        if(g_main.msg != NULL){
            handle_message (&g_main, g_main.msg);
        }else {
          /* We got no message, this means the timeout expired */
          if (g_main.playing) {
            gint64 current = -1;

            /* Query the current position of the stream */
            if (!gst_element_query_position (g_main.pipeline, GST_FORMAT_TIME, &current)) {
              g_printerr ("Could not query current position.\n");
            }

            /* If we didn't know it yet, query the stream duration */
            if (!GST_CLOCK_TIME_IS_VALID (g_main.duration)) {
              if (!gst_element_query_duration (g_main.pipeline, GST_FORMAT_TIME, &g_main.duration)) {
                g_printerr ("Could not query current duration.\n");
              }
            }

            /* Print current position and total duration */
            g_print ("Position %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\r",
                GST_TIME_ARGS (current), GST_TIME_ARGS (g_main.duration));

            /* If seeking is enabled, we have not done it yet, and the time is right, seek */
            if (g_main.seek_enabled && !g_main.seek_done && current > 5 * GST_SECOND) {
              g_print ("\nReached 10s, performing seek...\n");
              gst_element_seek_simple (g_main.pipeline, GST_FORMAT_TIME,
                  (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT), 15 * GST_SECOND);
              g_main.seek_done = TRUE;
            }
          }
        }
    }while (!g_main.terminate);
    /* Free resources */

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

static void handle_message (gst_main_element *data, GstMessage *msg) {
  GError *err;
  gchar *debug_info;

  switch (GST_MESSAGE_TYPE (msg)) {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error (msg, &err, &debug_info);
      g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
      g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error (&err);
      g_free (debug_info);
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_EOS:
      g_print ("\nEnd-Of-Stream reached.\n");
      data->terminate = TRUE;
      break;
    case GST_MESSAGE_DURATION:
      /* The duration has changed, mark the current one as invalid */
      data->duration = GST_CLOCK_TIME_NONE;
      break;
    case GST_MESSAGE_STATE_CHANGED: {
      GstState old_state, new_state, pending_state;
      gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
      if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data->pipeline)) {
        g_print ("Pipeline state changed from %s to %s:\n",
            gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));

        /* Remember whether we are in the PLAYING state or not */
        data->playing = (new_state == GST_STATE_PLAYING);

        if (data->playing) {
          /* We just moved to PLAYING. Check if seeking is possible */
          GstQuery *query;
          gint64 start, end;
          query = gst_query_new_seeking (GST_FORMAT_TIME);
          if (gst_element_query (data->pipeline, query)) {
            gst_query_parse_seeking (query, NULL, &data->seek_enabled, &start, &end);
            if (data->seek_enabled) {
              g_print ("Seeking is ENABLED from %" GST_TIME_FORMAT " to %" GST_TIME_FORMAT "\n",
                  GST_TIME_ARGS (start), GST_TIME_ARGS (end));
            } else {
              g_print ("Seeking is DISABLED for this stream.\n");
            }
          }
          else {
            g_printerr ("Seeking query failed.");
          }
          gst_query_unref (query);
        }
      }
    } break;
    default:
      /* We should not reach here */
      g_printerr ("Unexpected message received.\n");
      break;
  }
  gst_message_unref (msg);
}

