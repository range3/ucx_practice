#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <spdlog/cfg/env.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ucp/api/ucp.h>

#include <cxxopts.hpp>
#include <fstream>
#include <iostream>
#include <string>

enum class app_state {
  INIT,
  CONNECTED,
  SENDING,
  SENT,
  DISCONNECTING,
  DISCONNECTED,
  ERROR,
};

using app_context_t = struct app_context {
  ucp_context_h ctx;
  ucp_worker_h worker;
  ucp_listener_h listener;
  ucp_ep_h ep;
  app_state state = app_state::INIT;
};

bool is_server(const cxxopts::ParseResult& parsed) { return parsed.count("server") != 0U; }

auto close_ep(ucp_worker_h* worker, ucp_ep_h* ep) -> ucs_status_t {
  ucp_request_param_t params = {
      .op_attr_mask = UCP_OP_ATTR_FIELD_FLAGS,
      .flags = UCP_EP_CLOSE_MODE_FLUSH,
  };
  ucs_status_t status;
  ucs_status_ptr_t close;
  close = ucp_ep_close_nbx(*ep, &params);
  if (close == nullptr) {
    return UCS_OK;
  }
  if (UCS_PTR_IS_ERR(close)) {
    spdlog::error("ucp_ep_close_nbx: {}", ucs_status_string(UCS_PTR_STATUS(close)));
    return UCS_PTR_STATUS(close);
  }
  status = ucp_request_check_status(close);
  if (status == UCS_INPROGRESS) {
    do {
      ucp_worker_progress(*worker);
      status = UCS_PTR_STATUS(close);
    } while (status == UCS_INPROGRESS);
    if (status != UCS_OK) {
      spdlog::error("ucp_ep_close_nbx: {}", ucs_status_string(status));
      return status;
    }
    return UCS_OK;
  }
  spdlog::error("close_ep: unexpectecd status {}", ucs_status_string(status));
  return UCS_OK;
}

static void err_cb(void* arg, ucp_ep_h ep, ucs_status_t status) {
  auto* app = reinterpret_cast<app_context_t*>(arg);
  spdlog::error("err_cb: {}", ucs_status_string(status));
  if (app->state != app_state::INIT) {
    close_ep(&app->worker, &ep);
    app->state = app_state::DISCONNECTING;
  }
}

void server_conn_handler_cb(ucp_conn_request_h conn_request, void* arg) {
  auto* app = reinterpret_cast<app_context_t*>(arg);
  ucs_status_t status;
  ucp_conn_request_attr_t attr = {
      .field_mask = UCP_CONN_REQUEST_ATTR_FIELD_CLIENT_ID,
  };
  status = ucp_conn_request_query(conn_request, &attr);
  if (status != UCS_OK) {
    spdlog::error("ucp_conn_request_query: {}", ucs_status_string(status));
    return;
  }
  spdlog::info("ucp_conn_request_query: client_id={}", attr.client_id);
  ucp_ep_params_t params = {
    .field_mask = UCP_EP_PARAM_FIELD_CONN_REQUEST |
                  UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE |
                  UCP_EP_PARAM_FIELD_ERR_HANDLER,
    .err_mode = UCP_ERR_HANDLING_MODE_PEER,
    .err_handler = {
        .cb = err_cb,
        .arg = app->ctx,
    },
    .conn_request = conn_request,
  };
  status = ucp_ep_create(app->worker, &params, &app->ep);
  if (status != UCS_OK) {
    spdlog::error("ucp_ep_create: {}", ucs_status_string(status));
  }
  app->state = app_state::CONNECTED;
}

sig_atomic_t shutdown_desired;
static void signal_handler(int sig, siginfo_t* info, void* ucontext) {
  spdlog::info("signal_handler is called with sig={}", sig);
  if (sig == SIGINT) {
    shutdown_desired = 1;
  }
}

auto recv_cb(void* arg, const void* header, size_t header_length, void* data, size_t length,
             const ucp_am_recv_param_t* param) -> ucs_status_t {
  assert(!(param->recv_attr & UCP_AM_RECV_ATTR_FLAG_RNDV));
  assert(!(param->recv_attr & UCP_AM_RECV_ATTR_FLAG_DATA));

  spdlog::debug("recb_cb header_length={}", header_length);

  size_t iov_count = *(size_t*)header;
  spdlog::info("recv_cb iov_count={}", iov_count);
  auto* iov_len = (size_t*)UCS_PTR_BYTE_OFFSET(header, sizeof(size_t));

  size_t offset = 0;
  for (size_t idx = 0; idx < iov_count; idx++) {
    spdlog::info("recv_cb item: {}", ((char*)data) + offset);
    offset += iov_len[idx];
  }
  return UCS_OK;
}

void send_cb(void* request, ucs_status_t status, void* user_data) {
  spdlog::info("send_cb: {}", ucs_status_string(status));
  ucp_request_free(request);
}

  std::array<char, sizeof(size_t) * 3> header;
  std::array<ucp_dt_iov_t, 2> iov;
auto send_msg_nbx(app_context_t* app) -> ucs_status_ptr_t {
  ucp_request_param_t rparam = {
      .op_attr_mask = UCP_OP_ATTR_FIELD_DATATYPE
        | UCP_OP_ATTR_FIELD_CALLBACK
        | UCP_OP_ATTR_FIELD_FLAGS
        | UCP_OP_ATTR_FIELD_USER_DATA,
      .flags = UCP_AM_SEND_FLAG_EAGER,
      .cb = {
          .send = send_cb,
      },
      .datatype = UCP_DATATYPE_IOV,
      .user_data = &app,
  };

  const size_t iov_cnt = 2;
  iov[0].buffer = (void*)"Hello,";
  iov[0].length = sizeof("Hello,");
  iov[1].buffer = (void*)"World!";
  iov[1].length = sizeof("World!");
  {
    // headerのシリアライズ。少し大変
    size_t offset = 0;
    // iov_cnt
    *((size_t*)UCS_PTR_BYTE_OFFSET(header.data(), offset)) = iov_cnt;
    offset += sizeof(size_t);
    // "Hello,"
    *((size_t*)UCS_PTR_BYTE_OFFSET(header.data(), offset)) = iov[0].length;
    offset += sizeof(size_t);
    // "World!"
    *((size_t*)UCS_PTR_BYTE_OFFSET(header.data(), offset)) = iov[1].length;
    // offset += sizeof(size_t);
  }

  return ucp_am_send_nbx(app->ep, 0, header.data(), header.size(), iov.data(), iov.size(), &rparam);
}

auto main(int argc, char const* argv[]) -> int {
  spdlog::cfg::load_env_levels();
  cxxopts::Options options("listener", "ucp_listener_h practice");
  // clang-format off
  options.add_options()
    ("h,help", "Print usage")
    ("a,addr", "Address", cxxopts::value<std::string>()->default_value("0.0.0.0"))
    ("p,port", "Port", cxxopts::value<uint16_t>()->default_value("0"))
    ("S,server", "Server")
  ;
  // clang-format on

  auto parsed = options.parse(argc, argv);
  if (parsed.count("help") != 0U) {
    std::cout << options.help() << std::endl;
    return 0;
  }

  struct sigaction sa_sigint = {
      .sa_flags = SA_SIGINFO,
  };
  sa_sigint.sa_sigaction = signal_handler;

  shutdown_desired = 0;

  if (sigaction(SIGINT, &sa_sigint, nullptr) < 0) {
    spdlog::error("sigaction");
    return -1;
  }

  ucs_status_t status;
  app_context_t app;
  ucp_params_t params = {
      .field_mask = UCP_PARAM_FIELD_FEATURES,
      .features = UCP_FEATURE_AM, //| UCP_FEATURE_RMA | UCP_FEATURE_AMO64,
  };
  spdlog::info("ucp init");
  status = ucp_init(&params, nullptr, &app.ctx);
  if (status != UCS_OK) {
    spdlog::error("failed to ucp_init: {}", ucs_status_string(status));
    return -1;
  }

  ucp_worker_params_t wparams = {
      .field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE,
      .thread_mode = UCS_THREAD_MODE_SINGLE,
  };
  status = ucp_worker_create(app.ctx, &wparams, &app.worker);
  if (status != UCS_OK) {
    spdlog::error("failed to ucp_worker_create: {}", ucs_status_string(status));
    goto clean_ctx;
  }

  // set Active message recv handler
  {
    ucp_am_handler_param_t amparams = {
        .field_mask = UCP_AM_HANDLER_PARAM_FIELD_ID | UCP_AM_HANDLER_PARAM_FIELD_ARG
                      | UCP_AM_HANDLER_PARAM_FIELD_CB,
        .id = 0,
        .cb = recv_cb,
        .arg = &app,
    };
    status = ucp_worker_set_am_recv_handler(app.worker, &amparams);
    if (status != UCS_OK) {
      spdlog::error("ucp_worker_set_am_recv_handler: {}", ucs_status_string(status));
      goto clean_worker;
    }
  }

  struct addrinfo* conn_addr;
  if (parsed.count("server") != 0U) {
    spdlog::info("server listen");
    struct sockaddr_in listen_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(parsed["port"].as<uint16_t>()),
      .sin_addr = {
        .s_addr = INADDR_ANY,
      },
    };
    ucp_listener_params_t listener_params
        = {.field_mask = UCP_LISTENER_PARAM_FIELD_SOCK_ADDR | UCP_LISTENER_PARAM_FIELD_CONN_HANDLER,
           .sockaddr = {
               .addr = reinterpret_cast<const struct sockaddr*>(&listen_addr),
               .addrlen = sizeof(listen_addr),
           },
           .conn_handler = {
            .cb = server_conn_handler_cb,
            .arg = &app,
           },
           };
    status = ucp_listener_create(app.worker, &listener_params, &app.listener);
    if (status != UCS_OK) {
      spdlog::error("failed to ucp_listener_create: {}", ucs_status_string(status));
      goto clean_worker;
    }
    ucp_listener_attr_t attr = {
        .field_mask = UCP_LISTENER_ATTR_FIELD_SOCKADDR,
    };
    status = ucp_listener_query(app.listener, &attr);
    if (status != UCS_OK) {
      goto clean_listener;
    }
    std::array<char, 16> addr_str_buf;
    memcpy(&listen_addr, &attr.sockaddr, sizeof(struct sockaddr_in));
    spdlog::info(
        "server listen on {}:{}",
        inet_ntop(AF_INET, &listen_addr.sin_addr.s_addr, addr_str_buf.data(), addr_str_buf.size()),
        ntohs(listen_addr.sin_port));
  } else {
    // client
    int s = getaddrinfo(parsed["addr"].as<std::string>().c_str(),
                        std::to_string(parsed["port"].as<uint16_t>()).c_str(), nullptr, &conn_addr);
    if (s != 0) {
      spdlog::error("getaddrinfo: {}", gai_strerror(s));
      goto clean_worker;
    }

    ucp_ep_params_t ep_params = {
      .field_mask =
        UCP_EP_PARAM_FIELD_SOCK_ADDR
        | UCP_EP_PARAM_FIELD_FLAGS
        | UCP_EP_PARAM_FIELD_ERR_HANDLING_MODE
        | UCP_EP_PARAM_FIELD_ERR_HANDLER
        ,
      .err_mode = UCP_ERR_HANDLING_MODE_PEER,
      .err_handler = {
          .cb = err_cb,
          .arg = &app,
        },
      .flags =
        UCP_EP_PARAMS_FLAGS_CLIENT_SERVER | UCP_EP_PARAMS_FLAGS_SEND_CLIENT_ID,
      .sockaddr = {
          .addr = conn_addr->ai_addr,
          .addrlen = sizeof(struct sockaddr),
        },
    };
    status = ucp_ep_create(app.worker, &ep_params, &app.ep);
    if (status != UCS_OK) {
      spdlog::error("ucp_ep_create: {}", ucs_status_string(status));
      goto clean_conn_addr;
    }
    ucp_ep_print_info(app.ep, stdout);
    app.state = app_state::CONNECTED;
  }

  ucs_status_ptr_t nbx_send;
  while (app.state != app_state::ERROR && shutdown_desired == 0) {
    switch (app.state) {
      case app_state::INIT:
        break;
      case app_state::CONNECTED:
        if (!is_server(parsed)) {
          nbx_send = send_msg_nbx(&app);
          if (nbx_send == nullptr) {
            app.state = app_state::SENT;
          } else if (UCS_PTR_IS_ERR(nbx_send)) {
            spdlog::error("send_msg_nbx: {}", ucs_status_string(UCS_PTR_RAW_STATUS(nbx_send)));
            app.state = app_state::ERROR;
          } else {
            app.state = app_state::SENDING;
          }
        }
        break;
      case app_state::SENDING:
        status = ucp_request_check_status(nbx_send);
        if (status == UCS_OK) {
          app.state = app_state::SENT;
        } else if (status != UCS_INPROGRESS) {
          app.state = app_state::ERROR;
        }
        break;
      case app_state::SENT:
        break;
      case app_state::DISCONNECTING:
      case app_state::DISCONNECTED:
      default:
        break;
    }
    ucp_worker_progress(app.worker);
  }

  if (app.state == app_state::CONNECTED) {
    close_ep(&app.worker, &app.ep);
  }

clean_conn_addr:
  if (!is_server(parsed)) {
    freeaddrinfo(conn_addr);
  }
clean_listener:
  if (is_server(parsed)) {
    ucp_listener_destroy(app.listener);
  }
clean_worker:
  ucp_worker_destroy(app.worker);
clean_ctx:
  ucp_cleanup(app.ctx);

  return 0;
}
