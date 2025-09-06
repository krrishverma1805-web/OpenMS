// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#include <OpenMS/SYSTEM/NetworkGetRequest.h>

#include <OpenMS/CONCEPT/LogStream.h>
#include <cpr/cpr.h>

using namespace std;

namespace OpenMS
{

  NetworkGetRequest::NetworkGetRequest(void* /*parent*/) :
    url_(),
    response_(),
    error_string_(),
    has_error_(false)
  {
  }

  NetworkGetRequest::~NetworkGetRequest() = default;

  void NetworkGetRequest::setUrl(const std::string& url)
  {
    url_ = url;
  }

  void NetworkGetRequest::run()
  {
    try
    {
      // Use CPR library for HTTP GET request
      cpr::Response r = cpr::Get(cpr::Url{url_}, 
                                 cpr::Timeout{30000}); // 30 seconds timeout
      
      // Check if request was successful
      if (r.status_code == 200)
      {
        response_ = r.text;
        has_error_ = false;
        error_string_.clear();
        OPENMS_LOG_DEBUG << "NetworkGetRequest: Successfully retrieved URL: " << url_ << std::endl;
      }
      else
      {
        has_error_ = true;
        error_string_ = "HTTP request failed with status code: " + std::to_string(r.status_code);
        response_.clear();
        OPENMS_LOG_WARN << "NetworkGetRequest: " << error_string_ << " for URL: " << url_ << std::endl;
      }
    }
    catch (const std::exception& e)
    {
      has_error_ = true;
      error_string_ = "HTTP request failed with exception: " + std::string(e.what());
      response_.clear();
      OPENMS_LOG_WARN << "NetworkGetRequest: " << error_string_ << " for URL: " << url_ << std::endl;
    }
  }

  void NetworkGetRequest::timeOut()
  {
    has_error_ = true;
    error_string_ = "Request timed out";
  }

  std::string NetworkGetRequest::getResponse() const
  {
    return response_;
  }

  const std::string& NetworkGetRequest::getResponseBinary() const
  {
    return response_;
  }

  bool NetworkGetRequest::hasError() const
  {
    return has_error_;
  }

  std::string NetworkGetRequest::getErrorString() const
  {
    return error_string_;
  }

} // namespace OpenMS