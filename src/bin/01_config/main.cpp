#include <iostream>
using namespace std;
#include <spdlog/spdlog.h>
#include <ucp/api/ucp.h>

auto main(int argc, char const* argv[]) -> int {
  ucp_config_t* config;
  ucp_config_read(nullptr, nullptr, &config);
  ucp_config_print(config, stderr, "config:", UCS_CONFIG_PRINT_CONFIG);
  ucp_config_release(config);
  return 0;
}
