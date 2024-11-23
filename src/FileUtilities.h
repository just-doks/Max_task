#ifndef FILEUTILITIES_H
#define FILEUTILITIES_H

#include <cstdint>

#include "File.h"

namespace FileManagment
{
    // Вывести параметры файла в консоль
    void print_file_info(const File& file);

    // Метод для получения экземпляра класса File. 
    // Используется для поиска файла по заданному пути для дальнейших
    // манипуляций, а именно, дефрагментации.
    auto get_file(std::string& path, Partition& partition) -> File;

    // Открытый метод, запускающий процесс дефрагментации файла.
    // Возвращает количество дефрагментированных файлов.
    auto defragment(File& file) -> uint32_t;
    // Метод, используемый для проверки файла на фрагментацию.
    auto is_file_fragmented(const File& file) -> uint32_t;
    
}

#endif // FILEUTILITIES_H