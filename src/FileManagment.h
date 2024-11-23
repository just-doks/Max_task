#ifndef FM_H
#define FM_H

#include "Partition.h"
#include "File.h"

#include <cstdint>
#include <vector>

namespace FileManagment
{
    namespace MemoryManagment
    {
        // Метод, копирующий указанный кластер по указанному адресу.
        auto copy_cluster (File& file, uint32_t src, uint32_t dest) -> bool;

        auto clear_cluster(Partition& partition, uint32_t cluster) -> bool;

        // Метод для подсчёта занимаемых файлом кластеров.
        // Универсален для любых типов файлов, поскольку высчитывает
        // кластеры по таблице FAT из контейнера.
        auto count_file_clusters (const File& file) -> uint32_t;

        auto get_file_clusters (File& file) -> std::vector<uint32_t>;

        auto set_entry_first_cluster (File& file, uint32_t cluster) -> bool;

        auto move_fat_cell (File& file, uint32_t current_cell, uint32_t new_cell,
            uint32_t prev_cell = 0U) -> bool;

        //auto get_file_cluster_list(File& file, Partition& partition)
        //     -> std::vector<uint32_t>;
    }
}

#endif // FM_H