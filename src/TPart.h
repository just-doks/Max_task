#ifndef TPART_H
#define TPART_H

#include <iostream>
#include <cstddef>

#include "Partition.h"
#include "FileUtilities.h"
#include "../lib/File/src/FileManagment.h"
#include "../lib/File/src/FileDefragmentation.h"


// Наследник класса Partition, созданный с целью использования
// методов предшествующего класса в альтернативных сценариях.
// А именно, можно быстро вывести основную информацию о разделе,
// и скопировать указанный кластер по указанному адресу.
class TPart: public Partition
{
    public:
    TPart(const std::string& path) : Partition(path) {}

    void copy_cluster(uint32_t from, uint32_t to)
    {
        namespace FD = FileManagment::Defragmentation;
        FD::copy_cluster(from, to, *this);
    }

    void print_pbr()
    {
        std::cout << std::hex << std::uppercase << '\n';
        pbr.print_parameters();
        std::cout << std::dec << '\n';
        pbr.print_parameters();
        std::cout << std::endl;
    }

    /*
    uint32_t get_cluster_size()
    {
        return pbr.get_parameters().cluster_size;
    }

    
    void move_cluster(FileManagment::File& file, uint32_t src, uint32_t dest)
    {
        namespace MM = FileManagment::MemoryManagment;
        MM::copy_cluster(file, src, dest);
        MM::move_fat_cell(file, src, dest);
    }
    


    void set_first_cluster(FileManagment::File& file, uint32_t first_cluster)
    {
        namespace MM = FileManagment::MemoryManagment;
        MM::set_entry_first_cluster(file, first_cluster);
    }

    std::vector<uint32_t> get_file_clusters(FileManagment::File& file)
    {
        namespace MM = FileManagment::MemoryManagment;
        return MM::get_file_clusters(file);
    }
    */

    bool fragment_file(FileManagment::File& file, std::vector<uint32_t> new_clusters)
    {
        namespace MM = FileManagment::MemoryManagment;
        std::vector<uint32_t> cluster_list = MM::get_file_clusters(file);

        if (new_clusters.size() != cluster_list.size())
            return false;

        for (size_t i = cluster_list.size() - 1; i > 0; --i)
        {
            
            MM::copy_cluster(file, cluster_list[i], new_clusters[i]);
            MM::move_fat_cell(file, cluster_list[i], new_clusters[i], cluster_list[i - 1U]);
        }
        MM::copy_cluster(file, cluster_list[0], new_clusters[0]);
        MM::move_fat_cell(file, cluster_list[0], new_clusters[0]);

        MM::set_entry_first_cluster(file, new_clusters[0]);

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

        return true;
    }

};

#endif // TPART_H