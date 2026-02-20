# ConsensusMapArrowIO Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Full round-trip (export + import) for ConsensusMap data to/from Apache Arrow Tables and a directory of Parquet files.

**Architecture:** New `ConsensusMapArrowIO` class with 6 static methods (3 export, 3 import), following the `FeatureMapArrowIO` pattern. Reuses `QPXFile` for PSMs and `ProteinIdentificationArrowIO` for protein data. ConsensusMap-specific data (column headers, experiment type) stored as JSON in Arrow schema metadata. FeatureHandles stored as nested `list<struct>` within the consensus features table.

**Tech Stack:** Apache Arrow C++ API, Parquet C++ API, OpenMS test framework

**Design doc:** `docs/plans/2026-02-20-consensusmap-arrow-io-design.md`

---

### Task 1: Create header file and register in CMake

**Files:**
- Create: `src/openms/include/OpenMS/FORMAT/ConsensusMapArrowIO.h`
- Modify: `src/openms/include/OpenMS/FORMAT/sources.cmake:123-131` (add to `WITH_PARQUET` block)

**Step 1: Create header file**

```cpp
// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#pragma once

#include <OpenMS/config.h>

#ifdef WITH_PARQUET

#include <OpenMS/CONCEPT/Types.h>
#include <OpenMS/KERNEL/ConsensusMap.h>
#include <OpenMS/FORMAT/MSExperimentArrowExport.h>

#include <memory>

// Forward declarations
namespace arrow
{
  class Table;
}

namespace OpenMS
{

/**
  @brief Import and export ConsensusMap data to/from Apache Arrow format

  This class provides static methods to export and import ConsensusMap
  data to/from Apache Arrow Tables and Parquet files. Separate tables are
  provided for consensus features (with their handles and metadata) and for
  peptide spectrum matches (PSMs) associated with features.

  @experimental This API is experimental and may change in future versions.

  @ingroup FileIO
*/
class OPENMS_DLLAPI ConsensusMapArrowIO
{
public:
  // ==================== Export methods ====================

  /**
    @brief Export consensus features to Apache Arrow Table

    Each ConsensusFeature becomes one row with RT, MZ, intensity, charge,
    quality, width, nested FeatureHandles, and metadata columns.

    @param[in] cmap The ConsensusMap to export
    @return Shared pointer to Arrow Table, or nullptr on error
  */
  static std::shared_ptr<arrow::Table> exportFeaturesToArrow(
    const ConsensusMap& cmap);

  /**
    @brief Export peptide spectrum matches (PSMs) associated with consensus features to Apache Arrow Table

    Each PeptideHit from each PeptideIdentification (both feature-level
    and unassigned) becomes one row.

    @param[in] cmap The ConsensusMap whose identifications to export
    @return Shared pointer to Arrow Table, or nullptr on error
  */
  static std::shared_ptr<arrow::Table> exportPSMsToArrow(
    const ConsensusMap& cmap);

  /**
    @brief Export ConsensusMap to a directory of Parquet files

    Writes five Parquet files: consensus_features.parquet, psms.parquet,
    proteins.parquet, protein_groups.parquet, and search_params.parquet
    into the specified directory. Protein-level data is delegated to
    ProteinIdentificationArrowIO. ConsensusMap-level metadata
    (column headers, experiment type, DocumentIdentifier, DataProcessing)
    is stored as file-level key-value metadata in consensus_features.parquet.

    @param[in] cmap The ConsensusMap to export
    @param[in] directory Output directory path
    @param[in] config Parquet writing options
    @return true on success, false on error
  */
  static bool exportToParquet(
    const ConsensusMap& cmap,
    const String& directory,
    const ParquetWriteConfig& config = ParquetWriteConfig{});

  // ==================== Import methods ====================

  /**
    @brief Import consensus features from Apache Arrow Table

    Each row becomes a ConsensusFeature with RT, MZ, intensity, charge,
    quality, width, FeatureHandles, and metadata populated.

    @param[in] table Arrow Table with consensus feature data
    @param[out] cmap ConsensusMap to populate
    @return true on success, false on error
  */
  static bool importFeaturesFromArrow(
    const std::shared_ptr<arrow::Table>& table,
    ConsensusMap& cmap);

  /**
    @brief Import PSMs from Apache Arrow Table

    Reconstructs PeptideIdentifications and PeptideHits from the table
    and assigns them to the appropriate consensus features or as unassigned.

    @param[in] table Arrow Table with PSM data
    @param[out] cmap ConsensusMap to populate
    @return true on success, false on error
  */
  static bool importPSMsFromArrow(
    const std::shared_ptr<arrow::Table>& table,
    ConsensusMap& cmap);

  /**
    @brief Import ConsensusMap from a directory of Parquet files

    Reads five Parquet files (consensus_features.parquet, psms.parquet,
    proteins.parquet, protein_groups.parquet, search_params.parquet)
    from the specified directory and reconstructs a complete ConsensusMap
    including FeatureHandles, PSM linkage, protein identifications,
    and ConsensusMap-level metadata.

    @param[in] directory Input directory path containing Parquet files
    @param[out] cmap ConsensusMap to populate
    @return true on success, false on error
  */
  static bool importFromParquet(
    const String& directory,
    ConsensusMap& cmap);
};

} // namespace OpenMS

#endif // WITH_PARQUET
```

**Step 2: Register header in sources.cmake**

In `src/openms/include/OpenMS/FORMAT/sources.cmake`, inside the `if (WITH_PARQUET)` block (after line 130), add:

```cmake
  list(APPEND sources_list_h ConsensusMapArrowIO.h)
```

**Step 3: Commit**

```bash
git add src/openms/include/OpenMS/FORMAT/ConsensusMapArrowIO.h src/openms/include/OpenMS/FORMAT/sources.cmake
git commit -m "feat: add ConsensusMapArrowIO header with export/import API"
```

---

### Task 2: Create source file with export helpers and exportFeaturesToArrow

**Files:**
- Create: `src/openms/source/FORMAT/ConsensusMapArrowIO.cpp`
- Modify: `src/openms/source/FORMAT/sources.cmake:109-117` (add to `WITH_PARQUET` block)

**Step 1: Create source file with helpers + exportFeaturesToArrow**

The source file follows the same anonymous-namespace helper pattern as `FeatureMapArrowIO.cpp`. The helpers (`appendMetaValues_`, JSON serialization for DataProcessing, `writeArrowTableToParquet_`, `readParquetTable_`, `getColumn_`, type-safe getters, `readMetaValues_`) are duplicated (they live in anonymous namespaces, so they are TU-private). Copy them verbatim from `FeatureMapArrowIO.cpp` lines 39-733.

Then implement `exportFeaturesToArrow`:

**Schema (9 columns):**
- `unique_id` (uint64) - `cf.getUniqueId()`
- `rt` (float32) - `cf.getRT()` (Note: use float32 matching design; FeatureMap uses float64 for rt but ConsensusFeature precision is float-level)
- `mz` (float64) - `cf.getMZ()`
- `intensity` (float32) - `cf.getIntensity()`
- `charge` (int32) - `cf.getCharge()`
- `quality` (float32) - `cf.getQuality()`
- `width` (float32, nullable) - `cf.getWidth()`, null if 0
- `handles` list<struct{map_index: uint64, unique_id: uint64, rt: float32, mz: float64, intensity: float32, charge: int32, width: float32}>
- `metavalues` list<struct{name: utf8, value: utf8, value_type: utf8}>

**Key differences from FeatureMapArrowIO::exportFeaturesToArrow:**
- No `depth`, `parent_feature_id` columns (flat structure)
- No convex hull columns / bounding box columns
- No `overall_quality`, `quality_rt`, `quality_mz` - just single `quality`
- Added `handles` nested column built from `cf.getFeatures()` (the `std::set<FeatureHandle>`)
- Use `rt` as float32 (not float64) to match `Peak2D::getRT()` return precision
- Actually: use float64 for rt and mz to match FeatureMapArrowIO conventions for coordinate precision. Check: Peak2D stores position as DPosition<2> which is `double`. Use float64 for rt/mz.

```cpp
// -- handles: list<struct{map_index: uint64, unique_id: uint64, rt: float64, mz: float64, intensity: float32, charge: int32, width: float32}> --
auto handle_map_index_b = std::make_shared<arrow::UInt64Builder>();
auto handle_unique_id_b = std::make_shared<arrow::UInt64Builder>();
auto handle_rt_b = std::make_shared<arrow::DoubleBuilder>();
auto handle_mz_b = std::make_shared<arrow::DoubleBuilder>();
auto handle_intensity_b = std::make_shared<arrow::FloatBuilder>();
auto handle_charge_b = std::make_shared<arrow::Int32Builder>();
auto handle_width_b = std::make_shared<arrow::FloatBuilder>();

auto handle_struct_type = arrow::struct_({
  arrow::field("map_index", arrow::uint64()),
  arrow::field("unique_id", arrow::uint64()),
  arrow::field("rt", arrow::float64()),
  arrow::field("mz", arrow::float64()),
  arrow::field("intensity", arrow::float32()),
  arrow::field("charge", arrow::int32()),
  arrow::field("width", arrow::float32())
});

auto handle_struct_b = std::make_shared<arrow::StructBuilder>(
  handle_struct_type, arrow::default_memory_pool(),
  std::vector<std::shared_ptr<arrow::ArrayBuilder>>{
    handle_map_index_b, handle_unique_id_b, handle_rt_b, handle_mz_b,
    handle_intensity_b, handle_charge_b, handle_width_b});
arrow::ListBuilder handles_builder(arrow::default_memory_pool(), handle_struct_b);
```

For each ConsensusFeature, iterate `cf.getFeatures()` (returns `HandleSetType` = `std::set<FeatureHandle, IndexLess>`):

```cpp
// For each ConsensusFeature cf:
(void)handles_builder.Append(); // begin list for this feature
for (const auto& handle : cf.getFeatures())
{
  (void)handle_struct_b->Append();
  (void)handle_map_index_b->Append(handle.getMapIndex());
  (void)handle_unique_id_b->Append(handle.getUniqueId());
  (void)handle_rt_b->Append(handle.getRT());
  (void)handle_mz_b->Append(handle.getMZ());
  (void)handle_intensity_b->Append(handle.getIntensity());
  (void)handle_charge_b->Append(static_cast<int32_t>(handle.getCharge()));
  (void)handle_width_b->Append(handle.getWidth());
}
```

**Step 2: Register source in sources.cmake**

In `src/openms/source/FORMAT/sources.cmake`, inside the `if (WITH_PARQUET)` block (after line 116), add:

```cmake
  list(APPEND sources_list ConsensusMapArrowIO.cpp)
```

**Step 3: Add column_headers JSON serialization for export**

Add two helpers in the anonymous namespace for serializing/deserializing column headers:

```cpp
std::string serializeColumnHeaders_(const ConsensusMap::ColumnHeaders& headers)
{
  std::string json = "[";
  bool first = true;
  for (const auto& [map_index, header] : headers)
  {
    if (!first) json += ",";
    json += "{\"map_index\":" + std::to_string(map_index)
          + ",\"filename\":\"" + escapeJsonString_(header.filename) + "\""
          + ",\"label\":\"" + escapeJsonString_(header.label) + "\""
          + ",\"size\":" + std::to_string(header.size)
          + ",\"unique_id\":" + std::to_string(header.unique_id)
          + "}";
    first = false;
  }
  json += "]";
  return json;
}
```

Deserialization:
```cpp
ConsensusMap::ColumnHeaders deserializeColumnHeaders_(const std::string& json)
{
  ConsensusMap::ColumnHeaders result;
  if (json.empty()) return result;
  // Parse JSON array of objects with fields: map_index, filename, label, size, unique_id
  // Use same parseJsonString_ / skipWhitespace_ helpers as DataProcessing
  size_t pos = 0;
  skipWhitespace_(json, pos);
  if (pos >= json.size() || json[pos] != '[') return result;
  ++pos;

  while (pos < json.size())
  {
    skipWhitespace_(json, pos);
    if (pos >= json.size() || json[pos] == ']') break;
    if (json[pos] == ',') { ++pos; continue; }
    if (json[pos] != '{') break;
    ++pos;

    ConsensusMap::ColumnHeader header;
    UInt64 map_index = 0;

    while (pos < json.size())
    {
      skipWhitespace_(json, pos);
      if (pos >= json.size() || json[pos] == '}') { ++pos; break; }
      if (json[pos] == ',') { ++pos; continue; }

      std::string key = parseJsonString_(json, pos);
      skipWhitespace_(json, pos);
      if (pos < json.size() && json[pos] == ':') ++pos;
      skipWhitespace_(json, pos);

      if (key == "map_index" || key == "size" || key == "unique_id")
      {
        // Parse number
        std::string num_str;
        while (pos < json.size() && (std::isdigit(json[pos]) || json[pos] == '-'))
        {
          num_str += json[pos++];
        }
        if (key == "map_index") map_index = std::stoull(num_str);
        else if (key == "size") header.size = std::stoull(num_str);
        else if (key == "unique_id") header.unique_id = std::stoull(num_str);
      }
      else
      {
        std::string val = parseJsonString_(json, pos);
        if (key == "filename") header.filename = val;
        else if (key == "label") header.label = val;
      }
    }
    result[map_index] = header;
  }
  return result;
}
```

**Step 4: Build to verify compilation**

Run: `cmake --build OpenMS-build --target OpenMS -j$(nproc)`
Expected: Compiles without errors

**Step 5: Commit**

```bash
git add src/openms/source/FORMAT/ConsensusMapArrowIO.cpp src/openms/source/FORMAT/sources.cmake
git commit -m "feat: add ConsensusMapArrowIO source with exportFeaturesToArrow"
```

---

### Task 3: Implement exportPSMsToArrow and exportToParquet

**Files:**
- Modify: `src/openms/source/FORMAT/ConsensusMapArrowIO.cpp`

**Step 1: Implement exportPSMsToArrow**

This follows the same pattern as `FeatureMapArrowIO::exportPSMsToArrow` but iterates ConsensusFeatures instead of Features (no subordinate recursion needed since ConsensusFeatures are flat):

```cpp
std::shared_ptr<arrow::Table> ConsensusMapArrowIO::exportPSMsToArrow(
  const ConsensusMap& cmap)
{
  PeptideIdentificationList all_pep_ids;
  std::vector<std::pair<int64_t, bool>> feature_ids_per_pep_id;

  // Collect from consensus features (flat - no subordinates)
  for (const auto& cf : cmap)
  {
    for (const auto& pep_id : cf.getPeptideIdentifications())
    {
      all_pep_ids.push_back(pep_id);
      feature_ids_per_pep_id.push_back({static_cast<int64_t>(cf.getUniqueId()), false});
    }
  }

  // Unassigned
  for (const auto& pep_id : cmap.getUnassignedPeptideIdentifications())
  {
    all_pep_ids.push_back(pep_id);
    feature_ids_per_pep_id.push_back({0, true});
  }

  // Delegate to QPXFile
  auto base_table = QPXFile::exportToArrow(
    cmap.getProteinIdentifications(), all_pep_ids, true);
  if (!base_table) { return nullptr; }

  // Build consensus_feature_id column
  arrow::Int64Builder feature_id_builder;
  for (size_t i = 0; i < all_pep_ids.size(); ++i)
  {
    if (all_pep_ids[i].getHits().empty()) continue;
    for (size_t j = 0; j < all_pep_ids[i].getHits().size(); ++j)
    {
      if (feature_ids_per_pep_id[i].second)
        (void)feature_id_builder.AppendNull();
      else
        (void)feature_id_builder.Append(feature_ids_per_pep_id[i].first);
    }
  }

  std::shared_ptr<arrow::Array> feature_id_array;
  auto status = feature_id_builder.Finish(&feature_id_array);
  if (!status.ok()) { /* log error, return nullptr */ }

  if (feature_id_array->length() != base_table->num_rows()) { /* log mismatch, return nullptr */ }

  auto chunked = std::make_shared<arrow::ChunkedArray>(feature_id_array);
  auto result = base_table->AddColumn(0, arrow::field("consensus_feature_id", arrow::int64()), chunked);
  if (!result.ok()) { /* log error, return nullptr */ }
  return *result;
}
```

Note: column name is `consensus_feature_id` (not `feature_id`) to distinguish from the FeatureMap equivalent and to be self-documenting.

**Step 2: Implement exportToParquet**

Same pattern as `FeatureMapArrowIO::exportToParquet` but writes `consensus_features.parquet` and adds column_headers + experiment_type to schema metadata:

```cpp
bool ConsensusMapArrowIO::exportToParquet(
  const ConsensusMap& cmap,
  const String& directory,
  const ParquetWriteConfig& config)
{
  // 1. Create output directory
  std::filesystem::create_directories(std::string(directory));

  // 2. Export features table with ConsensusMap-level metadata
  auto features_table = exportFeaturesToArrow(cmap);
  if (!features_table) { return false; }

  std::unordered_map<std::string, std::string> cmap_metadata;
  cmap_metadata["column_headers"] = serializeColumnHeaders_(cmap.getColumnHeaders());
  cmap_metadata["experiment_type"] = cmap.getExperimentType();
  cmap_metadata["document_id"] = cmap.getIdentifier();
  cmap_metadata["loaded_file_path"] = cmap.getLoadedFilePath();
  cmap_metadata["loaded_file_type"] = FileTypes::typeToName(cmap.getLoadedFileType());
  cmap_metadata["data_processing"] = serializeDataProcessing_(cmap.getDataProcessing());

  if (!writeArrowTableToParquet_(features_table, directory + "/consensus_features.parquet",
                                  "consensus_features", config, cmap_metadata))
  { return false; }

  // 3. Export PSMs
  auto psms_table = exportPSMsToArrow(cmap);
  if (!psms_table) { return false; }
  if (!writeArrowTableToParquet_(psms_table, directory + "/psms.parquet", "psms", config))
  { return false; }

  // 4. Delegate protein data
  const auto& prot_ids = cmap.getProteinIdentifications();
  if (!ProteinIdentificationArrowIO::exportProteinsToParquet(prot_ids, directory + "/proteins.parquet", config))
  { return false; }
  if (!ProteinIdentificationArrowIO::exportProteinGroupsToParquet(prot_ids, directory + "/protein_groups.parquet", config))
  { return false; }
  if (!ProteinIdentificationArrowIO::exportSearchParamsToParquet(prot_ids, directory + "/search_params.parquet", config))
  { return false; }

  return true;
}
```

**Step 3: Build to verify**

Run: `cmake --build OpenMS-build --target OpenMS -j$(nproc)`

**Step 4: Commit**

```bash
git add src/openms/source/FORMAT/ConsensusMapArrowIO.cpp
git commit -m "feat: add exportPSMsToArrow and exportToParquet for ConsensusMapArrowIO"
```

---

### Task 4: Implement importFeaturesFromArrow

**Files:**
- Modify: `src/openms/source/FORMAT/ConsensusMapArrowIO.cpp`

**Step 1: Add readHandles_ helper**

Add to anonymous namespace:
```cpp
void readHandles_(
  const std::shared_ptr<arrow::Array>& array,
  int64_t row,
  ConsensusFeature& cf)
{
  if (!array || array->IsNull(row)) return;
  auto list_arr = std::static_pointer_cast<arrow::ListArray>(array);
  auto struct_arr = std::static_pointer_cast<arrow::StructArray>(list_arr->value_slice(row));
  if (!struct_arr || struct_arr->length() == 0) return;

  auto map_index_arr = std::static_pointer_cast<arrow::UInt64Array>(struct_arr->field(0));
  auto unique_id_arr = std::static_pointer_cast<arrow::UInt64Array>(struct_arr->field(1));
  auto rt_arr = std::static_pointer_cast<arrow::DoubleArray>(struct_arr->field(2));
  auto mz_arr = std::static_pointer_cast<arrow::DoubleArray>(struct_arr->field(3));
  auto intensity_arr = std::static_pointer_cast<arrow::FloatArray>(struct_arr->field(4));
  auto charge_arr = std::static_pointer_cast<arrow::Int32Array>(struct_arr->field(5));
  auto width_arr = std::static_pointer_cast<arrow::FloatArray>(struct_arr->field(6));

  for (int64_t i = 0; i < struct_arr->length(); ++i)
  {
    FeatureHandle handle;
    handle.setMapIndex(map_index_arr->Value(i));
    handle.setUniqueId(unique_id_arr->Value(i));
    handle.setRT(rt_arr->Value(i));
    handle.setMZ(mz_arr->Value(i));
    handle.setIntensity(intensity_arr->Value(i));
    handle.setCharge(static_cast<Int>(charge_arr->Value(i)));
    handle.setWidth(width_arr->Value(i));
    cf.insert(handle);
  }
}
```

**Step 2: Implement importFeaturesFromArrow**

Simpler than FeatureMapArrowIO since there's no hierarchy:
```cpp
bool ConsensusMapArrowIO::importFeaturesFromArrow(
  const std::shared_ptr<arrow::Table>& table,
  ConsensusMap& cmap)
{
  if (!table) { /* log error */ return false; }

  auto combined_result = table->CombineChunks(arrow::default_memory_pool());
  if (!combined_result.ok()) { return false; }
  auto tbl = *combined_result;

  int64_t num_rows = tbl->num_rows();
  if (num_rows == 0) return true;

  auto col_unique_id = getColumn_(tbl, "unique_id");
  auto col_rt = getColumn_(tbl, "rt");
  auto col_mz = getColumn_(tbl, "mz");
  auto col_intensity = getColumn_(tbl, "intensity");
  auto col_charge = getColumn_(tbl, "charge");
  auto col_quality = getColumn_(tbl, "quality");
  auto col_width = getColumn_(tbl, "width", /*required=*/false);
  auto col_handles = getColumn_(tbl, "handles", /*required=*/false);
  auto col_metavalues = getColumn_(tbl, "metavalues", /*required=*/false);

  if (!col_unique_id || !col_rt || !col_mz || !col_intensity || !col_charge || !col_quality)
  { return false; }

  for (int64_t i = 0; i < num_rows; ++i)
  {
    ConsensusFeature cf;
    cf.setUniqueId(static_cast<UInt64>(getInt64Value_(col_unique_id, i, 0)));
    cf.setRT(getDoubleValue_(col_rt, i));
    cf.setMZ(getDoubleValue_(col_mz, i));
    cf.setIntensity(getFloatValue_(col_intensity, i));
    cf.setCharge(static_cast<Int>(getInt32Value_(col_charge, i)));
    cf.setQuality(getFloatValue_(col_quality, i));

    if (col_width && !isNull_(col_width, i))
    {
      cf.setWidth(getFloatValue_(col_width, i));
    }

    if (col_handles)
    {
      readHandles_(col_handles, i, cf);
    }

    if (col_metavalues)
    {
      readMetaValues_(col_metavalues, i, cf);
    }

    cmap.push_back(std::move(cf));
  }

  return true;
}
```

Note: `unique_id` is stored as uint64 in export but Arrow's `getColumn_` + `getInt64Value_` reads it as int64. Need to use `getUInt64Value_` or cast from the uint64 array. Check: FeatureMapArrowIO stores unique_id as int64 (`arrow::Int64Builder`). For consistency, also use int64 here (cast UInt64 <-> int64_t at boundaries). Actually, let's use uint64 in the schema as designed and add a `getUInt64Value_` helper, or just use int64 like FeatureMapArrowIO does. **Decision: use int64 for unique_id** (same as FeatureMapArrowIO) for consistency. Update schema in export accordingly.

**Step 3: Build to verify**

Run: `cmake --build OpenMS-build --target OpenMS -j$(nproc)`

**Step 4: Commit**

```bash
git add src/openms/source/FORMAT/ConsensusMapArrowIO.cpp
git commit -m "feat: add importFeaturesFromArrow for ConsensusMapArrowIO"
```

---

### Task 5: Implement importPSMsFromArrow and importFromParquet

**Files:**
- Modify: `src/openms/source/FORMAT/ConsensusMapArrowIO.cpp`

**Step 1: Implement importPSMsFromArrow**

Nearly identical to `FeatureMapArrowIO::importPSMsFromArrow` but:
- Reads `consensus_feature_id` column instead of `feature_id`
- Feature lookup uses `ConsensusFeature*` instead of `Feature*` (no subordinate recursion)

```cpp
bool ConsensusMapArrowIO::importPSMsFromArrow(
  const std::shared_ptr<arrow::Table>& table,
  ConsensusMap& cmap)
{
  if (!table || table->num_rows() == 0) return true;

  auto combined_result = table->CombineChunks(arrow::default_memory_pool());
  if (!combined_result.ok()) { return false; }
  auto tbl = *combined_result;
  int64_t num_rows = tbl->num_rows();

  // Build feature lookup: unique_id -> ConsensusFeature*
  std::unordered_map<int64_t, ConsensusFeature*> feature_lookup;
  for (auto& cf : cmap)
  {
    feature_lookup[static_cast<int64_t>(cf.getUniqueId())] = &cf;
  }

  // Read columns (same as FeatureMapArrowIO::importPSMsFromArrow)
  auto col_feature_id = getColumn_(tbl, "consensus_feature_id");
  auto col_p_id = getColumn_(tbl, "P_ID");
  // ... (same column reads as FeatureMapArrowIO)

  // Same grouping logic as FeatureMapArrowIO::importPSMsFromArrow
  // Group by P_ID, reconstruct PeptideIdentifications, assign to features or unassigned
  // The only difference: use consensus_feature_id column name and ConsensusFeature* lookup
}
```

**Step 2: Implement importFromParquet**

```cpp
bool ConsensusMapArrowIO::importFromParquet(
  const String& directory,
  ConsensusMap& cmap)
{
  // 1. Import protein identifications
  std::vector<ProteinIdentification> prot_ids;
  if (!ProteinIdentificationArrowIO::importFromParquet(
          directory + "/proteins.parquet",
          directory + "/protein_groups.parquet",
          directory + "/search_params.parquet",
          prot_ids))
  { return false; }
  cmap.setProteinIdentifications(prot_ids);

  // 2. Import features and ConsensusMap-level metadata
  auto features_table = readParquetTable_(directory + "/consensus_features.parquet");
  if (!features_table) { return false; }

  auto schema_md = features_table->schema()->metadata();
  if (schema_md)
  {
    // Column headers
    int idx = schema_md->FindKey("column_headers");
    if (idx >= 0) cmap.setColumnHeaders(deserializeColumnHeaders_(schema_md->value(idx)));

    // Experiment type
    idx = schema_md->FindKey("experiment_type");
    if (idx >= 0) cmap.setExperimentType(schema_md->value(idx));

    // DocumentIdentifier
    idx = schema_md->FindKey("document_id");
    if (idx >= 0) cmap.setIdentifier(schema_md->value(idx));

    idx = schema_md->FindKey("loaded_file_path");
    if (idx >= 0) cmap.setLoadedFilePath(schema_md->value(idx));

    // DataProcessing
    idx = schema_md->FindKey("data_processing");
    if (idx >= 0) cmap.setDataProcessing(deserializeDataProcessing_(schema_md->value(idx)));
  }

  if (!importFeaturesFromArrow(features_table, cmap)) { return false; }

  // 3. Import PSMs
  auto psms_table = readParquetTable_(directory + "/psms.parquet");
  if (!psms_table) { return false; }
  if (!importPSMsFromArrow(psms_table, cmap)) { return false; }

  return true;
}
```

**Step 3: Build to verify**

Run: `cmake --build OpenMS-build --target OpenMS -j$(nproc)`

**Step 4: Commit**

```bash
git add src/openms/source/FORMAT/ConsensusMapArrowIO.cpp
git commit -m "feat: add importPSMsFromArrow and importFromParquet for ConsensusMapArrowIO"
```

---

### Task 6: Create test file - export tests

**Files:**
- Create: `src/tests/class_tests/openms/source/ConsensusMapArrowIO_test.cpp`
- Modify: `src/tests/class_tests/openms/executables.cmake:289-295` (add to `WITH_PARQUET` block)

**Step 1: Create test file with export tests**

```cpp
// Copyright (c) 2002-present, OpenMS Inc. -- EKU Tuebingen, ETH Zurich, and FU Berlin
// SPDX-License-Identifier: BSD-3-Clause
//
// --------------------------------------------------------------------------
// $Maintainer: Timo Sachsenberg $
// $Authors: Timo Sachsenberg $
// --------------------------------------------------------------------------

#include <OpenMS/CONCEPT/ClassTest.h>
#include <OpenMS/test_config.h>

///////////////////////////
#include <OpenMS/FORMAT/ConsensusMapArrowIO.h>
///////////////////////////

#include <OpenMS/config.h>

#ifdef WITH_PARQUET

#include <OpenMS/KERNEL/ConsensusMap.h>
#include <OpenMS/KERNEL/ConsensusFeature.h>
#include <OpenMS/KERNEL/FeatureHandle.h>
#include <OpenMS/DATASTRUCTURES/DateTime.h>
#include <OpenMS/FORMAT/FileTypes.h>
#include <OpenMS/METADATA/DataProcessing.h>
#include <OpenMS/METADATA/ProteinIdentification.h>
#include <OpenMS/METADATA/ProteinHit.h>
#include <OpenMS/METADATA/PeptideIdentification.h>
#include <OpenMS/METADATA/PeptideHit.h>
#include <OpenMS/CHEMISTRY/AASequence.h>

#include <arrow/api.h>

using namespace OpenMS;
using namespace std;

START_TEST(ConsensusMapArrowIO, "$Id$")
```

**Test sections:**

1. **exportFeaturesToArrow - empty ConsensusMap**: Verify 0 rows, 9 columns
2. **exportFeaturesToArrow - single feature with handles and metavalues**: Build a ConsensusFeature with 2 FeatureHandles (from different maps), metavalues. Verify column values.
3. **exportPSMsToArrow - empty**: Verify empty table
4. **exportPSMsToArrow - feature and unassigned PSMs**: Build consensus features with PeptideIdentifications, verify consensus_feature_id column

**Step 2: Register test in executables.cmake**

In `src/tests/class_tests/openms/executables.cmake`, line ~294 (inside `if(WITH_PARQUET)` block), add `ConsensusMapArrowIO_test` to the list.

**Step 3: Build and run test**

```bash
cmake --build OpenMS-build -j$(nproc)
ctest --test-dir OpenMS-build -R ConsensusMapArrowIO_test -V
```

**Step 4: Commit**

```bash
git add src/tests/class_tests/openms/source/ConsensusMapArrowIO_test.cpp src/tests/class_tests/openms/executables.cmake
git commit -m "test: add export tests for ConsensusMapArrowIO"
```

---

### Task 7: Add round-trip tests for features

**Files:**
- Modify: `src/tests/class_tests/openms/source/ConsensusMapArrowIO_test.cpp`

**Step 1: Add feature round-trip test**

Test section: **Round-trip features with handles and metavalues**

Build a ConsensusMap with:
- 2 ConsensusFeatures
- Each with 2-3 FeatureHandles (different map_index values)
- MetaValues of int, float, string types
- Varying charge, quality, width

Export via `exportFeaturesToArrow`, import via `importFeaturesFromArrow`, verify:
- Same number of features
- RT, MZ, intensity, charge, quality, width match
- UniqueId matches
- FeatureHandles: same count per feature, same map_index/unique_id/rt/mz/intensity/charge/width
- MetaValues preserved with correct types

**Step 2: Run tests**

```bash
cmake --build OpenMS-build --target ConsensusMapArrowIO_test -j$(nproc) && ctest --test-dir OpenMS-build -R ConsensusMapArrowIO_test -V
```

**Step 3: Commit**

```bash
git add src/tests/class_tests/openms/source/ConsensusMapArrowIO_test.cpp
git commit -m "test: add feature round-trip tests for ConsensusMapArrowIO"
```

---

### Task 8: Add full Parquet directory round-trip test

**Files:**
- Modify: `src/tests/class_tests/openms/source/ConsensusMapArrowIO_test.cpp`

**Step 1: Add full round-trip test**

Test section: **Full Parquet directory round-trip**

Build a ConsensusMap with:
- Column headers (2 entries with filenames, labels, sizes, unique IDs)
- Experiment type "label-free"
- 2 ConsensusFeatures with handles, metavalues, PeptideIdentifications
- Unassigned PeptideIdentifications
- ProteinIdentifications with search parameters and protein hits
- Protein groups
- DocumentIdentifier fields
- DataProcessing entries

Export via `exportToParquet` to a temp directory, import via `importFromParquet`, verify all data round-trips:
- Column headers
- Experiment type
- DocumentIdentifier
- DataProcessing
- Feature data (RT, MZ, intensity, charge, quality, width, handles)
- PSM data (sequences, scores, protein accessions)
- Protein identifications

Use `File::getTempDirectory()` or a hardcoded test temp path for the output directory.

**Step 2: Run tests**

```bash
cmake --build OpenMS-build --target ConsensusMapArrowIO_test -j$(nproc) && ctest --test-dir OpenMS-build -R ConsensusMapArrowIO_test -V
```

**Step 3: Commit**

```bash
git add src/tests/class_tests/openms/source/ConsensusMapArrowIO_test.cpp
git commit -m "test: add full Parquet round-trip test for ConsensusMapArrowIO"
```

---

### Task 9: Add PSM round-trip test and metadata round-trip test

**Files:**
- Modify: `src/tests/class_tests/openms/source/ConsensusMapArrowIO_test.cpp`

**Step 1: Add PSM round-trip test**

Test section: **PSM round-trip with feature and unassigned PSMs**

Build a ConsensusMap with:
- 1 ConsensusFeature with 1 PeptideIdentification (2 hits with different scores, charges)
- 1 unassigned PeptideIdentification
- ProteinIdentification with `isHigherScoreBetter` set
- Additional scores on PeptideHit
- is_decoy metavalue
- Multiple protein accessions

Export via `exportPSMsToArrow`, import via `importPSMsFromArrow`, verify:
- Correct number of PeptideIdentifications on feature vs unassigned
- Score types, higher_score_better
- PeptideHit sequences, scores, charges
- Protein accessions, is_decoy, additional scores

**Step 2: Add ConsensusMap metadata round-trip test**

Test section: **ConsensusMap metadata round-trip**

- Column headers with 3 entries, various filenames/labels
- Experiment type "labeled_MS1"
- DocumentIdentifier fields
- DataProcessing with software info, actions, metavalues

Export/import via `exportToParquet`/`importFromParquet`, verify all metadata.

**Step 3: Run tests**

```bash
cmake --build OpenMS-build --target ConsensusMapArrowIO_test -j$(nproc) && ctest --test-dir OpenMS-build -R ConsensusMapArrowIO_test -V
```

**Step 4: Commit**

```bash
git add src/tests/class_tests/openms/source/ConsensusMapArrowIO_test.cpp
git commit -m "test: add PSM and metadata round-trip tests for ConsensusMapArrowIO"
```

---

## Implementation Notes

**Helper duplication:** The anonymous-namespace helpers (`appendMetaValues_`, `writeArrowTableToParquet_`, `readParquetTable_`, `getColumn_`, type-safe getters, `readMetaValues_`, JSON serialization) are duplicated from `FeatureMapArrowIO.cpp`. This is intentional because they live in anonymous namespaces (TU-private). A future refactoring could extract them into a shared internal header, but that's out of scope.

**unique_id type:** Use `int64` (not `uint64`) for `unique_id` columns to match `FeatureMapArrowIO` convention. Cast `UInt64 <-> int64_t` at boundaries.

**consensus_feature_id vs feature_id:** The PSM table uses `consensus_feature_id` (not `feature_id`) to distinguish from the FeatureMap variant and to be self-documenting about which type of feature it references.

**FeatureHandle `insert` vs `push_back`:** ConsensusFeature stores handles in `std::set<FeatureHandle, IndexLess>`. Use `cf.insert(handle)` during import (not `push_back`).

**Column order in handles struct:** Fields ordered as: map_index, unique_id, rt, mz, intensity, charge, width. This must be consistent between export (builder order) and import (field index order).

**Error handling:** Follow the same pattern as FeatureMapArrowIO — log with `OPENMS_LOG_ERROR`/`OPENMS_LOG_WARN`, return false/nullptr on failure.
