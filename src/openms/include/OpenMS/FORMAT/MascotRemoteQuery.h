// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Chris Bielow $
// $Authors: Andreas Bertsch, Daniel Jameson, Chris Bielow, Timo Sachsenberg $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/DATASTRUCTURES/DefaultParamHandler.h>
#include <OpenMS/DATASTRUCTURES/String.h>
#include <string>
#include <vector>


namespace OpenMS
{
  /**
      @brief Class which handles the communication between OpenMS and the Mascot server

      This class provides a communication interface which is able to query the Mascot
      server and reports the identifications provided be the Mascot server

      @htmlinclude OpenMS_MascotRemoteQuery.parameters

  */
  // NOTE:
  // This class previously depended on Qt (QObject/QNetwork*). To remove Qt from OpenMS core,
  // the interface is stubbed for now to preserve symbol presence without Qt.
  // A proper cpp-httplib based implementation should replace this stub.
  class MascotRemoteQuery :
    public DefaultParamHandler
  {

public:

    /** @name Constructors and destructors
    */
    //@{
    /// default constructor
    OPENMS_DLLAPI MascotRemoteQuery(void* parent = nullptr);

    /// assignment operator
    OPENMS_DLLAPI MascotRemoteQuery& operator=(const MascotRemoteQuery& rhs) = delete;

    /// copy constructor
    OPENMS_DLLAPI MascotRemoteQuery(const MascotRemoteQuery& rhs) = delete;

    /// destructor
    OPENMS_DLLAPI ~MascotRemoteQuery() override;
    //@}

    /// sets the query spectra, given in MGF file format
    OPENMS_DLLAPI void setQuerySpectra(const String& exp);

    /// returns the Mascot XML response which contains the identifications
    OPENMS_DLLAPI OpenMS::String getMascotXMLResponse() const;

    /// returns the Mascot XML response which contains the decoy identifications (note: setExportDecoys must be set to true, otherwise result will be empty)
    OPENMS_DLLAPI OpenMS::String getMascotXMLDecoyResponse() const;

    /// predicate which returns true if an error occurred during the query
    OPENMS_DLLAPI bool hasError() const;

    /// returns the error message, if hasError can be used to check whether an error has occurred
    OPENMS_DLLAPI const String& getErrorMessage() const;

    /// returns the search number
    OPENMS_DLLAPI String getSearchIdentifier() const;

    /// request export of decoy summary and decoys (note: internal decoy search must be enabled in the MGF file passed to mascot)
    OPENMS_DLLAPI void setExportDecoys(const bool b);

protected:

    OPENMS_DLLAPI void updateMembers_() override;

public:

    /// synchronous execution (stub)
    OPENMS_DLLAPI void run();

private:

    /// login to Mascot server
    void login();

    /// execute query (upload file)
    void execQuery();

    /// download result file
    void getResults(const OpenMS::String& /*results_path*/) {}

    /// finish a run
    OPENMS_DLLAPI void endRun_() {}

    OPENMS_DLLAPI String getSearchIdentifierFromFilePath(const String& path) const;

    // Input / Output data
    String query_spectra_;
    String mascot_xml_;
    String mascot_decoy_xml_;

    // Internal data structures
    String cookie_;
    String error_message_;
    String search_identifier_;

    /// Path on mascot server
    String server_path_;
    /// Hostname of the mascot server
    String host_name_;
    /// Login required
    bool requires_login_;
    /// Use SSL connection
    bool use_ssl_;
    /// boundary string that will be embedded into the HTTP requests
    String boundary_;
    /// Timeout after these many seconds
    Int to_;

    bool export_decoys_ = false;
  };

}

