// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#include <OpenMS/SYSTEM/UpdateCheck.h>
#include <OpenMS/SYSTEM/File.h>
#include <OpenMS/CONCEPT/LogStream.h>

#ifdef OPENMS_WINDOWSPLATFORM
#include <sys/utime.h>
#elif __APPLE__
#include <utime.h>
#else
#include <utime.h>
#endif

#include <sys/stat.h>
#include <filesystem>
#include <chrono>
#include <fstream>

#include <OpenMS/CONCEPT/VersionInfo.h>
#include <httplib.h>
#include <regex>

using namespace std;
  
namespace OpenMS
{
  void UpdateCheck::run(const String& tool_name, const String& version, int debug_level)
  {
    String architecture = sizeof(void*) == 4 ? "32" : "64";

    // if the revision info is meaningful, show it as well
    String revision("UNKNOWN");
    if (!VersionInfo::getRevision().empty() && VersionInfo::getRevision() != "exported")
    {
      revision = VersionInfo::getRevision();
    }
    String platform;

#ifdef OPENMS_WINDOWSPLATFORM
    platform = "Win";
#elif __APPLE__
    platform = "Mac";
#elif __linux__
    platform = "Linux";
#elif __unix__
    platform = "Unix";
#else
    platform = "unknown";
#endif

    // write to tmp + userid folder

    // e.g.: OpenMS_Default_Win_64_FeatureFinderCentroided_2.0.0
    String tool_version_string;
    String config_path;
    //Comply with https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html on unix identifying systems
    #ifdef __unix__
    if (getenv("XDG_CONFIG_HOME"))
    {
      config_path = String(getenv("XDG_CONFIG_HOME")) + "/OpenMS";
    }
    else
    {
      config_path = File::getOpenMSHomePath() + "/.config/OpenMS";
    }
    #else
    config_path =  File::getOpenMSHomePath() + "/.OpenMS";
    #endif
    tool_version_string = String("OpenMS") + "_" + "Default_" + platform + "_" + architecture + "_" + tool_name + "_" + version;

    String version_file_name = config_path + "/" + tool_name + ".ver";

    // create version file if it doesn't exist yet
    bool first_run(false);
    if (!File::exists(version_file_name) || !File::readable(version_file_name))
    {
      // create OpenMS folder for .ver files
      std::filesystem::path config_dir(static_cast<std::string>(config_path));

      if (!std::filesystem::exists(config_dir))
      {
        std::filesystem::create_directories(config_dir);
      }

      // touch file to create it and set initial modification time stamp
      std::ofstream f(static_cast<std::string>(version_file_name));
      f.close();
      first_run = true;
    }

    if (File::readable(version_file_name))
    {
      // Get file modification time using std::filesystem
      std::filesystem::path version_file_path(static_cast<std::string>(version_file_name));
      auto last_modified = std::filesystem::last_write_time(version_file_path);
      
      // Convert to system time for comparison
      auto last_modified_sys = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        last_modified - std::chrono::file_clock::now() + std::chrono::system_clock::now());
      auto current_sys = std::chrono::system_clock::now();
      
      // Check if at least one day (24 hours) passed since last request
      auto time_diff = current_sys - last_modified_sys;
      auto hours_diff = std::chrono::duration_cast<std::chrono::hours>(time_diff);
      
      if (first_run || hours_diff.count() >= 24)
      {
        // update modification time stamp
        struct stat old_stat;
        struct utimbuf new_times;
        stat(version_file_name.c_str(), &old_stat);
        new_times.actime = old_stat.st_atime; // keep accession time unchanged 
        new_times.modtime = time(nullptr);  // mod time to current time
        utime(version_file_name.c_str(), &new_times);          

        if (debug_level > 0)
        {
          OPENMS_LOG_INFO << "The OpenMS team is collecting usage statistics for quality control and funding purposes." << endl;
          OPENMS_LOG_INFO << "We will never give out your personal data, but you may disable this functionality by " << endl;
          OPENMS_LOG_INFO << "setting the environmental variable OPENMS_DISABLE_UPDATE_CHECK to ON." << endl;
        }
      
        // Simple blocking HTTP request using cpp-httplib
        try
        {
          httplib::Client client("openms-update.cs.uni-tuebingen.de");
          client.set_connection_timeout(30, 0); // 30 seconds timeout
          
          std::string path = "/check/" + tool_version_string;
          auto response = client.Get(path.c_str());
          
          if (response && response->status == 200)
          {
            if (debug_level > 0)
            {
              OPENMS_LOG_INFO << "Connecting to REST server successful. " << endl;
            }

            std::string response_body = response->body;
            VersionInfo::VersionDetails server_version = VersionInfo::VersionDetails::create(String(response_body));
            if (server_version != VersionInfo::VersionDetails::EMPTY)
            {
              if (VersionInfo::getVersionStruct() < server_version)
              {
                OPENMS_LOG_INFO << "Version " + version + " of " + tool_name + " is available at www.OpenMS.de" << endl;
              }
            }
          }
          else
          {
            if (debug_level > 0)
            {
              OPENMS_LOG_INFO << "Connecting to REST server failed. Skipping update check." << endl;
              if (response)
              {
                OPENMS_LOG_INFO << "Error: HTTP " << response->status << endl;
              }
              else
              {
                OPENMS_LOG_INFO << "Error: Connection failed" << endl;
              }
            }
          }
        }
        catch (const std::exception& e)
        {
          if (debug_level > 0)
          {
            OPENMS_LOG_INFO << "Connecting to REST server failed. Skipping update check." << endl;
            OPENMS_LOG_INFO << "Error: " << e.what() << endl;
          }
        }
      }
    }
  }

}

