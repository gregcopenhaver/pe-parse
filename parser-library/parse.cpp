#include <list>
#include "parse.h"
#include "nt-headers.h"

using namespace std;
using namespace boost;

struct section {
  string          sectionName;
  RVA             sectionBase;
  bounded_buffer  sectionData;
};

struct reloc {
  RVA shiftedAddr;
  RVA shiftedTo;
};

struct parsed_pe_internal {
  list<section>   secs;
};

list<section> getSections(bounded_buffer *file) {
  list<section> sections;

  return sections;
}

bool readOptionalHeader(bounded_buffer *b, optional_header_32 &header) {
#define READ_WORD(x) \
  if(readWord(b, _offset(optional_header_32, x), header.x) == false) { \
    return false; \
  } 
#define READ_DWORD(x) \
  if(readDword(b, _offset(optional_header_32, x), header.x) == false) { \
    return false; \
  } 
#define READ_BYTE(x) \
  if(readByte(b, _offset(optional_header_32, x), header.x) == false) { \
    return false; \
  }

  READ_WORD(Magic);

  if(header.Magic != NT_OPTIONAL_32_MAGIC) {
    return false;
  }

  READ_BYTE(MajorLinkerVersion);
  READ_BYTE(MinorLinkerVersion);
  READ_DWORD(SizeOfCode);
  READ_DWORD(SizeOfInitializedData);
  READ_DWORD(SizeOfUninitializedData);
  READ_DWORD(AddressOfEntryPoint);
  READ_DWORD(BaseOfCode);
  READ_DWORD(BaseOfData);
  READ_DWORD(ImageBase);
  READ_DWORD(SectionAlignment);
  READ_DWORD(FileAlignment);
  READ_WORD(MajorOperatingSystemVersion);
  READ_WORD(MinorOperatingSystemVersion);
  READ_WORD(MajorImageVersion);
  READ_WORD(MinorImageVersion);
  READ_WORD(MajorSubsystemVersion);
  READ_WORD(MinorSubsystemVersion);
  READ_DWORD(Win32VersionValue);
  READ_DWORD(SizeOfImage);
  READ_DWORD(SizeOfHeaders);
  READ_DWORD(CheckSum);
  READ_WORD(Subsystem);
  READ_WORD(DllCharacteristics);
  READ_DWORD(SizeOfStackReserve);
  READ_DWORD(SizeOfStackCommit);
  READ_DWORD(SizeOfHeapReserve);
  READ_DWORD(SizeOfHeapCommit);
  READ_DWORD(LoaderFlags);
  READ_DWORD(NumberOfRvaAndSizes);

#undef READ_WORD
#undef READ_DWORD
#undef READ_BYTE
  return true;
}

bool readFileHeader(bounded_buffer *b, file_header &header) {
 
#define READ_WORD(x) \
  if(readWord(b, _offset(file_header, x), header.x) == false) { \
    return false; \
  } 
#define READ_DWORD(x) \
  if(readDword(b, _offset(file_header, x), header.x) == false) { \
    return false; \
  } 

  READ_WORD(Machine);
  READ_WORD(NumberOfSections);
  READ_DWORD(TimeDateStamp);
  READ_DWORD(PointerToSymbolTable);
  READ_DWORD(NumberOfSymbols);
  READ_WORD(SizeOfOptionalHeader);
  READ_WORD(Characteristics);

#undef READ_DWORD
#undef READ_WORD
  return true;
}

bool readNtHeader(bounded_buffer *b, nt_header_32 &header) {
  if(b == NULL) {
    return false;
  }

  ::uint32_t  pe_magic;
  ::uint32_t  curOffset =0;
  if(readDword(b, curOffset, pe_magic) == false || pe_magic != NT_MAGIC) {
    return false;
  }

  header.Signature = pe_magic;
  bounded_buffer  *fhb = 
    splitBuffer(b, _offset(nt_header_32, FileHeader), b->bufLen);

  if(readFileHeader(fhb, header.FileHeader) == false) {
    return false;
  }

  bounded_buffer *ohb = 
    splitBuffer(b, _offset(nt_header_32, OptionalHeader), b->bufLen);

  if(readOptionalHeader(ohb, header.OptionalHeader) == false) {
    return false;
  }

  return false;
}

bool getHeader(bounded_buffer *file) {
  pe_header p;

  if(file == NULL) {
    return false;
  }

  //start by reading MZ
  ::uint16_t  tmp = 0;
  ::uint32_t  curOffset = 0;
  readWord(file, curOffset, tmp);
  if(tmp != MZ_MAGIC) {
    return false;
  }

  //read the offset to the NT headers
  ::uint32_t  offset;
  if(readDword(file, _offset(dos_header, e_lfanew), offset) == false) {
    return false;
  }
  curOffset += offset; 

  //now, we can read out the fields of the NT headers
  nt_header_32 nt;
  if(readNtHeader(splitBuffer(file, curOffset, file->bufLen), nt) == false) {
    return false;
  }

  return true;
}

parsed_pe *ParsePEFromFile(const char *filePath) {
  //first, create a new parsed_pe structure
  parsed_pe *p = new parsed_pe();

  if(p == NULL) {
    return NULL;
  }

  //make a new buffer object to hold just our file data 
  p->fileBuffer = readFileToFileBuffer(filePath);

  if(p->fileBuffer == NULL) {
    delete p;
    return NULL;
  }

  p->internal = new parsed_pe_internal();

  if(p->internal == NULL) {
    deleteBuffer(p->fileBuffer);
    delete p;
    return NULL;
  }

  //now, we need to do some actual PE parsing and file carving.

  //get header information
  if(getHeader(p->fileBuffer) == false) {
    deleteBuffer(p->fileBuffer);
    delete p;
    return NULL;
  }

  //get the raw data of each section
  p->internal->secs = getSections(p->fileBuffer);

  //get exports

  //get relocations

  return p;
}

void DestructParsedPE(parsed_pe *p) {

  delete p;
  return;
}

//iterate over the imports by RVA and string
void IterImpRVAString(parsed_pe *pe, iterRVAStr cb, void *cbd) {

  return;
}

//iterate over relocations in the PE file
void IterRelocs(parsed_pe *pe, iterReloc cb, void *cbd) {

  return;
}

//iterate over the exports by RVA
void IterExpRVA(parsed_pe *pe, iterRVA cb, void *cbd) {

  return;
}

//iterate over sections
void IterSec(parsed_pe *pe, iterSec cb, void *cbd) {
  parsed_pe_internal  *pint = pe->internal;

  for(list<section>::iterator sit = pint->secs.begin(), e = pint->secs.end();
      sit != e;
      ++sit)
  {
    section s = *sit;
    cb(cbd, s.sectionBase, s.sectionName, &s.sectionData);
  }

  return;
}
