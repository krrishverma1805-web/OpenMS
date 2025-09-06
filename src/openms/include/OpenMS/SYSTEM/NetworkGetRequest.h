// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/config.h>

#include <string>

namespace OpenMS
{

  class NetworkGetRequest
  {

  public:

    /** @name Constructors and destructors
    */
    //@{
    /// default constructor
    OPENMS_DLLAPI NetworkGetRequest(void* parent = nullptr);

    /// destructor
    OPENMS_DLLAPI ~NetworkGetRequest();
    //@}

    // set request parameters
    OPENMS_DLLAPI void setUrl(const std::string& url);

    /// returns the response
    OPENMS_DLLAPI std::string getResponse() const;

    /// returns the response
    OPENMS_DLLAPI const std::string& getResponseBinary() const;

    /// returns true if an error occurred during the query
    OPENMS_DLLAPI bool hasError() const;

    /// returns the error message, if hasError can be used to check whether an error has occurred
    OPENMS_DLLAPI std::string getErrorString() const;

    /// run the request (blocking)
    OPENMS_DLLAPI void run();

    /// timeout handling
    OPENMS_DLLAPI void timeOut();

  private:
    std::string url_;
    std::string response_;
    std::string error_string_;
    bool has_error_;
  };

} // namespace OpenMS
  };
}

