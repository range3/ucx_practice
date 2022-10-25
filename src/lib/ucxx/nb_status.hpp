#ifndef UCXX_NB_STATUS_HPP
#define UCXX_NB_STATUS_HPP

#include "raii_types.hpp"

namespace ucxx {
  inline ucs_status_ptr_t get_status_and_valid_ptr(ucs_status_ptr_t opt, ucs_status_t* status) {
    if (opt == nullptr) {
      *status = UCS_OK;
      return nullptr;
    } else if (UCS_PTR_IS_PTR(opt)) {
      *status = UCS_INPROGRESS;
      return opt;
    } else {
      *status = UCS_PTR_STATUS(opt);
      return nullptr;
    }
  }
}  // namespace ucxx

#endif
