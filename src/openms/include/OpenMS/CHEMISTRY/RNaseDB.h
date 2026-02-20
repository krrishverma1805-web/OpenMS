// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Hendrik Weisser $
// $Authors: Hendrik Weisser $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/CHEMISTRY/DigestionEnzymeDB.h>
#include <OpenMS/CHEMISTRY/DigestionEnzymeRNA.h>
#include <OpenMS/DATASTRUCTURES/String.h>

#include <vector>

namespace OpenMS
{
  /**
    @ingroup Chemistry

    @brief Database for enzymes that digest RNA (RNases)

    The enzymes stored in this DB are defined as built-in defaults. Additional
    user-defined enzymes can be loaded from share/CHEMISTRY/Enzymes_RNA.xml if present.
  */
  class OPENMS_DLLAPI RNaseDB: public DigestionEnzymeDB<DigestionEnzymeRNA, RNaseDB>
  {
    // allow access to constructor in DigestionEnzymeDB::getInstance():
    friend class DigestionEnzymeDB<DigestionEnzymeRNA, RNaseDB>;

  protected:
    /// constructor
    RNaseDB();

    /// adds built-in RNA enzymes
    void addBuiltInEnzymes_();
  };
}

