// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#include <OpenMS/SYSTEM/NetworkGetRequest.h>

#include <OpenMS/CONCEPT/LogStream.h>

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
    // Stub implementation - networking functionality disabled
    // In a full implementation, this would use a HTTP client library like libcurl
    OPENMS_LOG_WARN << "NetworkGetRequest: HTTP functionality disabled in Qt-free build. URL was: " << url_ << std::endl;
    has_error_ = true;
    error_string_ = "HTTP functionality disabled in Qt-free build";
    response_ = "";
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