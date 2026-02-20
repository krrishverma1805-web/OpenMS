// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hendrik Weisser $
// $Authors: Hendrik Weisser $
// --------------------------------------------------------------------------
//
// IO-layer populator for RNaseDB: loads enzyme definitions from
// share/CHEMISTRY/Enzymes_RNA.xml and adds them to the RNaseDB singleton.
// This file is compiled into OpenMS_IO. Its static initializer registers
// the populator before main() runs.

#include <OpenMS/CHEMISTRY/RNaseDB.h>
#include <OpenMS/CHEMISTRY/DigestionEnzymeRNA.h>
#include <OpenMS/FORMAT/ParamXMLFile.h>
#include <OpenMS/SYSTEM/File.h>
#include <OpenMS/CONCEPT/Exception.h>

namespace OpenMS
{
  static const bool rnase_loader_registered_ = []() {
    RNaseDB::registerPopulator([](RNaseDB& db) {
      // Find Enzymes_RNA.xml — if not present, silently skip (built-in enzymes suffice)
      String file;
      try { file = File::find("CHEMISTRY/Enzymes_RNA.xml"); }
      catch (Exception::FileNotFound&) { return; }

      Param param;
      ParamXMLFile().load(file, param);
      if (param.empty()) return;

      std::vector<String> split;
      String(param.begin().getName()).split(':', split);
      if (split[0] != "Enzymes") return;

      try
      {
        std::map<String, String> values;
        String previous_enzyme = split[1];
        for (Param::ParamIterator it = param.begin(); it != param.end(); ++it)
        {
          String(it.getName()).split(':', split);
          if (split[0] != "Enzymes") break;
          if (split[1] != previous_enzyme)
          {
            DigestionEnzymeRNA* enzy = new DigestionEnzymeRNA();
            for (auto& kv : values) enzy->setValueFromFile(kv.first, kv.second);
            db.addEnzyme(enzy);
            previous_enzyme = split[1];
            values.clear();
          }
          values[it.getName()] = String(it->value.toString());
        }
        // add last enzyme
        DigestionEnzymeRNA* enzy = new DigestionEnzymeRNA();
        for (auto& kv : values) enzy->setValueFromFile(kv.first, kv.second);
        db.addEnzyme(enzy);
      }
      catch (Exception::BaseException& e)
      {
        throw Exception::ParseError(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, e.what(), "");
      }
    });
    return true;
  }();
} // namespace OpenMS
