#ifndef LEVELDB_DB_VERSION_SET_H_
#define LEVELDB_DB_VERSION_SET_H_

#include <map>
#include <set>
#include <vector>
#include "dbformat.h"
#include "version_edit.h"
#include "../port/port.h"
#include "../port/thread_annotations.h"

namespace leveldb {

namespace log {
class Writer;
}

class Compaction;
class Iterator;
class MemTable;
class TableBuilder;
class TableCache;
class Version;
class VersionSet;
class WritableFile;
int FindFile(const InternalKeyComparator& icmp,
             const std::vector<FileMetaData*>& files, const Slice& key);
bool SomeFileOverlapsRange(const InternalKeyComparator& icmp,
                           bool disjoint_sorted_files,
                           const std::vector<FileMetaData*>& files,
                           const Slice* smallest_user_key,
                           const Slice* largest_user_key);

class Version {
 public:
  struct GetStats {
    FileMetaData* seek_file;
    int seek_file_level;
  };
  void AddIterators(const ReadOptions&, std::vector<Iterator*>* iters);
  Status Get(const ReadOptions&, const LookupKey& key, std::string* val,
             GetStats* stats);
  bool UpdateStats(const GetStats& stats);
  bool RecordReadSample(Slice key);
  void Ref();
  void Unref();
  void GetOverlappingInputs(
      int level,
      const InternalKey* begin,  // nullptr means before all keys
      const InternalKey* end,    // nullptr means after all keys
      std::vector<FileMetaData*>* inputs);
  bool OverlapInLevel(int level, const Slice* smallest_user_key,
                      const Slice* largest_user_key);
  int PickLevelForMemTableOutput(const Slice& smallest_user_key,
                                 const Slice& largest_user_key);

  int NumFiles(int level) const { return files_[level].size(); }

  std::string DebugString() const;

 private:
  friend class Compaction;
  friend class VersionSet;

  class LevelFileNumIterator;

  explicit Version(VersionSet* vset)
      : vset_(vset),
        next_(this),
        prev_(this),
        refs_(0),
        file_to_compact_(nullptr),
        file_to_compact_level_(-1),
        compaction_score_(-1),
        compaction_level_(-1) {}

  Version(const Version&) = delete;
  Version& operator=(const Version&) = delete;

  ~Version();

  Iterator* NewConcatenatingIterator(const ReadOptions&, int level) const;
  void ForEachOverlapping(Slice user_key, Slice internal_key, void* arg,
                          bool (*func)(void*, int, FileMetaData*));

  VersionSet* vset_;  // VersionSet to which this Version belongs
  Version* next_;     // Next version in linked list
  Version* prev_;     // Previous version in linked list
  int refs_;          // Number of live refs to this version

  std::vector<FileMetaData*> files_[config::kNumLevels];

  FileMetaData* file_to_compact_;
  int file_to_compact_level_;

  double compaction_score_;
  int compaction_level_;
};

class VersionSet {
 public:
  VersionSet(const std::string& dbname, const Options* options,
             TableCache* table_cache, const InternalKeyComparator*);
  VersionSet(const VersionSet&) = delete;
  VersionSet& operator=(const VersionSet&) = delete;

  ~VersionSet();
  Status LogAndApply(VersionEdit* edit, port::Mutex* mu)
      EXCLUSIVE_LOCKS_REQUIRED(mu);

  Status Recover(bool* save_manifest);

  Version* current() const { return current_; }

  uint64_t ManifestFileNumber() const { return manifest_file_number_; }

  uint64_t NewFileNumber() { return next_file_number_++; }

  void ReuseFileNumber(uint64_t file_number) {
    if (next_file_number_ == file_number + 1) {
      next_file_number_ = file_number;
    }
  }

  int NumLevelFiles(int level) const;

  int64_t NumLevelBytes(int level) const;

  uint64_t LastSequence() const { return last_sequence_; }

  void SetLastSequence(uint64_t s) {
    assert(s >= last_sequence_);
    last_sequence_ = s;
  }

  void MarkFileNumberUsed(uint64_t number);

  uint64_t LogNumber() const { return log_number_; }

  uint64_t PrevLogNumber() const { return prev_log_number_; }

  Compaction* PickCompaction();

  Compaction* CompactRange(int level, const InternalKey* begin,
                           const InternalKey* end);

  int64_t MaxNextLevelOverlappingBytes();

  Iterator* MakeInputIterator(Compaction* c);

  bool NeedsCompaction() const {
    Version* v = current_;
    return (v->compaction_score_ >= 1) || (v->file_to_compact_ != nullptr);
  }

  void AddLiveFiles(std::set<uint64_t>* live);

  uint64_t ApproximateOffsetOf(Version* v, const InternalKey& key);

  struct LevelSummaryStorage {
    char buffer[100];
  };
  const char* LevelSummary(LevelSummaryStorage* scratch) const;

 private:
  class Builder;

  friend class Compaction;
  friend class Version;

  bool ReuseManifest(const std::string& dscname, const std::string& dscbase);

  void Finalize(Version* v);

  void GetRange(const std::vector<FileMetaData*>& inputs, InternalKey* smallest,
                InternalKey* largest);

  void GetRange2(const std::vector<FileMetaData*>& inputs1,
                 const std::vector<FileMetaData*>& inputs2,
                 InternalKey* smallest, InternalKey* largest);

  void SetupOtherInputs(Compaction* c);

  Status WriteSnapshot(log::Writer* log);

  void AppendVersion(Version* v);

  Env* const env_;
  const std::string dbname_;
  const Options* const options_;
  TableCache* const table_cache_;
  const InternalKeyComparator icmp_;
  uint64_t next_file_number_;
  uint64_t manifest_file_number_;
  uint64_t last_sequence_;
  uint64_t log_number_;
  uint64_t prev_log_number_;  // 0 or backing store for memtable being compacted

  WritableFile* descriptor_file_;
  log::Writer* descriptor_log_;
  Version dummy_versions_;  // Head of circular doubly-linked list of versions.
  Version* current_;        // == dummy_versions_.prev_

  std::string compact_pointer_[config::kNumLevels];
};

class Compaction {
 public:
  ~Compaction();

  int level() const { return level_; }

  VersionEdit* edit() { return &edit_; }

  int num_input_files(int which) const { return inputs_[which].size(); }

  FileMetaData* input(int which, int i) const { return inputs_[which][i]; }

  uint64_t MaxOutputFileSize() const { return max_output_file_size_; }

  bool IsTrivialMove() const;

  void AddInputDeletions(VersionEdit* edit);

  bool IsBaseLevelForKey(const Slice& user_key);

  bool ShouldStopBefore(const Slice& internal_key);

  void ReleaseInputs();

 private:
  friend class Version;
  friend class VersionSet;

  Compaction(const Options* options, int level);

  int level_;
  uint64_t max_output_file_size_;
  Version* input_version_;
  VersionEdit edit_;

  std::vector<FileMetaData*> inputs_[2];  // The two sets of inputs

  std::vector<FileMetaData*> grandparents_;
  size_t grandparent_index_;  // Index in grandparent_starts_
  bool seen_key_;             // Some output key has been seen
  int64_t overlapped_bytes_;  // Bytes of overlap between current output
                              // and grandparent files
  size_t level_ptrs_[config::kNumLevels];
};
} 
#endif
