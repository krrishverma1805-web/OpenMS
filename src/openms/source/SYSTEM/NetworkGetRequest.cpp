// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#include <OpenMS/SYSTEM/NetworkGetRequest.h>

#include <OpenMS/CONCEPT/LogStream.h>
#include <httplib.h>
#include <regex>

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
      // Parse URL to extract host and path
      std::regex url_regex("^https?://([^/]+)(.*)$");
      std::smatch matches;
      
      if (!std::regex_match(url_, matches, url_regex))
      {
        has_error_ = true;
        error_string_ = "Invalid URL format: " + url_;
        response_.clear();
        OPENMS_LOG_WARN << "NetworkGetRequest: " << error_string_ << std::endl;
        return;
      }
      
      std::string host = matches[1].str();
      std::string path = matches[2].str();
      if (path.empty()) path = "/";
      
      OPENMS_LOG_DEBUG << "NetworkGetRequest: Connecting to host: " << host << ", path: " << path << std::endl;
      
      // Create HTTP client
      httplib::Client cli(host);
      cli.set_connection_timeout(30); // 30 seconds timeout
      cli.set_read_timeout(30);
      
      // Make GET request
      auto res = cli.Get(path);
      
      if (res)
      {
        if (res->status == 200)
        {
          response_ = res->body;
          has_error_ = false;
          error_string_.clear();
          OPENMS_LOG_DEBUG << "NetworkGetRequest: Successfully retrieved URL: " << url_ << std::endl;
        }
        else
        {
          has_error_ = true;
          error_string_ = "HTTP request failed with status code: " + std::to_string(res->status);
          response_.clear();
          OPENMS_LOG_WARN << "NetworkGetRequest: " << error_string_ << " for URL: " << url_ << std::endl;
        }
      }
      else
      {
        has_error_ = true;
        error_string_ = "HTTP request failed: connection error";
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