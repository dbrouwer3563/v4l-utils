if HAVE_JSONC

lib_LTLIBRARIES = libtracer.la
libtracer_la_SOURCES = libtracer.cpp trace.cpp trace-vp8.cpp trace-h264.cpp trace-fwht.cpp \
trace-helper.cpp trace-info.cpp $(top_srcdir)/utils/common/v4l2-info.cpp \
$(top_srcdir)/utils/common/media-info.cpp

libtracer_la_CPPFLAGS = -I$(top_srcdir)/utils/common $(JSONC_CFLAGS)
libtracer_la_LDFLAGS = -avoid-version -module -shared -export-dynamic -ldl $(JSONC_LIBS)
libtracer_la_LIBTOOLFLAGS = --tag=disable-static

bin_PROGRAMS = v4l2-tracer
man_MANS = v4l2-tracer.1

v4l2_tracer_SOURCES = v4l2-tracer.cpp retracer.cpp trace-info.cpp retrace-helper.cpp retrace-vp8.cpp \
retrace-h264.cpp retrace-fwht.cpp  $(top_srcdir)/utils/common/v4l2-info.cpp

v4l2_tracer_CPPFLAGS = -I$(top_srcdir)/utils/common -DLIBTRACER_PATH=$(libdir) $(JSONC_CFLAGS)
v4l2_tracer_LDFLAGS = -lrt -lpthread $(JSONC_LIBS)

EXTRA_DIST = v4l2-tracer.1

endif
