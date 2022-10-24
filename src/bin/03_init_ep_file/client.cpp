#include <spdlog/cfg/env.h>
#include <spdlog/fmt/bin_to_hex.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <ucp/api/ucp.h>

#include <fstream>
#include <iterator>
#include <string>

bool read_worker_address(std::string file) {
  std::ifstream ifs{file, std::ios::binary};
  if (!ifs) {
    spdlog::error("open error: {}", std::strerror(errno));
    return false;
  }
  ifs.exceptions(std::ios::failbit | std::ios::badbit);
  try {
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    spdlog::info("worker address length = {}, address = {}", content.size(),
                 spdlog::to_hex(content));
  } catch (std::ifstream::failure& err) {
    spdlog::error("{}", err.what());
    return false;
  }

  return true;
}

bool write_worker_address(ucp_worker_h worker, std::string file) {
  ucp_address_t* addr;
  size_t addrlen;
  ucs_status_t status;
  status = ucp_worker_get_address(worker, &addr, &addrlen);
  if (status != UCS_OK) {
    spdlog::error("failed to ucp_worker_get_address: {}", ucs_status_string(status));
    return false;
  }

  ucp_worker_release_address(worker, addr);
  status = ucp_worker_get_address(worker, &addr, &addrlen);
  spdlog::info("worker address length = {}, address = {}", addrlen,
               spdlog::to_hex((char*)addr, (char*)addr + addrlen));

  std::ofstream ofs{file, std::ios::binary};
  if (!ofs) {
    spdlog::error("open error: {}", std::strerror(errno));
    goto clean_addr;
  }
  ofs.exceptions(std::ios::failbit | std::ios::badbit);
  try {
    ofs.write((char*)addr, addrlen);
    ofs.close();
  } catch (std::ofstream::failure& err) {
    spdlog::error("{}", err.what());
    goto clean_addr;
  }

  return true;
clean_addr:
  ucp_worker_release_address(worker, addr);
  return false;
}

auto main(int argc, char const* argv[]) -> int {
  spdlog::cfg::load_env_levels();
  read_worker_address("server_worker_addr");

  ucs_status_t status;
  ucp_context_h cxt;
  ucp_params_t params = {
      .field_mask = UCP_PARAM_FIELD_FEATURES,
      .features = UCP_FEATURE_AM | UCP_FEATURE_RMA | UCP_FEATURE_AMO64,
  };
  ucp_worker_h worker;
  ucp_worker_params_t wparams = {
      .field_mask = UCP_WORKER_PARAM_FIELD_THREAD_MODE,
      .thread_mode = UCS_THREAD_MODE_SINGLE,
  };

  spdlog::info("ucp init");
  status = ucp_init(&params, nullptr, &cxt);
  if (status != UCS_OK) {
    spdlog::error("failed to ucp_init: {}", ucs_status_string(status));
    return -1;
  }

  status = ucp_worker_create(cxt, &wparams, &worker);
  if (status != UCS_OK) {
    spdlog::error("failed to ucp_worker_create: {}", ucs_status_string(status));
    goto fail_worker_create;
  }

  if (!write_worker_address(worker, "server_worker_addr")) {
    goto fail_worker_get_address;
  }

  for (size_t i = 0; i < 10000; ++i) {
    ucp_worker_progress(worker);
  }

fail_worker_get_address:
  ucp_worker_destroy(worker);
fail_worker_create:
  ucp_cleanup(cxt);

  return 0;
}
