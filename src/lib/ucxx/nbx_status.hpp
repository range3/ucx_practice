#ifndef UCXX_NBX_STATUS_HPP
#define UCXX_NBX_STATUS_HPP

#include "raii_types.hpp"

namespace ucxx {

  class nbx_status {
  public:
    nbx_status() = default;
    nbx_status(ucs_status_ptr_t sptr) {
      if (sptr == nullptr) {
        status_ = UCS_OK;
      } else if (UCS_PTR_IS_ERR(sptr)) {
        status_ = UCS_PTR_STATUS(sptr);
      } else {
        status_ = UCS_INPROGRESS;
        status_ptr_.reset(sptr);
      }
    }

    void update() { status_ = ucp_request_check_status(status_ptr_.get()); }
    ucs_status_t get() const { return status_; }

  private:
    ucs_status_t status_;
    unique_ucs_status_ptr_t status_ptr_;
  };

}  // namespace ucxx

#endif
