#ifndef UCXX_UCX_RAII_HPP
#define UCXX_UCX_RAII_HPP

#include <ucp/api/ucp.h>

#include <memory>

namespace ucxx {

  namespace detail {
    struct ucp_context_h_deleter {
      using pointer = ucp_context_h;
      void operator()(pointer ptr) { ucp_cleanup(ptr); }
    };

    struct ucp_worker_h_deleter {
      using pointer = ucp_worker_h;
      void operator()(pointer ptr) { ucp_worker_destroy(ptr); }
    };

    struct ucp_listener_h_deleter {
      using pointer = ucp_listener_h;
      void operator()(pointer ptr) { ucp_listener_destroy(ptr); }
    };

  }  // namespace detail

  using unique_ucp_context_h = std::unique_ptr<void, detail::ucp_context_h_deleter>;
  using unique_ucp_worker_h = std::unique_ptr<void, detail::ucp_worker_h_deleter>;
  using unique_ucp_listener_h = std::unique_ptr<void, detail::ucp_listener_h_deleter>;

}  // namespace ucxx

#endif
