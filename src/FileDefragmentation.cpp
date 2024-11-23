
#include "FileDefragmentation.h"
#include "FileUtilities.h"
#include "FileManagment.h"
#include "FileSearch.h"

#include "PBR.h"

#include <iostream>
#include <cassert>

namespace FM = FileManagment;
namespace FD = FM::Defragmentation;
namespace FS = FM::Search;


uint32_t FD::defragment_dir(File& file)
{
    if (file.type != File::Type::DIR && file.type != File::Type::ROOT_DIR)
        return 0;
    auto fat_type = file.partition->pbr.get_parameters().fat_type;
    assert((fat_type == PBR::FAT12 || fat_type == PBR::FAT16
        || fat_type == PBR::FAT32) && "Invalid partition type.");

    uint32_t counter = 0;
    Bytes buff;
    auto cluster_size = file.partition->pbr.get_parameters().cluster_size;
    auto data_offset = file.partition->pbr.get_parameters().data_offset;
    auto root_dir_size = file.partition->pbr.get_parameters().root_dir_size;

    if (file.type == File::Type::ROOT_DIR && fat_type != PBR::FAT32)
    {
        buff.resize(root_dir_size);
        file.partition->drive.seekg(data_offset, file.partition->drive.beg);
        file.partition->drive.read(buff, root_dir_size);
        counter = defragment_dir_cluster(buff, 
            file.partition->pbr.get_parameters().root_dir_cluster,
            *file.partition);
    }
    else
    {
        uint8_t shift = 1U;
        uint64_t offset;
        if (fat_type == PBR::FAT16 || fat_type == PBR::FAT12)
        {
            data_offset += file.partition->pbr.get_parameters().root_dir_size;
            shift = 2U;
        }
        buff.resize(cluster_size);
        uint32_t current_cluster;
        uint32_t next_cluster = file.first_cluster;
        do
        {
            current_cluster = next_cluster;
            next_cluster = file.partition->FAT.get_value<uint32_t>
                (next_cluster * 2, Bytes::WORD);

            offset = data_offset + (current_cluster - shift) * cluster_size;
            file.partition->drive.seekg(offset, file.partition->drive.beg);
            file.partition->drive.read(buff, cluster_size);
            counter += defragment_dir_cluster(buff, current_cluster,
                *file.partition);
            
        } while (next_cluster != 0xFFFFU);
    }
    return counter;
}


uint32_t FD::defragment_dir_cluster(Bytes& dir_cluster, 
    uint32_t cluster_number, Partition& p)
{
    uint32_t counter = 0;
    File file {};
    file.partition = &p;
    file.partition_sn = p.pbr.get_parameters().serial_number;
    unsigned char ch;
    for (int32_t i = 0; i < dir_cluster.length(); i += 0x20U)
    {
        ch = dir_cluster.get_value<unsigned char>(i, Bytes::BYTE);
        if (ch == 0)
            break;
        if (ch == 0xE5U || ch == '.')
            continue;
        file = FS::get_file_from_entry(dir_cluster, cluster_number, i, *file.partition);
        if (file.type != File::Type::NONE)
            counter += defragment_file(file);
    }
    return counter;
}


uint32_t FD::defragment_file(File& file)
{
    if (file.partition_sn != file.partition->pbr.get_parameters().serial_number
        || file.type == File::Type::NONE || file.type == File::Type::ROOT_DIR)
    {
        //std::cout << "Некорректный файл\n";
        return 0;
    }
    if (!is_file_fragmented(file))
        return 0;
    uint32_t clusters_per_file = count_file_clusters(file); 
    uint32_t first_free_cluster = find_empty_space(clusters_per_file, *file.partition);
    if (first_free_cluster == 0)
    {
        //std::cout << "Недостаточно свободного места для дефрагментации.\n";
        return 0;
    }

    // Копирование кластеров данных файла в новое пространство.
    /*
    uint32_t src_cluster = file.first_cluster;
    uint32_t dest_cluster = first_free_cluster;
    for (uint32_t i = 0; i < clusters_per_file; ++i)
    {
        copy_cluster(src_cluster, dest_cluster + i, *file.partition);
        src_cluster = file.partition->FAT.get_value<uint32_t>
            (src_cluster * 2, Bytes::WORD);

        //++dest_cluster;
        if ((i + 1U) == clusters_per_file)
        {
            file.partition->FAT.insert<uint32_t>
                (0xFFFFU, (dest_cluster + i - 1U) * 2, Bytes::WORD);
            break;
        }
        file.partition->FAT.insert<uint32_t>
            (dest_cluster, (dest_cluster + i - 1U) * 2, Bytes::WORD);
    }
    */
   
    // Копирование кластеров файла в новое пространство.
    // Заполнение новой цепочки кластеров в таблице FAT.
    // Стирание старых ячеек таблицы FAT.
    uint32_t src_cluster = file.first_cluster;
    uint32_t dest_cluster = first_free_cluster;
    for (uint32_t i = 0; i < clusters_per_file; ++i)
    {
        // Копирование кластера
        copy_cluster(src_cluster, dest_cluster + i, *file.partition);

        // Сохранение адреса старой ячейки
        uint32_t prev_cluster = src_cluster;

        // Запись адреса следующего кластера в цепочке
        src_cluster = file.partition->FAT.get_value<uint32_t>
            (src_cluster * 2, Bytes::WORD);
    
        // Стирание старой ячейки FAT
        file.partition->FAT.insert<uint32_t>
            (0U, (prev_cluster) * 2, Bytes::WORD);

        // Запись нового адреса в таблицу FAT
        // Если кластер последний - записывается код последнего кластера.
        if ((i + 1U) == clusters_per_file)
        {
            file.partition->FAT.insert<uint32_t>
                (0xFFFFU, (dest_cluster + i) * 2, Bytes::WORD);
            break;
        } else        
            file.partition->FAT.insert<uint32_t>
                (dest_cluster + i + 1U, (dest_cluster + i) * 2, Bytes::WORD);
    }      
    
    // Стирание старых блоков файла в таблице FAT.
    /*
    uint32_t current_src_cluster = file.first_cluster;
    uint32_t previous_src_cluster;
    do {
        previous_src_cluster = current_src_cluster;
        current_src_cluster = file.partition->FAT.get_value<uint32_t>
            (current_src_cluster * 2, Bytes::WORD);
        file.partition->FAT.insert<uint32_t>
            (0U, previous_src_cluster * 2, Bytes::WORD);
    } while (current_src_cluster != 0xFFFF);
    */

    // Запись таблиц FAT из буфера на накопитель.
    uint64_t fat_offset = file.partition->pbr.get_parameters().fat_offset;
    uint64_t fat_size = file.partition->pbr.get_parameters().fat_size;
    uint64_t cur_fat_offset;
    for (uint8_t i = 0; i < file.partition->pbr.get_parameters().fat_number; ++i)
    {
        cur_fat_offset = fat_offset + fat_size * i;
        file.partition->drive.seekp(cur_fat_offset, file.partition->drive.beg);
        file.partition->drive.write(file.partition->FAT, fat_size);
    }
    
    // Запись номера нового первого кластера файла в запись файла.
    Bytes buff(2);
    buff.insert<uint32_t>(first_free_cluster, 0, Bytes::WORD);
    file.partition->drive.seekp(file.entry_offset + 0x1AU, file.partition->drive.beg);
    file.partition->drive.write(buff, 2);

    return 1;
}

void FD::copy_cluster(uint32_t source, uint32_t destination, Partition& p)
{
    PBR::FAT_Type fat_type = p.pbr.get_parameters().fat_type;
    if (fat_type == PBR::NONE)
    {
        return;
    }
    uint8_t shift;
    uint64_t src_offset;
    uint64_t dest_offset;

    uint32_t cluster_size = p.pbr.get_parameters().cluster_size;
    uint64_t data_offset = p.pbr.get_parameters().data_offset;

    Bytes buff;

    if (fat_type == PBR::FAT12 || fat_type == PBR::FAT16)
    {
        shift = 2U;
        src_offset = data_offset + p.pbr.get_parameters().root_dir_size 
            + cluster_size * (source - shift);
        dest_offset = data_offset + p.pbr.get_parameters().root_dir_size 
            + cluster_size * (destination - shift);
        buff.resize(cluster_size);
        p.drive.seekg(src_offset, p.drive.beg);
        p.drive.read(buff, cluster_size);
        p.drive.seekp(dest_offset, p.drive.beg);
        p.drive.write(buff, cluster_size);
    }
    if (fat_type == PBR::FAT32)
    {
        shift = 1U;
        src_offset = data_offset
            + cluster_size * (source - shift);
        dest_offset = data_offset
            + cluster_size * (destination - shift);
        
        buff.resize(cluster_size);
        p.drive.seekg(src_offset, p.drive.beg);
        p.drive.read(buff, cluster_size);
        p.drive.seekp(dest_offset, p.drive.beg);
        p.drive.write(buff, cluster_size);
    }

    FileManagment::MemoryManagment::clear_cluster(p, source);
}

// Принимает на вход количество кластеров, необходимых файлу,
// и возвращает номер первого кластера, в который можно производить запись.
uint32_t FD::find_empty_space(uint32_t clusters_number, const Partition& p)
{
    uint32_t fat_block;
    uint32_t counter = 0;
    uint32_t first_cluster = 0;
    uint32_t block_size = 2U;
    uint32_t last_data_cluster = 0xFFEFU;
    // Первые два блока (0, 1) зарезервированы. Третий (2) не используется.
    for (uint32_t i = 3; i < p.FAT.length(); ++i)
    {
        if (counter == clusters_number) break;

        if ((i / block_size) > last_data_cluster)
        {
            first_cluster = 0;
            break;
        } 
        fat_block = p.FAT.get_value<uint32_t>(i * block_size, Bytes::WORD);
        if (fat_block == 0)
        {
            if (counter == 0) 
                first_cluster = i;    
            ++counter;
            continue;
        }
        if (counter > 0)
        {
            counter = 0;
            first_cluster = 0;
        }
    }
    return first_cluster;
}

uint32_t FD::count_file_clusters(const File& file)
{   
    if (file.first_cluster == 0)
        return 0U;
    uint32_t counter = 0;
    uint32_t current_cluster = file.first_cluster;
    do
    {
        ++counter;
        current_cluster = file.partition->FAT.get_value<uint32_t>
            (current_cluster * 2, Bytes::WORD);
    } while (current_cluster != 0xFFFF);
    return counter;
}

FM::File FS::get_file_from_entry(Bytes& dir, uint32_t dir_cluster_number, 
    size_t offset, Partition& p)
{
    File file = {};
    unsigned char ch = dir.get_value<unsigned char>(offset);
    if (ch == 0xE5U || ch == '.' || ch == 0)
        return file;
    file.partition = &p;
    file.type = get_file_type(dir, offset);
    if (file.type != File::NONE)
    {
        auto data_offset = p.pbr.get_parameters().data_offset;
        auto root_dir_size = p.pbr.get_parameters().root_dir_size;
        auto cluster_size = p.pbr.get_parameters().cluster_size;
        auto fat_type = p.pbr.get_parameters().fat_type;

        file.name = get_entry_name(dir, offset);
        file.partition_sn = p.pbr.get_parameters().serial_number;
        if (uint16_t f_c_h = dir.get_value<uint16_t>(offset + 0x14))
        {
            file.first_cluster += static_cast<uint32_t>(f_c_h) << 16U;
        }
        file.first_cluster += dir.get_value<uint16_t>(offset + 0x1A);
        file.size = dir.get_value<uint32_t>(offset + 0x1C);
        if (fat_type == PBR::FAT12 || fat_type == PBR::FAT16)
            file.entry_offset = data_offset + root_dir_size
                + (cluster_size * (dir_cluster_number - 2U)) + offset;
        if (fat_type == PBR::FAT32)
            file.entry_offset = data_offset
                + (cluster_size * (dir_cluster_number - 1U)) + offset;
    }
    return file;
}