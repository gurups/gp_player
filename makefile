#Make file for gst app
export GST_APP_HOME: = $HOME/Work/Guru_repo/gst_app/
CC = g++
GST_APP_INCLUDE:=include
GST_APP_SRC_FILES:=gst_main.cpp
# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
CFLAGS  = -g -Wall -I$(GST_APP_INCLUDE)
LDFLAGS = -lpthread
# The build target 
TARGET = gst_app 
 
 $(TARGET): $(GST_APP_SRC_FILES)
	$(CC) $(CFLAGS) $? $(LDFLAGS) -o $@ `pkg-config --cflags --libs gstreamer-1.0`
  all: $(TARGET)
 
  #$(TARGET): $(TARGET).cpp
  #this part for gstreamer code compilation
#	$(CC) $(CFLAGS) -o $(TARGET) $(SRC_FILES_LIST) `pkg-config --cflags --libs gstreamer-1.0`
 
  clean:
	rm -f $(TARGET)

