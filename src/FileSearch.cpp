#include <iostream>
#include <cstdlib> // system()

#include "FileSearch.h"

#include "Bytes.h"
#include "Partition.h"

namespace FM = FileManagment;
namespace FS = FileManagment::Search;


FM::File FS::get_root_dir(Partition& p)
{
    File file;
    file.name = "/";
    file.partition = &p;
    file.partition_sn = p.pbr.get_parameters().serial_number;
    file.type = File::ROOT_DIR;
    file.first_cluster = p.pbr.get_parameters().root_dir_cluster;
    file.size = p.pbr.get_parameters().root_dir_size;
    if (p.pbr.get_parameters().fat_type == PBR::FAT12 
        || p.pbr.get_parameters().fat_type == PBR::FAT16)
        file.entry_offset = p.pbr.get_parameters().data_offset;
    if (p.pbr.get_parameters().fat_type == PBR::FAT32)
        file.entry_offset = p.pbr.get_parameters().data_offset
            + p.pbr.get_parameters().cluster_size * (file.first_cluster - 1);
    return file;
}

FM::File FS::search_file(std::string& filename, Bytes& dir, Partition& p)
{
    File file {};
    for (size_t i = 0; i < dir.length(); i += 0x20)
    {
        char ch = dir.get_value<char>(i);
        if (static_cast<unsigned char>(ch) == 0xE5U 
            || ch == '.')
            continue;
        if (ch == 0)
            break;
        file.name = get_entry_name(dir, i);
        if (file.name == filename)
        {
            file.partition = &p;
            file.partition_sn = p.pbr.get_parameters().serial_number;
            file.type = get_file_type(dir, i);
            if (uint16_t f_c_h = dir.get_value<uint16_t>(i + 0x14))
            {
                file.first_cluster += static_cast<uint32_t>(f_c_h) << 16U;
            }
            file.first_cluster += dir.get_value<uint16_t>(i + 0x1A);
            file.size = dir.get_value<uint32_t>(i + 0x1C);
            file.entry_offset = i;
            break;
        }
    }
    return ((file.type != File::NONE) ? file : File{});
}

std::string FS::get_entry_name(Bytes& dir, size_t offset)
{
    std::string name;
    char ch;
    for (uint8_t j = 0; j < 8; ++j)
    {
        ch = dir.get_value<char>(offset + j);
        //std::cout << ch;
        if (ch == 0x20) 
            break;
        name += ch;
    }
    ch = dir.get_value<char>(offset + 0xB);
    if ((ch & 0x10) == 0)
    {
        name += '.';
        for (uint8_t j = 0x8; j <= 0xA; ++j)
        {
            
            ch = dir.get_value<char>(offset + j);
            if (ch == 0x20) 
                break;
            name += ch;
        }
    }
    return name;
}

FM::File::Type FS::get_file_type(Bytes& dir, size_t offset)
{
    File::Type type = File::NONE;
    char ch = dir.get_value<char>(offset + 0xB);
    if (ch == 0x10)
        type = File::DIR;
    if (ch == 0x20)
        type = File::FILE;
    return type;
}

std::string FS::extract_name(const std::string& path)
{
    std::string name;
    for (const char& el : path)
    {
        if (el == '/')
            break;
        name += el;
    }
    return name;
}

void FS::cut_string(std::string& path, char ch)
{
    std::string new_path = "";
    char flag = 1;
    for (auto& el : path)
    {    
        if (flag) 
        {
            if (el != ch) 
                continue;
            else 
            {
                flag = 0;
                continue;
            }
        }
        new_path += el;
    }
    path = new_path;
}