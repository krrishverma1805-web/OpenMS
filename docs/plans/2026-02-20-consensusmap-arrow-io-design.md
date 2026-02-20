# ConsensusMapArrowIO Design

## Goal

Full round-trip (export + import) for ConsensusMap data to/from Apache Arrow Tables and Parquet files. New `ConsensusMapArrowIO` class following the `FeatureMapArrowIO` pattern, with a schema tailored to ConsensusMap's flat structure.

## Output Files

```
output_dir/
  consensus_features.parquet   <- consensus features + nested handles
  psms.parquet                 <- via QPXFile (with consensus_feature_id)
  proteins.parquet             <- via ProteinIdentificationArrowIO
  protein_groups.parquet       <- via ProteinIdentificationArrowIO
  search_params.parquet        <- via ProteinIdentificationArrowIO
```

## consensus_features.parquet Schema

| Column | Arrow Type | Source |
|--------|-----------|--------|
| `unique_id` | uint64 | `cf.getUniqueId()` |
| `rt` | float32 | `cf.getRT()` |
| `mz` | float64 | `cf.getMZ()` |
| `intensity` | float32 | `cf.getIntensity()` |
| `charge` | int32 | `cf.getCharge()` |
| `quality` | float32 | `cf.getQuality()` |
| `width` | float32 | `cf.getWidth()` |
| `handles` | list\<struct\> | `cf.getFeatures()` |
| `metavalues` | list\<struct\> | MetaInfoInterface |

### handles struct

`{map_index: uint64, unique_id: uint64, rt: float32, mz: float64, intensity: float32, charge: int32, width: float32}`

### metavalues struct

`{name: utf8, value: utf8, value_type: int32}` — same pattern as FeatureMapArrowIO

### Schema-level metadata (Arrow key-value)

- `column_headers` — JSON array of `{map_index, filename, label, size, unique_id}`
- `experiment_type` — string
- `document_id`, `loaded_file_path`, `loaded_file_type` — DocumentIdentifier fields
- `data_processing` — JSON (same serialization as FeatureMapArrowIO)

## psms.parquet

Reuses `QPXFile::exportToArrow()` with an added `consensus_feature_id` column (uint64, nullable) linking each PSM row to its parent ConsensusFeature. Unassigned PSMs get null.

## proteins.parquet, protein_groups.parquet, search_params.parquet

Delegated entirely to `ProteinIdentificationArrowIO`.

## Public API

```cpp
class OPENMS_DLLAPI ConsensusMapArrowIO
{
public:
  // Export
  static std::shared_ptr<arrow::Table> exportFeaturesToArrow(const ConsensusMap& cmap);
  static std::shared_ptr<arrow::Table> exportPSMsToArrow(const ConsensusMap& cmap);
  static bool exportToParquet(const ConsensusMap& cmap, const String& directory,
                              const ParquetWriteConfig& config = ParquetWriteConfig{});

  // Import
  static bool importFeaturesFromArrow(const std::shared_ptr<arrow::Table>& table,
                                      ConsensusMap& cmap);
  static bool importPSMsFromArrow(const std::shared_ptr<arrow::Table>& table,
                                  ConsensusMap& cmap);
  static bool importFromParquet(const String& directory, ConsensusMap& cmap);
};
```

## Differences from FeatureMapArrowIO

- **No hierarchy columns** (`depth`, `parent_feature_id`) — ConsensusFeatures are flat
- **No convex hull columns** — ConsensusFeatures don't have convex hulls
- **Added `handles` column** — nested list\<struct\> for FeatureHandle data
- **Added `width` column** — ConsensusFeature has width for RT peak shape
- **Schema metadata includes `column_headers` and `experiment_type`** — ConsensusMap-specific

## Out of Scope

- `Ratio` struct on ConsensusFeature (experimental/rarely used, can be added later)

## Shared Components

- `QPXFile` — PSM export/import with consensus_feature_id linking
- `ProteinIdentificationArrowIO` — protein hits, groups, search parameters
- Helper functions reused from FeatureMapArrowIO pattern: `appendMetaValues_`, `writeArrowTableToParquet_`, `readParquetTable_`, `getColumn_`, type-safe getters, `readMetaValues_`, JSON serialization for DataProcessing
