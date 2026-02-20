// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hendrik Weisser  $
// $Authors: Hendrik Weisser $
// --------------------------------------------------------------------------
//

#include <OpenMS/CHEMISTRY/RNaseDB.h>

using namespace std;

namespace OpenMS
{
  RNaseDB::RNaseDB():
    DigestionEnzymeDB<DigestionEnzymeRNA, RNaseDB>()
  {
    addBuiltInEnzymes_();
    // XML loading (CHEMISTRY/Enzymes_RNA.xml) is handled by RNaseDBLoader in the IO layer
    // via registerPopulator(), which is called after construction by getInstance().
  }

  void RNaseDB::addBuiltInEnzymes_()
  {
    // Built-in enzyme definitions extracted from share/OpenMS/CHEMISTRY/Enzymes_RNA.xml

    auto make = [&](const String& name, const String& desc,
                     const String& cuts_after, const String& cuts_before,
                     const String& three_prime_gain = "", const String& five_prime_gain = "")
    {
      DigestionEnzymeRNA* e = new DigestionEnzymeRNA();
      e->setName(name);
      e->setRegExDescription(desc);
      e->setCutsAfterRegEx(cuts_after);
      e->setCutsBeforeRegEx(cuts_before);
      if (!three_prime_gain.empty()) e->setThreePrimeGain(three_prime_gain);
      if (!five_prime_gain.empty()) e->setFivePrimeGain(five_prime_gain);
      addEnzyme_(e);
    };

    // RNase_T1
    make("RNase_T1",
         "RNase T1 cuts after G.",
         "G(?!m)", ".*",
         "p");

    // RNase_T1_Phosphatase
    make("RNase_T1_Phosphatase",
         "RNase T1 cuts after G. Then a phosphatase is added to remove the 3' terminal phosphate",
         "G(?!m)", ".*");

    // cusativin
    make("cusativin",
         "Cusativin cuts after a row of Cs.",
         "C(?!m)", "^[^C]+$",
         "p");

    // RNase_U2
    make("RNase_U2",
         "RNase U2 cuts after A or G.",
         "G|A(?!m)", ".*",
         "p");

    // RNase_A
    make("RNase_A",
         "RNase A cuts after C or U.",
         "[CUY](?!m)", ".*",
         "p");

    // RNase_MC1
    make("RNase_MC1",
         "RNase MC1 cuts before U.",
         ".*(?!m)$", "U|Y",
         "p");

    // RNase_H
    make("RNase_H",
         "RNase H can cut anywhere (in an RNA-DNA duplex).",
         ".*", ".*",
         "", "p");

    // RNase_4
    make("RNase_4",
         "RNase 4 cuts after U (or modified U) only if followed by A or G. NB: 3' ends are Heterogenous, run in parallel with RNase_4p and RNase_4c to get 3'p and 3'c as well",
         "U|P|]|D|5", "G|A");

    // RNase_4p
    make("RNase_4p",
         "RNase 4 cuts after U (or modified U) only if followed by A or G. NB: 3' ends are Heterogenous, run in parallel with RNase_4 and RNase_4c to get 3'OH and 3'c as well",
         "U|P|m1Y|D|5", "G|A",
         "p");

    // RNase_4c
    make("RNase_4c",
         "RNase 4 cuts after U (or modified U) only if followed by A or G. NB: 3' ends are Heterogenous, run in parallel with RNase_4 and RNase_4p to get 3'OH and 3'p as well",
         "U|P|m1Y|D|5", "G|A",
         "c");

    // mazF
    make("mazF",
         "mazF cuts before ACA but does not cut if the first A is an m6A OR the C is an m5C. NB cleaving behaviour relative to other methylation is not considered",
         ".*", "(?<!m6)A,(?<!m5)C,A",
         "p");

    // colicin_E5
    make("colicin_E5",
         "colicin E5 cuts after G (or Q) followed by U.",
         "G|Q(?!m)", "U",
         "p");

    // no cleavage
    make("no cleavage",
         "No cleavage.",
         "", "");

    // unspecific cleavage
    make("unspecific cleavage",
         "Unspecific cleavage cuts at every site.",
         ".*", ".*",
         "p");
  }

} // namespace OpenMS
