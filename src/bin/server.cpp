#include <iostream>
using namespace std;
#include <spdlog/spdlog.h>
#include <ucp/api/ucp.h>

auto main(int argc, char const* argv[]) -> int {
  spdlog::info("hello spdlog.");

  ucp_context_h ucp_context;
  ucp_worker_h  ucp_worker;
  ucp_params_t ucp_params;
  ucs_status_t status;
  memset(&ucp_params, 0, sizeof(ucp_params));
  ucp_params.field_mask = UCP_PARAM_FIELD_FEATURES;
  ucp_params.features = UCP_FEATURE_AM;

  spdlog::info("ucp init");
  status = ucp_init(&ucp_params, nullptr, &ucp_context);
  if (status != UCS_OK) {
    fprintf(stderr, "failed to ucp_init (%s)\n", ucs_status_string(status));
    return -1;
  }

  ucp_cleanup(ucp_context);

  return 0;
}
