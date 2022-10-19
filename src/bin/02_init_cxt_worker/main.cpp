#include <spdlog/cfg/env.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <ucp/api/ucp.h>

auto main(int argc, char const* argv[]) -> int {
  spdlog::cfg::load_env_levels();

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

  for(size_t i = 0; i < 10000; ++i) {
    ucp_worker_progress(worker);
  }

  ucp_worker_destroy(worker);
fail_worker_create:
  ucp_cleanup(cxt);

  return 0;
}
