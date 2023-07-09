//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 2012-2023 HALX99 (halx99 at live dot com)

#pragma once

#include "config.hpp"

#if YASIO__HAS_KQUEUE && defined(YASIO_ENABLE_HPERF_IO)
#  include "impl/kqueue_io_watcher.hpp"
#elif YASIO__HAS_EPOLL && defined(YASIO_ENABLE_HPERF_IO)
#  include "impl/epoll_io_watcher.hpp"
#elif YASIO__HAS_EVPORT && defined(YASIO_ENABLE_HPERF_IO)
#  include "impl/evport_io_watcher.hpp"
#elif !defined(YASIO_DISABLE_POLL)
#  include "impl/poll_io_watcher.hpp"
#else
#  include "impl/select_io_watcher.hpp"
#endif

namespace yasio
{
namespace inet
{
#if defined(YASIO__KQUEUE_IO_WATCHER_HPP)
using io_watcher = kqueue_io_watcher;
#elif defined(YASIO__EPOLL_IO_WATCHER_HPP)
using io_watcher = epoll_io_watcher;
#elif defined(YASIO__EVPORT_IO_WATCHER_HPP)
using io_watcher = evport_io_watcher;
#elif !defined(YASIO_DISABLE_POLL)
using io_watcher = poll_io_watcher;
#else
using io_watcher = select_io_watcher;
#endif
} // namespace inet
} // namespace yasio
