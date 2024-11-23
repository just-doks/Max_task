#ifndef FILEDEFRAGMENTATION_H
#define FILEDEFRAGMENTATION_H

#include "File.h"
#include "Partition.h"

namespace FileManagment
{
    namespace Defragmentation
    {
        // Обнаружение/устранение фрагментации

        // Главная функция дефрагментации - осуществляет проверку
        // на фрагментацию и дефрагментацию указанного файла или каталога, 
        // поскольку, каталоги также, в теории, могут быть фрагментированы. 
        auto defragment_file(File& file) -> uint32_t;
        // Вспомогательный метод при обработке директорий.
        // Проходится по всем кластерам директории, вызывая
        // для каждого кластера метод defragment_dir_cluster().
        auto defragment_dir(File& file) -> uint32_t;
        // Метод, извлекающий из буферизованных кластеров директорий
        // файлы, которые затем передаёт в метод defragment_file().
        auto defragment_dir_cluster(Bytes& dir_cluster, 
            uint32_t cluster_number, Partition& partition) -> uint32_t;
        // Метод, используемый для поиска требуемого свободного пространства
        // для дефрагментации файла.
        auto find_empty_space(uint32_t clusters_number, const Partition& partition) -> uint32_t;

        // Метод, копирующий указанный кластер по указанному адресу.
        auto copy_cluster
            (uint32_t source, uint32_t destination, Partition& partition) -> void;

        // Метод для подсчёта занимаемых файлом кластеров.
        // Универсален для любых типов файлов, поскольку высчитывает
        // кластеры по таблице FAT из контейнера.
        auto count_file_clusters
            (const File& file) -> uint32_t;
        
    }
    namespace Search
    {
        // Метод для получения экземпляра класса из буфферизованной
        // директории по указанному смещению байт. Номер кластера
        // директории указывается, чтобы заполнить информацию
        // о смещении записи файла в разделе, для возможного внесения
        // изменений.
        auto get_file_from_entry(Bytes& dir, uint32_t dir_cluster_number, 
            size_t offset, Partition& partition) -> File;
    }
}

#endif // FILEDEFRAGMENTATION_H