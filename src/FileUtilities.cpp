
#include "FileUtilities.h"
#include "FileManagment.h"
#include "FileSearch.h"
#include "FileDefragmentation.h"

#include <iostream>

namespace FM = FileManagment;
namespace MM = FM::MemoryManagment;
namespace FS = FM::Search;
namespace FD = FM::Defragmentation;

void FM::print_file_info(const File& file)
{
    std::cout << "Имя: ";
    if (file.name.length() == 0) 
        std::cout << "-\n";
    else 
        std::cout << file.name << '\n';
    std::cout << "Тип: ";
    switch(file.type)
    {
        case  File::DIR:      std::cout << "директория\n";          break;
        case File::FILE:      std::cout << "файл\n";                break;
        case File::ROOT_DIR:  std::cout << "корневая директория\n"; break;
        case File::NONE:      std::cout << "неизвестно\n";          return;
    }

    if (file.type == File::FILE)
        std::cout << "Размер файла: " << file.size << " байт\n";
    std::cout << "Номер первого кластера: " << file.first_cluster << '\n';
    std::cout << "Количество кластеров: "
              << MM::count_file_clusters(file) << '\n';
    std::cout << "Степень фрагментации: ";
    if (uint32_t fragments = FM::is_file_fragmented(file))
    {
        std::cout << fragments << " фрагментов\n";
    }
    else
        std::cout << " не фрагментирован\n";    
}

FM::File FM::get_file(std::string& path, Partition& p)
{
    using namespace FS;

    if (path == "/")
    {
        return get_root_dir(p);
    }
    std::string filename;

    File file {};
    
    uint32_t current_cluster = p.pbr.get_parameters().root_dir_cluster;
    uint32_t root_dir_size   = p.pbr.get_parameters().root_dir_size;
    uint32_t dir_size        = root_dir_size;
    uint32_t cluster_size    = p.pbr.get_parameters().cluster_size;
    uint64_t data_offset     = p.pbr.get_parameters().data_offset;
    uint64_t cluster_offset;
    uint32_t shift = 1U; // сдвиг из-за нулевого блока таблицы FAT

    Bytes dir_cluster(dir_size);

    cut_string(path, '/');
    
    while (path != "")
    {
        filename = extract_name(path);
        do
        {
            cluster_offset = data_offset
                + (current_cluster - shift) * cluster_size;
            
            p.drive.seekg(cluster_offset, p.drive.beg);
            p.drive.read(dir_cluster, dir_size);

            file = search_file(filename, dir_cluster, p);

            if (dir_size != cluster_size)
            {
                data_offset += root_dir_size;
                dir_size = cluster_size;
                shift = 2U; // прибавляем кластер корневой директории
                dir_cluster.resize(dir_size);
            }
            if (file.type != File::NONE)
            {
                file.entry_offset += cluster_offset;
                break;
            }
            current_cluster = p.FAT.get_value<uint32_t>
                (current_cluster * 2, Bytes::WORD);

        } while (current_cluster != 0xFFFF);
        
        cut_string(path, '/');

        if (file.type != File::DIR && path.length() != 0)
        {
            file = File();
            break;
        }
        if (file.type == File::DIR)
        {
            current_cluster = file.first_cluster;
        }
    }
    return ((file.type != File::NONE) ? file : File{});
}

uint32_t FM::defragment(File& file)
{
    if (file.type == File::Type::NONE)
        return 0;
    uint32_t defragmented_files;
    if (file.type == File::Type::FILE)
        defragmented_files = FD::defragment_file(file);

    if (file.type == File::Type::DIR || file.type == File::Type::ROOT_DIR)
        defragmented_files = FD::defragment_dir(file);

    return defragmented_files;
}

uint32_t FM::is_file_fragmented(const File& file)
{
    uint32_t fragments = 0;
    uint32_t current_cluster = file.first_cluster;
    uint32_t previous_cluster;
    do
    {
        previous_cluster = current_cluster;
        current_cluster = file.partition->FAT.get_value<uint32_t>
            ( current_cluster * 2U, Bytes::WORD );
        if ((current_cluster != 0xFFFFU)
            && (current_cluster - previous_cluster) != 1)
        {
            ++fragments;
        }

    } while (current_cluster != 0xFFFFU);

    return ((fragments > 0) ? ++fragments : 0);
}