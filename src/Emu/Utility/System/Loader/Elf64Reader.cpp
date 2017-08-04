// 
// Copyright (c) 2005-2008 Kenichi Watanabe.
// Copyright (c) 2005-2008 Yasuhiro Watari.
// Copyright (c) 2005-2008 Hironori Ichibayashi.
// Copyright (c) 2008-2009 Kazuo Horio.
// Copyright (c) 2009-2015 Naruki Kurata.
// Copyright (c) 2005-2015 Ryota Shioya.
// Copyright (c) 2005-2015 Masahiro Goshima.
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software. If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
// 
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
// 
// 3. This notice may not be removed or altered from any source
// distribution.
// 
// 


#include <pch.h>

#include "Emu/Utility/System/Loader/Elf64Reader.h"

#include <ios>
#include <algorithm>
#include <cstring>
#include <limits>

#include "SysDeps/Endian.h"

using namespace std;
using namespace Onikiri;
using namespace Onikiri::EmulatorUtility;

Elf64Reader::Elf64Reader()
{
    m_sectionNameTable = 0;
    m_bigEndian = false;
}

Elf64Reader::~Elf64Reader()
{
    delete[] m_sectionNameTable;
}

void Elf64Reader::Open(const char *name)
{
    m_file.open(name, ios_base::in | ios_base::binary);
    if (m_file.fail()) {
        stringstream ss;
        ss << "'" << name << "' : cannot open file";
        throw runtime_error(ss.str());
    }

    try {
        ReadELFHeader();
        ReadSectionHeaders();
        ReadProgramHeaders();
        ReadSectionNameTable();
    }
    catch (runtime_error& e) {
        m_file.close();

        stringstream ss;
        ss << "'" << name << "' : " << e.what();
        throw runtime_error(ss.str());
    }
}

void Elf64Reader::ReadELFHeader()
{
    m_file.read((char *)&m_elfHeader, sizeof(m_elfHeader));
    if (m_file.fail())
        throw runtime_error("cannot read ELF header");

    // ELF64かどうかチェック
    static const int CLASS_ELF64 = 2;
    if (!equal(&m_elfHeader.e_ident[0], &m_elfHeader.e_ident[3], "\177ELF") && m_elfHeader.e_ident[4] != CLASS_ELF64)
        throw runtime_error("not a valid ELF file");

    // エンディアンの検出
    static const int DATA_LE = 1;
    static const int DATA_BE = 2;
    switch (m_elfHeader.e_ident[5]) {
    case DATA_LE:
        m_bigEndian = false;
        break;
    case DATA_BE:
        m_bigEndian = true;
        break;
    default:
        throw runtime_error("unknown endian type");
    }

    EndianSpecifiedToHostInPlace(m_elfHeader, m_bigEndian);

    // ヘッダのサイズが想定値に一致しないならエラーにしてしまう手抜き
    if (m_elfHeader.e_ehsize != sizeof(m_elfHeader) ||
        m_elfHeader.e_shentsize != sizeof(Elf_Shdr) ||
        m_elfHeader.e_phentsize != sizeof(Elf_Phdr))
    {
        throw runtime_error("invalid header size");
    }
}

void Elf64Reader::ReadSectionHeaders()
{
    const char* readError = "cannot read section headers";

    // セクションヘッダがない
    if (m_elfHeader.e_shoff == 0)
        return;

    m_file.seekg(static_cast<istream::off_type>(m_elfHeader.e_shoff), ios_base::beg);
    if (m_file.fail())
        throw runtime_error(readError);


    m_elfSectionHeaders.clear();
    for (int i = 0; i < m_elfHeader.e_shnum; i ++) {
        Elf_Shdr sh;

        m_file.read((char*)&sh, sizeof(sh));
        EndianSpecifiedToHostInPlace(sh, m_bigEndian);
        m_elfSectionHeaders.push_back(sh);
    }

    if (m_file.fail())
        throw runtime_error(readError);
}

void Elf64Reader::ReadProgramHeaders()
{
    const char* readError = "cannot read program headers";

    // プログラムヘッダがない
    if (m_elfHeader.e_phoff == 0)
        return;

    m_file.seekg(static_cast<istream::off_type>(m_elfHeader.e_phoff), ios_base::beg);
    if (m_file.fail())
        throw runtime_error(readError);

    m_elfProgramHeaders.clear();
    for (int i = 0; i < m_elfHeader.e_phnum; i ++) {
        Elf_Phdr ph;

        m_file.read((char*)&ph, sizeof(ph));
        EndianSpecifiedToHostInPlace(ph, m_bigEndian);
        m_elfProgramHeaders.push_back(ph);
    }

    if (m_file.fail())
        throw runtime_error(readError);
}


void Elf64Reader::ReadSectionNameTable()
{
    if (m_elfHeader.e_shstrndx >= GetSectionHeaderCount())
        throw runtime_error("no section name table found");

    int shstrndx = m_elfHeader.e_shstrndx;
    int shstrsize = static_cast<int>( GetSectionHeader(shstrndx).sh_size );

    delete[] m_sectionNameTable;
    m_sectionNameTable = new char[ shstrsize ];
    ReadSectionBody(shstrndx, m_sectionNameTable, shstrsize);
}

void Elf64Reader::Close()
{
    m_file.close();
}

void Elf64Reader::ReadSectionBody(int index, char *buf, size_t buf_size) const
{
    const Elf_Shdr &sh = GetSectionHeader(index);
    if (buf_size != sh.sh_size)
        throw runtime_error("not enough buffer");

    ReadRange((size_t)sh.sh_offset, buf, (size_t)sh.sh_size);
}

void Elf64Reader::ReadRange(size_t offset, char *buf, size_t buf_size) const
{
    m_file.seekg(static_cast<istream::off_type>(offset), ios_base::beg);
    m_file.read(buf, static_cast<std::streamsize>(buf_size));

    if (m_file.fail())
        throw runtime_error("read error");
}

const char *Elf64Reader::GetSectionName(int index) const
{
    const Elf_Shdr &sh = GetSectionHeader(index);

    return m_sectionNameTable + sh.sh_name;
}

int Elf64Reader::FindSection(const char *name) const
{
    for (int i = 0; i < GetSectionHeaderCount(); i++) {
        if (strcmp(GetSectionName(i), name) == 0)
            return i;
    }

    return -1;
}

streamsize Elf64Reader::GetImageSize() const
{
    m_file.seekg(0, ios_base::end);
    return m_file.tellg();
}

void Elf64Reader::ReadImage(char *buf, size_t buf_size) const
{
    streamsize size = GetImageSize();
    if ((streamsize)buf_size < size)
        throw runtime_error("read error");

    m_file.seekg(0, ios_base::beg);
    m_file.read(buf, static_cast<std::streamoff>(size));

    if (m_file.fail())
        throw runtime_error("read error");
}


int Elf64Reader::GetClass() const
{
    return m_elfHeader.e_ident[EI_CLASS];
}

int Elf64Reader::GetDataEncoding() const
{
    return m_elfHeader.e_ident[EI_DATA];
}

int Elf64Reader::GetVersion() const
{
    return m_elfHeader.e_ident[EI_VERSION];
}

u16 Elf64Reader::GetMachine() const
{
    return m_elfHeader.e_machine;
}

bool Elf64Reader::IsBigEndian() const
{
    return m_bigEndian;
}

Elf64Reader::Elf_Addr Elf64Reader::GetEntryPoint() const
{
    return m_elfHeader.e_entry;
}

Elf64Reader::Elf_Off Elf64Reader::GetSectionHeaderOffset() const
{
    return m_elfHeader.e_shoff;
}

Elf64Reader::Elf_Off Elf64Reader::GetProgramHeaderOffset() const
{
    return m_elfHeader.e_phoff;
}

int Elf64Reader::GetSectionHeaderCount() const
{
    // 元々Elf32_Half = 16bitなので
    return (int)m_elfSectionHeaders.size();
}

int Elf64Reader::GetProgramHeaderCount() const
{
    return (int)m_elfProgramHeaders.size();
}

const Elf64Reader::Elf_Shdr &Elf64Reader::GetSectionHeader(int index) const
{
    return m_elfSectionHeaders[index];
}

const Elf64Reader::Elf_Phdr &Elf64Reader::GetProgramHeader(int index) const
{
    return m_elfProgramHeaders[index];
}
