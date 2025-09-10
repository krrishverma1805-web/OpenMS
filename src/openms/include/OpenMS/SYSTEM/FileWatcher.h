// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Marc Sturm $
// --------------------------------------------------------------------------


#pragma once

//OpenMS
#include <OpenMS/CONCEPT/Types.h>
#include <OpenMS/DATASTRUCTURES/String.h>
#include <map>

// NOTE: Qt dependency removed from OpenMS core. FileWatcher is disabled in core build.
// If functionality is required, it must be provided from OpenMS GUI or via a non-Qt implementation.

//STL
#include <map>

namespace OpenMS
{
  class String;

  /**
      @brief Watcher that monitors file changes.

      This class can be used similar to QFileSystemWatcher.
      Additionally it offers a delayed fileChanged signal.

      This behaviour is required for the following reason:
      Normally QFileSystemWatcher emits a signal every time a file is changed.
      This causes several signals for large files (one for each flush of the buffer).

      @ingroup System
  */
  // FileWatcher is not available without Qt in OpenMS core.
  // A lightweight stub is provided to keep API presence without linking Qt.
  class OPENMS_DLLAPI FileWatcher
  {
  public:
    FileWatcher(void* /*parent*/ = nullptr) {}
    ~FileWatcher() = default;

    inline void setDelayInSeconds(double /*delay*/) {}

    inline void addFile(const String& /*path*/) {}
    inline void removeFile(const String& /*path*/) {}
  };

  // OPENMS_DLLAPI extern FileWatcher myFileWatcher_instance;
}

