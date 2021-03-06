//===- EditedSource.h - Collection of source edits --------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_EDIT_EDITEDSOURCE_H
#define LLVM_CLANG_EDIT_EDITEDSOURCE_H

#include "Clang/IdentifierTable.h"
#include "Clang/LLVM.h"
#include "Clang/SourceLocation.h"
#include "Clang/FileOffset.h"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Allocator.h>
#include <map>
#include <tuple>
#include <utility>

namespace c2lang {

class SourceManager;

namespace edit {

class Commit;
class EditsReceiver;

class EditedSource {
  const SourceManager &SourceMgr;

  struct FileEdit {
    StringRef Text;
    unsigned RemoveLen = 0;

    FileEdit() = default;
  };

  using FileEditsTy = std::map<FileOffset, FileEdit>;

  FileEditsTy FileEdits;

  struct MacroArgUse {
    IdentifierInfo *Identifier;
    SourceLocation ImmediateExpansionLoc;

    // Location of argument use inside the top-level macro
    SourceLocation UseLoc;

    bool operator==(const MacroArgUse &Other) const {
      return std::tie(Identifier, ImmediateExpansionLoc, UseLoc) ==
             std::tie(Other.Identifier, Other.ImmediateExpansionLoc,
                      Other.UseLoc);
    }
  };

  llvm::DenseMap<unsigned, SmallVector<MacroArgUse, 2>> ExpansionToArgMap;
  SmallVector<std::pair<SourceLocation, MacroArgUse>, 2>
    CurrCommitMacroArgExps;

  IdentifierTable IdentTable;
  llvm::BumpPtrAllocator StrAlloc;

public:
  EditedSource(const SourceManager &SM
               )
      : SourceMgr(SM),  IdentTable() {}

  const SourceManager &getSourceManager() const { return SourceMgr; }


  bool canInsertInOffset(SourceLocation OrigLoc, FileOffset Offs);

  bool commit(const Commit &commit);

  void applyRewrites(EditsReceiver &receiver, bool adjustRemovals = true);
  void clearRewrites();

  StringRef copyString(StringRef str) { return str.copy(StrAlloc); }
  StringRef copyString(const Twine &twine);

private:
  bool commitInsert(SourceLocation OrigLoc, FileOffset Offs, StringRef text,
                    bool beforePreviousInsertions);
  bool commitInsertFromRange(SourceLocation OrigLoc, FileOffset Offs,
                             FileOffset InsertFromRangeOffs, unsigned Len,
                             bool beforePreviousInsertions);
  void commitRemove(SourceLocation OrigLoc, FileOffset BeginOffs, unsigned Len);

  StringRef getSourceText(FileOffset BeginOffs, FileOffset EndOffs,
                          bool &Invalid);
  FileEditsTy::iterator getActionForOffset(FileOffset Offs);
  void deconstructMacroArgLoc(SourceLocation Loc,
                              SourceLocation &ExpansionLoc,
                              MacroArgUse &ArgUse);

  void startingCommit();
  void finishedCommit();
};

} // namespace edit

} // namespace clang

#endif // LLVM_CLANG_EDIT_EDITEDSOURCE_H
