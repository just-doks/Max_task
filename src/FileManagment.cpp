#include "FileManagment.h"
#include "Bytes.h"
#include "Partition.h"

namespace MM = FileManagment::MemoryManagment;

std::vector<uint32_t> MM::get_file_clusters(File& file)
{
    //std::cout << "get_file_clusters()\n";
    std::vector<uint32_t> list;
    uint32_t current_cluster = file.first_cluster;
    do {
        list.push_back(current_cluster);
        //std::cout << "current_cluster = " << current_cluster << '\n';
        current_cluster = file.partition->FAT.get_value<uint32_t>
            ( current_cluster * 2U, Bytes::WORD );
    } while (current_cluster != 0xFFFFU);

    return list;
}

bool MM::set_entry_first_cluster (File& file, uint32_t cluster)
{
    if (cluster == file.first_cluster)
        return true;
        
    if (!file.partition->is_open())
        return false;
    // Запись номера нового первого кластера файла в запись файла.
    Bytes buff(2);
    buff.insert<uint32_t>(cluster, 0, Bytes::WORD);
    file.partition->drive.seekp(file.entry_offset + 0x1AU, file.partition->drive.beg);
    file.partition->drive.write(buff, 2);

    file.first_cluster = cluster;
    return true;
}

bool MM::move_fat_cell (File& file, uint32_t curr_cell, uint32_t new_cell, uint32_t prev_cell) // ???
{
    if (new_cell == curr_cell)
        return true;
        
    Bytes& FAT = file.partition->FAT;

    const uint32_t last_data_cluster = 0xFFEFU;

    if ((new_cell > FAT.length()) || (new_cell > last_data_cluster))
        return false;

    uint32_t cell_value = FAT.get_value<uint32_t>(curr_cell * 2U, Bytes::WORD);
    if (cell_value == 0U)
        return false;
    
    // Если предыдущей ячейки нет (первый кластер в цепочке)
    if (prev_cell <= 0)
    {
        // Записываем значение старой ячейки в новую 
        FAT.insert<uint32_t>(cell_value, new_cell * 2, Bytes::WORD);
        // Зануляем старую ячейку
        FAT.insert<uint32_t>(0U, curr_cell * 2, Bytes::WORD);
    } else // Если кластер не первый
    {
        // Записываем в предыдущую ячейку значение адреса новой ячейки
        FAT.insert<uint32_t>(new_cell, prev_cell * 2, Bytes::WORD);
        // Записываем в новую ячейку значение из старой ячейки
        FAT.insert<uint32_t>(cell_value, new_cell * 2, Bytes::WORD);
        // Зануляем старую ячейку
        FAT.insert<uint32_t>(0U, curr_cell * 2, Bytes::WORD);  
    }

    return true;
}

bool MM::copy_cluster(File& file, uint32_t src, uint32_t dest)
{
    if (src == dest)
        return true;
        
    Partition& p = *file.partition;
    PBR::FAT_Type fat_type = p.pbr.get_parameters().fat_type;
    if (fat_type == PBR::NONE)
    {
        return false;
    }
    uint8_t shift;
    uint64_t src_offset;
    uint64_t dest_offset;

    uint32_t cluster_size = p.pbr.get_parameters().cluster_size;
    uint64_t data_offset = p.pbr.get_parameters().data_offset;

    Bytes buff;

    if (fat_type == PBR::FAT12 || fat_type == PBR::FAT16)
    {
        shift = 2U; // Вычитаем позицию кластера 
        // (чтобы смещение байт указывало на начало кластера)
        // и позицию корневой директории,
        // поскольку корневая директория имеет свой размер.
        src_offset = data_offset + p.pbr.get_parameters().root_dir_size 
            + cluster_size * (src - shift);
        dest_offset = data_offset + p.pbr.get_parameters().root_dir_size 
            + cluster_size * (dest - shift);
        buff.resize(cluster_size);
        p.drive.seekg(src_offset, p.drive.beg);
        p.drive.read(buff, cluster_size);
        p.drive.seekp(dest_offset, p.drive.beg);
        p.drive.write(buff, cluster_size);
    }
    if (fat_type == PBR::FAT32)
    {
        shift = 1U; // Вычитаем только позицию кластера.
        src_offset = data_offset
            + cluster_size * (src - shift);
        dest_offset = data_offset
            + cluster_size * (dest - shift);
        
        buff.resize(cluster_size);
        p.drive.seekg(src_offset, p.drive.beg);
        p.drive.read(buff, cluster_size);
        p.drive.seekp(dest_offset, p.drive.beg);
        p.drive.write(buff, cluster_size);
    }

    clear_cluster(*file.partition, src);

    return true;
}

bool MM::clear_cluster(Partition& p, uint32_t cluster)
{
    uint32_t cluster_size = p.pbr.get_parameters().cluster_size;

    uint32_t offset = p.pbr.get_parameters().data_offset 
        + p.pbr.get_parameters().root_dir_size
        + p.pbr.get_parameters().cluster_size * (cluster - 2U);
    
    char* zeros = new char[cluster_size]();

    p.drive.seekp(offset, p.drive.beg);
    p.drive.write(zeros, cluster_size);
    delete[] zeros;
    return true;
}

uint32_t MM::count_file_clusters (const File& file)
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