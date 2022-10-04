if HAVE_JSONC

lib_LTLIBRARIES = libv4l2tracer.la
libv4l2tracer_la_SOURCES = libv4l2tracer.cpp trace-gen.cpp trace-info-gen.cpp trace-helper.cpp \
trace.cpp $(top_srcdir)/utils/common/v4l2-info.cpp
libv4l2tracer_la_CPPFLAGS = -I$(top_srcdir)/utils/common $(JSONC_CFLAGS)
libv4l2tracer_la_LDFLAGS = -avoid-version -module -shared -export-dynamic -ldl $(JSONC_LIBS)
libv4l2tracer_la_LIBTOOLFLAGS = --tag=disable-static

bin_PROGRAMS = v4l2-tracer
man_MANS = v4l2-tracer.1

v4l2_tracer_SOURCES = v4l2-tracer.cpp retrace-gen.cpp retrace-helper.cpp retracer.cpp \
$(top_srcdir)/utils/common/v4l2-info.cpp $(top_srcdir)/utils/common/codec-fwht.h
v4l2_tracer_CPPFLAGS = -I$(top_srcdir)/utils/common -DLIBTRACER_PATH=$(libdir) $(JSONC_CFLAGS)
v4l2_tracer_LDFLAGS = -lrt -lpthread $(JSONC_LIBS)

V4L2_TRACER_GEN_PREREQS = $(top_srcdir)/include/linux/v4l2-controls.h \
$(top_srcdir)/include/linux/videodev2.h $(top_srcdir)/include/linux/media.h
V4L2_TRACER_GEN = trace-gen.cpp trace-gen.h trace-info-gen.cpp trace-info-gen.h \
retrace-gen.cpp retrace-gen.h
v4l2-tracer.cpp: $(V4L2_TRACER_GEN)
$(V4L2_TRACER_GEN): gen.intermediate;
.INTERMEDIATE: gen.intermediate
gen.intermediate: v4l2-tracer-gen.pl $(V4L2_TRACER_GEN_PREREQS)
	$(AM_V_GEN) $(top_srcdir)/utils/v4l2-tracer/v4l2-tracer-gen.pl $(V4L2_TRACER_GEN_PREREQS)

clean-local:
	-rm -vf $(V4L2_TRACER_GEN)

EXTRA_DIST = v4l2-tracer.1, v4l2-tracer-gen.pl

endif
