// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Xiao Liang $
// $Authors: Xiao Liang, Chris Bielow $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/CHEMISTRY/DigestionEnzyme.h>
#include <OpenMS/CONCEPT/Exception.h>
#include <OpenMS/CONCEPT/LogStream.h>
#include <OpenMS/DATASTRUCTURES/Param.h>
#include <OpenMS/DATASTRUCTURES/String.h>
#include <OpenMS/SYSTEM/File.h>

#include <functional>
#include <set>
#include <map>

namespace OpenMS
{
  /**
    @ingroup Chemistry

    @brief Digestion enzyme database (base class)

    Template parameters:
    @p DigestionEnzymeType should be a subclass of DigestionEnzyme.
    @p InstanceType should be a subclass of DigestionEnzymeDB ("Curiously Recurring Template Pattern", see https://stackoverflow.com/a/34519373).

    Subclasses define built-in enzyme data in their constructors. The IO layer can
    register a populator callback via registerPopulator() to load additional enzyme
    definitions from XML files after the DB is constructed.
  */
  template<typename DigestionEnzymeType, typename InstanceType> class DigestionEnzymeDB
  {
  public:

    /** @name Typedefs
    */
    //@{
    typedef typename std::set<const DigestionEnzymeType*>::const_iterator ConstEnzymeIterator;
    typedef typename std::set<const DigestionEnzymeType*>::iterator EnzymeIterator;
    /// Callback type for IO-layer populators that extend the DB after construction.
    using PopulatorFunc = std::function<void(InstanceType&)>;
    //@}

    /// Register a populator callback that will be called after the DB is constructed.
    /// The IO layer uses this to load enzyme definitions from XML files.
    static void registerPopulator(PopulatorFunc fn)
    {
      getPopulator_() = std::move(fn);
    }

    /// this member function serves as a replacement of the constructor
    static InstanceType* getInstance()
    {
      static InstanceType* db_ = nullptr;
      if (db_ == nullptr)
      {
        db_ = new InstanceType;
        auto& pop = getPopulator_();
        if (pop) pop(*db_);
      }
      return db_;
    }

    /// Add an enzyme to the DB. Takes ownership of the pointer.
    /// If an enzyme with the same name already exists, it is replaced.
    void addEnzyme(const DigestionEnzymeType* enzyme)
    {
      addEnzyme_(enzyme);
    }

    /** @name Constructors and Destructors
    */
    //@{
    /// destructor
    virtual ~DigestionEnzymeDB()
    {
      for (ConstEnzymeIterator it = const_enzymes_.begin(); it != const_enzymes_.end(); ++it)
      {
        delete *it;
      }
    }
    //@}

    /** @name Accessors
    */
    //@{
    /// returns a pointer to the enzyme with name (supports synonym names)
    /// @throw Exception::ElementNotFound if enzyme is unknown
    /// @note enzymes are registered in regular and in toLowercase() style, if unsure use toLowercase
    const DigestionEnzymeType* getEnzyme(const String& name) const
    {
      auto pos = enzyme_names_.find(name);
      if (pos == enzyme_names_.end())
      {
        throw Exception::ElementNotFound(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION, name);
      }
      return pos->second;
    }

    /// returns a pointer to the enzyme with cleavage regex
    /// @throw Exception::IllegalArgument if enzyme regex  is unregistered.
    const DigestionEnzymeType* getEnzymeByRegEx(const String& cleavage_regex) const
    {
      if (!hasRegEx(cleavage_regex))
      {
        // @TODO: why does this use a different exception than "getEnzyme"?
        throw Exception::IllegalArgument(__FILE__, __LINE__, OPENMS_PRETTY_FUNCTION,
                                         String("Enzyme with regex " + cleavage_regex + " was not registered in Enzyme DB, register first!").c_str());
      }
      return enzyme_regex_.at(cleavage_regex);
    }

    /// returns all the enzyme names (does NOT include synonym names)
    void getAllNames(std::vector<String>& all_names) const
    {
      all_names.clear();
      for (ConstEnzymeIterator it = const_enzymes_.begin(); it != const_enzymes_.end(); ++it)
      {
        all_names.push_back((*it)->getName());
      }
    }
    //@}

    /** @name Predicates
    */
    //@{
    /// returns true if the db contains a enzyme with the given name (supports synonym names)
    bool hasEnzyme(const String& name) const
    {
      return (enzyme_names_.find(name) != enzyme_names_.end());
    }

    /// returns true if the db contains a enzyme with the given regex
    bool hasRegEx(const String& cleavage_regex) const
    {
      return (enzyme_regex_.find(cleavage_regex) != enzyme_regex_.end());
    }

    /// returns true if the db contains the enzyme of the given pointer
    bool hasEnzyme(const DigestionEnzymeType* enzyme) const
    {
      return (const_enzymes_.find(enzyme) != const_enzymes_.end() );
    }
    //@}

    /** @name Iterators
    */
    //@{
    inline ConstEnzymeIterator beginEnzyme() const { return const_enzymes_.begin(); }  // we only allow constant iterators -- this DB is not meant to be modifiable
    inline ConstEnzymeIterator endEnzyme() const { return const_enzymes_.end(); }

    //@}
  protected:
    DigestionEnzymeDB() = default;

    ///copy constructor
    DigestionEnzymeDB(const DigestionEnzymeDB& enzymes_db) = delete;
    //@}

    /** @name Assignment
    */
    //@{
    /// assignment operator
    DigestionEnzymeDB& operator=(const DigestionEnzymeDB& enzymes_db) = delete;
    //@}

    /// add to internal data; also update indices for search by name and regex.
    /// If an enzyme with the same name already exists, it is replaced.
    void addEnzyme_(const DigestionEnzymeType* enzyme)
    {
      String name = enzyme->getName();

      // if an enzyme with the same name exists, remove the old one first
      auto existing = enzyme_names_.find(name);
      if (existing != enzyme_names_.end())
      {
        const DigestionEnzymeType* old = existing->second;
        const_enzymes_.erase(old);
        // remove old name/synonym entries
        String old_name = old->getName();
        enzyme_names_.erase(old_name);
        enzyme_names_.erase(old_name.toLower());
        for (const auto& syn : old->getSynonyms())
        {
          enzyme_names_.erase(syn);
        }
        // remove old regex entry
        if (!old->getRegEx().empty())
        {
          enzyme_regex_.erase(old->getRegEx());
        }
        delete old;
      }

      // add to internal storage
      const_enzymes_.insert(enzyme);
      // add to internal indices (by name and its synonyms)
      enzyme_names_[name] = enzyme;
      enzyme_names_[name.toLower()] = enzyme;
      for (std::set<String>::const_iterator it = enzyme->getSynonyms().begin(); it != enzyme->getSynonyms().end(); ++it)
      {
        enzyme_names_[*it] = enzyme;
      }
      // ... and by regex
      if (enzyme->getRegEx() != "")
      {
        enzyme_regex_[enzyme->getRegEx()] = enzyme;
      }
      return;
    }

    std::map<String, const DigestionEnzymeType*> enzyme_names_; ///< index by names

    std::map<String, const DigestionEnzymeType*> enzyme_regex_; ///< index by regex

    std::set<const DigestionEnzymeType*> const_enzymes_; ///< set of enzymes

  private:
    /// Returns a reference to the static populator callback for this DB type.
    static PopulatorFunc& getPopulator_()
    {
      static PopulatorFunc populator;
      return populator;
    }

  };
}
