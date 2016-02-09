#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <elf.h>
#include <unistd.h>
#include <sys/mman.h>

#include <cassert>
#include <cstdio>
#include <cstring>

#include <iostream>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("usage: %s <elf-binary>\n", argv[0]);
    return 1;
  }
  FILE *pyfile = fopen(argv[1], "r");
  if (pyfile == nullptr) {
    return 1;
  }
  if (fseek(pyfile, 0, SEEK_END)) {
    fclose(pyfile);
    return 1;
  }
  long pyfile_size = ftell(pyfile);

  void *pybytes = mmap(nullptr, (size_t)pyfile_size, PROT_READ, MAP_PRIVATE,
                       fileno(pyfile), 0);
  if (pybytes == nullptr) {
    fclose(pyfile);
    perror("mmap()");
    return 1;
  }
  fclose(pyfile);
  printf("%p\n", pybytes);

  const unsigned char expected_magic[] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};
  Elf64_Ehdr elf_hdr;
  memmove(&elf_hdr, pybytes, sizeof(elf_hdr));
  if (memcmp(elf_hdr.e_ident, expected_magic, sizeof(expected_magic)) != 0) {
    std::cerr << "Target is not an ELF executable\n";
    return 1;
  }
  if (elf_hdr.e_ident[EI_CLASS] != ELFCLASS64) {
    std::cerr << "Sorry, only ELF-64 is supported.\n";
    return 1;
  }
#if 0
  if (elf_hdr.e_ident[EI_OSABI] != ELFOSABI_LINUX) {
    std::cerr << "Sorry, only ELFOSABI_LINUX is supported.\n";
    return 1;
  }
#endif
  if (elf_hdr.e_machine != EM_X86_64) {
    std::cerr << "Sorry, only x86-64 is supported.\n";
    return 1;
  }

  printf("file size: %zd\n", pyfile_size);
  printf("program header offset: %zd\n", elf_hdr.e_phoff);
  printf("program header num: %d\n", elf_hdr.e_phnum);
  printf("section header offset: %zd\n", elf_hdr.e_shoff);
  printf("section header num: %d\n", elf_hdr.e_shnum);
  printf("section header string table: %d\n", elf_hdr.e_shstrndx);

  size_t string_offset = elf_hdr.e_shstrndx;
  printf("string offset at %zd\n", string_offset);
  printf("\n");

  char *cbytes = (char *)pybytes;
  for (uint16_t i = 0; i < elf_hdr.e_phnum; i++) {
    size_t offset = elf_hdr.e_phoff + i * elf_hdr.e_phentsize;
    Elf64_Phdr phdr;
    memmove(&phdr, pybytes + offset, sizeof(phdr));
    printf("PROGRAM HEADER %d, offset = %zd\n", i, offset);
    printf("========================\n");
    printf("p_type = ");
    switch (phdr.p_type) {
      case PT_NULL:
        puts("PT_NULL");
        break;
      case PT_LOAD:
        puts("PT_LOAD");
        break;
      case PT_DYNAMIC:
        puts("PT_DYNAMIC");
        break;
      case PT_INTERP:
        puts("PT_INTERP");
        break;
      case PT_NOTE:
        puts("PT_NOTE");
        break;
      case PT_SHLIB:
        puts("PT_SHLIB");
        break;
      case PT_PHDR:
        puts("PT_PHDR");
        break;
      case PT_LOPROC:
        puts("PT_LOPROC");
        break;
      case PT_HIPROC:
        puts("PT_HIPROC");
        break;
      case PT_GNU_STACK:
        puts("PT_GNU_STACK");
        break;
      default:
        printf("UNKNOWN/%d\n", phdr.p_type);
        break;
    }
    printf("p_offset = %zd\n", phdr.p_offset);
    printf("p_vaddr = %zd\n", phdr.p_vaddr);
    printf("p_paddr = %zd\n", phdr.p_paddr);
    printf("p_filesz = %zd\n", phdr.p_filesz);
    printf("p_memsz = %zd\n", phdr.p_memsz);
    printf("p_flags = %d\n", phdr.p_flags);
    printf("p_align = %lu\n", phdr.p_align);
    printf("\n");
  }

  size_t dynstr_off = 0;
  size_t dynsym_off = 0;
  size_t dynsym_sz = 0;

  for (uint16_t i = 0; i < elf_hdr.e_shnum; i++) {
    size_t offset = elf_hdr.e_shoff + i * elf_hdr.e_shentsize;
    Elf64_Shdr shdr;
    memmove(&shdr, pybytes + offset, sizeof(shdr));
    switch (shdr.sh_type) {
      case SHT_SYMTAB:
      case SHT_STRTAB:
        // TODO: have to handle multiple string tables better
        if (!dynstr_off) {
          printf("found string table at %zd\n", shdr.sh_offset);
          dynstr_off = shdr.sh_offset;
        }
        break;
      case SHT_DYNSYM:
        dynsym_off = shdr.sh_offset;
        dynsym_sz = shdr.sh_size;
        printf("found dynsym table at %zd, size %zd\n", shdr.sh_offset,
               shdr.sh_size);
        break;
      default:
        break;
    }
  }
  assert(dynstr_off);
  assert(dynsym_off);
  printf("final value for dynstr_off = %zd\n", dynstr_off);
  printf("final value for dynsym_off = %zd\n", dynsym_off);
  printf("final value for dynsym_sz = %zd\n", dynsym_sz);

  for (size_t j = 0; j * sizeof(Elf64_Sym) < dynsym_sz; j++) {
    Elf64_Sym sym;
    size_t absoffset = dynsym_off + j * sizeof(Elf64_Sym);
    memmove(&sym, cbytes + absoffset, sizeof(sym));
    printf("SYMBOL TABLE ENTRY %zd\n", j);
    printf("st_name = %d", sym.st_name);
    if (sym.st_name != 0) {
      printf(" (%s)", cbytes + dynstr_off + sym.st_name);
    }
    printf("\n");
    printf("st_info = %d\n", sym.st_info);
    printf("st_other = %d\n", sym.st_other);
    printf("st_shndx = %d\n", sym.st_shndx);
    printf("st_value = %p\n", (void *)sym.st_value);
    printf("st_size = %zd\n", sym.st_size);
    printf("\n");
  }
  printf("\n");
  return 0;
}
