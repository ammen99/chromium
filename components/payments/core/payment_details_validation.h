// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_PAYMENT_DETAILS_VALIDATION_H_
#define COMPONENTS_PAYMENTS_CORE_PAYMENT_DETAILS_VALIDATION_H_

#include <string>

// TODO(crbug.com/760946): Write unit tests for this function.

namespace payments {

class PaymentDetails;

bool ValidatePaymentDetails(const PaymentDetails& details,
                            std::string* error_message);

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_PAYMENT_DETAILS_VALIDATION_H_
