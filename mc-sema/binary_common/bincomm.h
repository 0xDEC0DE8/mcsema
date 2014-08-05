/*
Copyright (c) 2014, Trail of Bits
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright notice, this  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  Neither the name of Trail of Bits nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef BINCOMM_H
#define BINCOMM_H
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCDisassembler.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/MemoryObject.h"
#include "llvm/Object/COFF.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/LLVMContext.h"
#include "llvm/Type.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include <string>
#include <list>
#include <boost/cstdint.hpp>

#include <BaseBMO.h>

typedef boost::uint64_t VA;

class ExecutableContainer : public llvm::MemoryObject {
public:
  static ExecutableContainer *open(std::string, const llvm::Target *);

  virtual ~ExecutableContainer(void)  { };
  
  //did constructor work (ugh)
  virtual bool is_open(void) = 0;

  //lookup imported symbol by address
  virtual bool find_import_name(uint32_t, std::string &) = 0;

  //is an address relocated
  virtual bool is_addr_relocated(uint32_t) = 0;

  //get the relocated value of an address
  virtual bool relocate_addr(VA, VA &) = 0;

  //get a list of exports and symbols 
  virtual bool get_exports(std::list<std::pair<std::string, VA> >  &) = 0;

  enum SectionType {
    CodeSection,
    DataSection
  };
  struct SectionDesc {
    std::string           secName;
    SectionType           type;
    uint32_t              base;
    std::vector<uint8_t>  contents;
    std::vector<VA>       reloc_addrs;
    bool read_only;
  };
  //get a list of all sections, their contents, base, and name
  virtual bool get_sections(std::vector<SectionDesc>  &) = 0;

  virtual std::string name(void) = 0;

  virtual uint64_t getBase() const = 0;
  virtual uint64_t getExtent() const = 0;
  virtual int readByte(uint64_t addr, uint8_t *b) const = 0;
};

#endif //BINCOMM_H
