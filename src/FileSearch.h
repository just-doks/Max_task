#ifndef FILESEARCH_H
#define FILESEARCH_H

#include "File.h"
#include "Partition.h"
#include <string>


namespace FileManagment
{
    namespace Search
    {
        // Поиск файла:

        // Метод возвращает экземпляр, заполненный данными по корневой
        // директории. 
        auto get_root_dir(Partition& partition) -> File;
        // Метод ищет файл по указанному имени в кластере буферизованной
        // директории. При нахождении, возвращает заполненный экземпляр
        // класса File с данными файла.
        auto search_file(std::string& filename, 
            Bytes& dir_cluster, Partition& p) -> File;

        // Вспомогательный метод для извлечения имени файла из байт
        // в буфферизованной директории. Имя затем сравнивается с искомым.
        auto get_entry_name(Bytes& dir, size_t offset) -> std::string;
        // Вспомогательный метод, извлекающий из байт буфферизованной 
        // директории информацию о типе файла (файл, директория, неизвестное).
        auto get_file_type(Bytes& dir, size_t offset) -> File::Type;
        

        // Вспомогательные методы для поиска файла.
        // Метод извлекает имя файла/директории до первого слэша ("/").
        auto extract_name(const std::string& path) -> std::string;
        // Метод обрезает строку, в которой указан путь к файлу, от начала
        // до первого слэша ("/").
        auto cut_string(std::string& path, char ch) -> void;
    }
}

#endif // FILESEARCH_H