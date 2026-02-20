// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------
//
// IO-layer populator for ModificationsDB: loads modification definitions from
// Unimod XML and OBO files. This file is compiled into OpenMS_IO. Its static
// initializer registers the populator before main() runs.

#include <OpenMS/CHEMISTRY/ModificationsDB.h>

namespace OpenMS
{
  static const bool modifications_loader_registered_ = []() {
    ModificationsDB::registerPopulator([](ModificationsDB& db) {
      if (!db.getUnimodFile().empty())
      {
        db.readFromUnimodXMLFile(db.getUnimodFile());
      }
      if (!db.getCustomModFile().empty())
      {
        db.readFromUnimodXMLFile(db.getCustomModFile());
      }
      if (!db.getPsiModFile().empty())
      {
        db.readFromOBOFile(db.getPsiModFile());
      }
      if (!db.getXlModFile().empty())
      {
        db.readFromOBOFile(db.getXlModFile());
      }
    });
    return true;
  }();
} // namespace OpenMS
