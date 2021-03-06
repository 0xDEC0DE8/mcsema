#
# Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

# import os ;
# import feature ;
# 
# lib ssl ;
# lib crypto ;
# 
# if [ os.name ] = SOLARIS
# {
#   lib socket ;
#   lib nsl ;
# }
# else if [ os.name ] = NT
# {
#   lib ws2_32 ;
#   lib mswsock ;
# }
# else if [ os.name ] = HPUX
# {
#   lib ipv6 ;
# }

#local USE_SELECT =
#  <define>BOOST_ASIO_DISABLE_EPOLL
#  <define>BOOST_ASIO_DISABLE_KQUEUE
#  <define>BOOST_ASIO_DISABLE_IOCP
#  ;

#project
#  : requirements
#    <library>/boost/date_time//boost_date_time
#    <library>/boost/thread//boost_thread
#    <define>BOOST_ALL_NO_LIB=1
#    <threading>multi
#    <os>SOLARIS:<library>socket
#    <os>SOLARIS:<library>nsl
#    <os>NT:<define>_WIN32_WINNT=0x0501
#    <os>NT,<toolset>gcc:<library>ws2_32
#    <os>NT,<toolset>gcc:<library>mswsock
#    <os>NT,<toolset>gcc-cygwin:<define>__USE_W32_SOCKETS
#    <os>HPUX,<toolset>gcc:<define>_XOPEN_SOURCE_EXTENDED
#    <os>HPUX:<library>ipv6
#  ;

foreach (test 
    basic_context
    context_base
    context_service
    context
    stream_base
    stream_service
    stream)
  boost_test_compile(${test})
  boost_test_compile(${test}_select ${test}.cpp COMPILE_FLAGS ${USE_SELECT})
endforeach()


#   [ compile basic_context.cpp ]
#   [ compile basic_context.cpp : $(USE_SELECT) : basic_context_select ]
#   [ compile context_base.cpp ]
#   [ compile context_base.cpp : $(USE_SELECT) : context_base_select ]
#   [ compile context_service.cpp ]
#   [ compile context_service.cpp : $(USE_SELECT) : context_service_select ]
#   [ compile context.cpp ]
#   [ compile context.cpp : $(USE_SELECT) : context_select ]
#   [ compile stream_base.cpp ]
#   [ compile stream_base.cpp : $(USE_SELECT) : stream_base_select ]
#   [ compile stream_service.cpp ]
#   [ compile stream_service.cpp : $(USE_SELECT) : stream_service_select ]
#   [ compile stream.cpp ]
#   [ compile stream.cpp : $(USE_SELECT) : stream_select ]
#   ;
