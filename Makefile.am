AUTOMAKE_OPTIONS = foreign

ROOTCINT = rootcint

LDFLAGS = -L$(libdir) -L$(ONLINE_MAIN)/lib

INCLUDES = -I$(includedir) -I$(ROOTSYS)/include -I$(ONLINE_MAIN)/include -I/opt/phenix/include


if DATABASE

databaselibs = -L/opt/phenix/lib -lodbc++

include_HEADERS = \
  sharedmemory.h \
  et_control.h \
  et_structure.h \
  et_PGConnection.h \
  oBufferThreaded.h

else

databaselibs = 

include_HEADERS = \
  sharedmemory.h \
  et_structure.h \
  et_control.h \
  oBufferThreaded.h

endif



lib_LTLIBRARIES = \
  libloggerUtils.la \
  libevent_selector.la \
  libGL1EventTagger.la \
  libOdd_Even_EventTagger.la \
  libEventFilter.la

bin_PROGRAMS = \
  aml  \
  et_start_phenix  \
  et_server  \
  oncs_et_server  \
  et_server_display  \
  etlogger_display  \
  aml_display  \
  aml_memclear  \
  etlogger  \
  aml  \
  aml_nextfile  \
  mb_per_second \
  et_putdata_direct \
  et_putdata_net \
  et_send_data \
  file_to_aml \
  et_send_buffers \
  et_online_monitor \
  et_remove_station \
  evfilter \
  readsharedmem

if DATABASE
bin_PROGRAMS += pgtest

pgtest_SOURCES = pgtest.cc
pgtest_LDFLAGS = $(ADDLDFLAGS)
pgtest_LDADD =  libloggerUtils.la -lNoRootEvent \
  $(databaselibs)
endif

evfilter_SOURCES = evfilter.cc
evfilter_LDFLAGS = $(ADDLDFLAGS)
evfilter_LDADD = \
  libEventFilter.la \
  -lNoRootEvent -lmessage -lz

et_start_phenix_SOURCES = et_start_phenix.cc
et_start_phenix_LDFLAGS = $(ADDLDFLAGS)
et_start_phenix_LDADD = \
  -let 

et_online_monitor_SOURCES = et_online_monitor.cc
et_online_monitor_LDFLAGS = $(ADDLDFLAGS)
et_online_monitor_LDADD = \
  -let 

et_remove_station_SOURCES = et_remove_station.cc
et_remove_station_LDFLAGS = $(ADDLDFLAGS)
et_remove_station_LDADD = \
  -let 


etlogger_SOURCES = \
  etlogger.cc \
  oBufferThreaded.cc
etlogger_LDFLAGS = $(ADDLDFLAGS)   libloggerUtils.la \
  -lNoRootEvent \
  -let \
  -lmessage \
  $(databaselibs) -lz 

aml_SOURCES = \
  aml.cc \
  oBufferThreaded.cc


aml_LDADD = \
  libloggerUtils.la \
  -lNoRootEvent \
  -let \
  $(databaselibs) -lz \
  -lrt

aml3_SOURCES = \
  aml3.cc \
  oBufferThreaded.cc


aml3_LDADD = \
  libloggerUtils.la \
  -lNoRootEvent \
  -let \
  -lmessage \
  $(databaselibs) -lz \
  -lrt


aml_memclear_SOURCES = \
  aml_memclear.cc


aml_display_SOURCES = \
  aml_display.cc \
  oBufferThreaded.cc

aml_LDFLAGS = $(ADDLDFLAGS)

aml_memclear_LDADD = \
  libloggerUtils.la \
  $(databaselibs)

aml_display_LDADD = \
  libloggerUtils.la \
  -lNoRootEvent \
  -let \
  -lmessage \
  $(databaselibs) -lz 

readsharedmem_SOURCES = \
  readsharedmem.cc
readsharedmem_LDFLAGS = $(ADDLDFLAGS)
readsharedmem_LDADD =  \
  libloggerUtils.la  -lNoRootEvent $(databaselibs)

aml_nextfile_SOURCES = aml_nextfile.cc
aml_nextfile_LDFLAGS = $(ADDLDFLAGS)
aml_nextfile_LDADD =  \
  libloggerUtils.la  -lNoRootEvent $(databaselibs)

et_server_SOURCES = et_server.cc
et_server_LDFLAGS = -Wl,-export-dynamic
et_server_LDADD = \
  libloggerUtils.la \
  -let \
  -lNoRootEvent \
  -lmessage \
  -lz \
  $(databaselibs) \
  -lrt

oncs_et_server_SOURCES = oncs_et_server.cc
oncs_et_server_LDFLAGS = -Wl,-export-dynamic
oncs_et_server_LDADD = \
  libloggerUtils.la \
  -let \
  -lNoRootEvent \
  -lmessage \
  -lz \
  $(databaselibs) \
  -lrt


et_server_drop_SOURCES = et_server_drop.cc
et_server_drop_LDADD = \
  libloggerUtils.la \
  -let \
  -lNoRootEvent \
  -lmessage \
  -lz \
  $(databaselibs) \
  -lrt

etlogger_display_SOURCES = etlogger_display.cc
etlogger_display_LDADD =  \
  libloggerUtils.la \
  -let  -lNoRootEvent \
  $(databaselibs) \
  -lrt

et_server_display_SOURCES = et_server_display.cc
et_server_display_LDADD = \
  libloggerUtils.la \
  -let -lNoRootEvent \
  $(databaselibs) \
  -lrt


mb_per_second_SOURCES = mb_per_second.cc
mb_per_second_LDADD = \
  libloggerUtils.la \
  -let -lNoRootEvent \
  $(databaselibs) \
  -lrt 

file_to_aml_SOURCES = file_to_aml.cc
file_to_aml_LDADD = \
  -lNoRootEvent \
  -lz 


et_putdata_direct_SOURCES = et_putdata_direct.cc
et_putdata_direct_LDADD = \
  -let \
  -lNoRootEvent \
  -lmessage \
  -lz \
  -lrt

et_putdata_net_SOURCES = et_putdata_net.cc
et_putdata_net_LDADD = \
  -let \
  -lNoRootEvent \
  -lmessage \
  -lz \
  -lrt

et_send_data_SOURCES = et_send_data.cc
et_send_data_LDADD = \
  -let \
  -lNoRootEvent \
  -lmessage \
  -lz \
  -lrt

et_send_buffers_SOURCES = et_send_buffers.cc
et_send_buffers_LDADD = \
  -let \
  -lNoRootEvent \
  -lmessage \
  -lz \
  -lrt


if DATABASE
libloggerUtils_la_SOURCES = \
  sharedmemory.cc \
  et_PGConnection.cc \
  et_control.cc
else
libloggerUtils_la_SOURCES = \
  sharedmemory.cc \
  et_control.cc
endif



libloggerUtils_la_LIBADD = -lNoRootEvent


libevent_selector_la_SOURCES = \
  event_selector.cc

libGL1EventTagger_la_SOURCES = \
  gl1_EventTagger.cc

libOdd_Even_EventTagger_la_SOURCES = \
  odd_even_EventTagger.cc

libEventFilter_la_SOURCES = \
  AmlOutStream.C \
  EventFilter.C \
  EvFilterConfig.C \
  FileOutStream.C \
  FilterBase.C \
  TriggerFilter.C

