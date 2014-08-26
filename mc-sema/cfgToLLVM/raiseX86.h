/*
Copyright (c) 2013, Trail of Bits
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

  Redistributions in binary form must reproduce the above copyright notice, this  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

  Neither the name of the {organization} nor the names of its
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
#pragma once
#include <list>
#include <set>
#include <vector>

#include "TransExcn.h"
#include "llvm/BasicBlock.h"
#include "peToCFG.h"
#include "toModule.h"
#include <llvm/Constants.h>

enum InstTransResult {
    ContinueBlock,
    EndBlock,
    EndCFG,
    TranslateErrorUnsupported,
    TranslateError
};

//type that maps registers to their Value defn in a flow
typedef std::vector<llvm::Value*>   regDefT;

//setup code in the first block of the function that defines all of the
//registers via alloca and then copies into them from the structure argument
void setupFlow(llvm::Function *, regDefT &);

llvm::BasicBlock *bbFromStrName(std::string n, llvm::Function *F);

///////////////////////////////////////////////////////////////////////////////
// state modeling functions
///////////////////////////////////////////////////////////////////////////////

llvm::Value *MCRegToValue(llvm::BasicBlock *b, unsigned reg);
int mapPlatRegToOffset(unsigned reg);
int mapStrToGEPOff(std::string regName);
int mapStrToFloatOff(std::string regName);
llvm::Value *lookupLocalByName(llvm::Function *F, std::string localName);
void writeLocalsToContext(llvm::BasicBlock *B, unsigned bits);
void writeContextToLocals(llvm::BasicBlock *B, unsigned bits);

template <int width>
llvm::ConstantInt *CONST_V_INT(llvm::LLVMContext &ctx, uint64_t val) {
    llvm::IntegerType *bTy = llvm::Type::getIntNTy(ctx, width);
    return llvm::ConstantInt::get(bTy, val);
}

template <int width>
llvm::ConstantInt *CONST_V(llvm::BasicBlock *b, uint64_t val) {
    llvm::IntegerType *bTy = llvm::Type::getIntNTy(b->getContext(), width);
    return llvm::ConstantInt::get(bTy, val);
}

template <int width>
llvm::Value *R_READ(llvm::BasicBlock *b, unsigned reg) {
    //we should return the pointer to the Value object that represents the 
    //value read. we'll truncate that value to the width specified by 
    //the width parameter. 

    //lookup the value that defines the cell that stores the current register
    llvm::Value   *localRegVar = MCRegToValue(b, reg);
    if(localRegVar == NULL)
    {
      throw TErr(__LINE__, __FILE__, "Could not find register");
    }
    //assert(localRegVar != NULL);

    //do a load from this value into a temporary
    llvm::Value   *tmpVal = new llvm::LoadInst(localRegVar, "", b);
    TASSERT(tmpVal != NULL, "Could not read from register");

	llvm::Value	*readVal;
    //if the width requested is less than the native bitwidth, 
    //then we need to truncate the read
    if( width != 32 ) {
        int     readOff = mapPlatRegToOffset(reg);
        llvm::Value   *shiftedVal = NULL;

        if( readOff ) {
            //if we are reading from a subreg that is a non-zero
            //offset, we need to do some bitshifting
			shiftedVal = 
				llvm::BinaryOperator::Create(llvm::Instruction::LShr, 
										tmpVal, 
										CONST_V<32>(b, readOff), 
										"", 
										b);
        } else {
            shiftedVal = tmpVal;
        }

		//then, truncate the shifted value to the appropriate width
		readVal = new llvm::TruncInst(shiftedVal, 
								llvm::Type::getIntNTy(b->getContext(), width), 
								"", 
								b);
    } else {
		readVal = tmpVal;
	}

    //return that temporary
    return readVal;
}

template <int width>
void R_WRITE(llvm::BasicBlock *b, unsigned reg, llvm::Value *write) {
    //we don't return anything as this becomes a store

    //lookup the 'stack' local for the register we want to write
    llvm::Value   *localRegVar = MCRegToValue(b, reg);
    if(localRegVar == NULL)
      throw TErr(__LINE__, __FILE__, "Could not find register");

    //store to that stack local
	if( width == 32 ) {
		llvm::Value   *v = new llvm::StoreInst(write, localRegVar, b);
		TASSERT(v != NULL, "");
	} else {
		//we need to model this as a write of a specific offset and width
		int		writeOff = mapPlatRegToOffset(reg);
		llvm::Value	*maskVal;
		llvm::Value	*addVal;
		llvm::Value	*write_z = 
			new llvm::ZExtInst(write, llvm::Type::getInt32Ty(b->getContext()), "", b);

		//maskVal will be whatever the appropriate mask is
		//addVal will be the value in 'write', shifted appropriately
		if( writeOff ) {
			//this is a write to a high offset + some width, so, 
			//shift the mask and add values to the left by writeOff
            switch(width) {
                case 8:
                    maskVal = CONST_V<32>(b, ~0xFF00);
                    addVal = llvm::BinaryOperator::CreateShl(	write_z, 
                            CONST_V<32>(b, writeOff), 
                            "", 
                            b);
                    break;

                default:
                    throw TErr(__LINE__, __FILE__, "Unsupported bit width in write");
            }
		} else {
			//this is a write to the base + some width
			//simply compute the mask and add values 
            switch(width) {
                case 16:
                    maskVal = CONST_V<32>(b, ~0xFFFF);
                    addVal = write_z;
                    break;

                case 8:
                    maskVal = CONST_V<32>(b, ~0xFF);
                    addVal = write_z;
                    break;

                default:
                    throw TErr(__LINE__, __FILE__, "Unsupported bit width in write");
            }
		}

		//read the full register
		llvm::Value	*fullReg = R_READ<32>(b, reg);

		//AND the value with maskVal
		llvm::Value	*andedVal =
			llvm::BinaryOperator::CreateAnd(fullReg, maskVal, "", b);

		//ADD the addVal to the resulting value
		llvm::Value	*addedVal = 
			llvm::BinaryOperator::CreateAdd(andedVal, addVal, "", b);

		//write this value back into the full-width local
		R_WRITE<32>(b, reg, addedVal);
	}

    return;
}

template <int width>
llvm::Value *INTERNAL_M_READ(unsigned addrspace, llvm::BasicBlock *b, llvm::Value *addr) {
    //this is straightforward
    llvm::Value   *readLoc = addr;

    if( addr->getType()->isPointerTy() == false ) {
		llvm::Type    *ptrTy = llvm::Type::getIntNPtrTy(b->getContext(), width, addrspace);
        readLoc = new llvm::IntToPtrInst(addr, ptrTy, "", b); 
	} else if( addr->getType() != llvm::Type::getIntNPtrTy(b->getContext(), width, addrspace) ) {
		//we need to bitcast the pointer value to a pointer type of the appropriate width
		readLoc = llvm::CastInst::CreatePointerCast(addr, llvm::Type::getIntNPtrTy(b->getContext(), width, addrspace), "", b);
	}

    bool is_volatile = addrspace != 0;
    llvm::Value   *read = new llvm::LoadInst(readLoc, "", is_volatile , b);

    return read;
}

template <int width>
llvm::Value *M_READ(InstPtr ip, llvm::BasicBlock *b, llvm::Value *addr) {
    return INTERNAL_M_READ<width>(ip->get_addr_space(), b, addr);
}

template <int width>
llvm::Value *M_READ_0(llvm::BasicBlock *b, llvm::Value *addr) {
    return INTERNAL_M_READ<width>(0, b, addr);
}

// defined in raiseX86.cpp
void M_WRITE_T(InstPtr ip, llvm::BasicBlock *b, llvm::Value *addr, llvm::Value *data, llvm::Type *ptrtype);

template <int width>
void INTERNAL_M_WRITE(unsigned addrspace, llvm::BasicBlock *b, llvm::Value *addr, llvm::Value *data) {
    //this is also straightforward
    llvm::Value   *writeLoc = addr;
    //however, if the incoming 'addr' location is not a pointer, we must
    //first turn it into an addr 

    if( addr->getType()->isPointerTy() == false ) {
        llvm::Type    *ptrTy = llvm::Type::getIntNPtrTy(b->getContext(), width, addrspace);
        writeLoc = new llvm::IntToPtrInst(addr, ptrTy, "", b); 
	} else if( addr->getType() != llvm::Type::getIntNPtrTy(b->getContext(), width, addrspace) ) {
		writeLoc = llvm::CastInst::CreatePointerCast(addr, llvm::Type::getIntNPtrTy(b->getContext(), width, addrspace), "", b);
	}

    bool is_volatile = addrspace != 0;
    llvm::Value   *written = new llvm::StoreInst(data, writeLoc, is_volatile, b);
    TASSERT( written != NULL, "");

    return;
}

template <int width>
void M_WRITE(InstPtr ip, llvm::BasicBlock *b, llvm::Value *addr, llvm::Value *data) {
    return INTERNAL_M_WRITE<width>(ip->get_addr_space(), b, addr, data);
}

template <int width>
void M_WRITE_0(llvm::BasicBlock *b, llvm::Value *addr, llvm::Value *data) {
    return INTERNAL_M_WRITE<width>(0, b, addr, data);
}

void GENERIC_WRITEREG(llvm::BasicBlock *b, std::string regname, llvm::Value *v);
llvm::Value *GENERIC_READREG(llvm::BasicBlock *b, std::string regname);

llvm::Value *F_READ(llvm::BasicBlock *b, std::string flag);

void F_WRITE(llvm::BasicBlock *b, std::string flag, llvm::Value *v);

void F_ZAP(llvm::BasicBlock *b, std::string flag);

void F_SET(llvm::BasicBlock *b, std::string flag);

void F_CLEAR(llvm::BasicBlock *b, std::string flag);


void allocateLocals(llvm::Function *, int);

llvm::BasicBlock *bbFromStrName(std::string, llvm::Function *);

///////////////////////////////////////////////////////////////////////////////
// API usage functions
///////////////////////////////////////////////////////////////////////////////

InstTransResult disInstr(   InstPtr             ip, 
                            llvm::BasicBlock          *&block, 
                            NativeBlockPtr      nb,
                            llvm::Function            *F,
                            NativeFunctionPtr   natF,
                            NativeModulePtr     natM);


llvm::Value *makeCallbackForLocalFunction(llvm::Module *M, VA local_target);

void dataSectionToTypesContents(
        const std::list<DataSection>  &globaldata, 
        DataSection& ds, 
        llvm::Module *M,
        std::vector<llvm::Constant*>&    secContents,
        std::vector<llvm::Type*>& data_section_types,
        bool convert_to_callback);

extern bool ignoreUnsupportedInsts;
