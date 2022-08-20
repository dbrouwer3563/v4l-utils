if HAVE_JSONC

lib_LTLIBRARIES = libtracer.la
include_HEADERS = libtracer.h ../../utils/common/v4l2-info.h ../../utils/common/media-info.h
libtracer_la_SOURCES = libtracer.cpp trace.cpp trace-vp8.cpp trace-helper.cpp \
trace-info.cpp ../../utils/common/v4l2-info.cpp ../../utils/common/media-info.cpp

libtracer_la_CFLAGS = $(JSONC_CFLAGS)
libtracer_la_CPPFLAGS = -I$(top_srcdir)/utils/common $(JSONC_CFLAGS)
libtracer_la_LDFLAGS = -avoid-version -module -shared -export-dynamic -ldl $(JSONC_LIBS)
libtracer_la_LIBTOOLFLAGS = --tag=disable-static

bin_PROGRAMS = tracer retracer

tracer_SOURCES = tracer.cpp
tracer_CPPFLAGS = -I$(top_srcdir)/utils/common $(JSONC_CFLAGS)
tracer_LDFLAGS = -lrt -lpthread $(JSONC_LIBS)

retracer_SOURCES = retracer.cpp retrace-helper.cpp retrace-vp8.cpp trace-info.cpp ../common/v4l2-info.cpp
retracer_CPPFLAGS = -I$(top_srcdir)/utils/common $(JSONC_CFLAGS)
retracer_LDFLAGS = -lrt -lpthread  $(JSONC_LIBS)

endif
