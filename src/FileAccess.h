#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include "File.h"

#include <string>

namespace FileManagment
{
    class FileAccess
    {
        protected:
            File& m_file;
        public:
            Partition& partition;
            uint32_t& partition_sn;
            File::Type& type;
            uint32_t& first_cluster;
            uint32_t& size;
            uint64_t& entry_offset;
            std::string& name;

            FileAccess(File& file): 
                partition(*file.partition),
                partition_sn(file.partition_sn),
                type(file.type),
                first_cluster(file.first_cluster),
                size(file.size),
                entry_offset(file.entry_offset), 
                name(file.name),
                m_file(file)
                {}
            FileAccess& operator=(File& file)
            {
                m_file = file;
            }
            FileAccess& operator=(File&& file)
            {
                m_file = file;
            }

            operator File() 
                { return File(m_file); }
    };

    class FileSetter
    {
        private:
            File* m_file;
        public:
            FileSetter() {}
            FileSetter(File& file): m_file(&file) {}

            void set_file (File& file)
            {
                m_file = &file;
            }

            void set_partition(Partition& partition)
            {
                m_file->partition = &partition;
            }
            void set_partition_sn(uint32_t partition_sn)
            {
                m_file->partition_sn = partition_sn;
            }
            void set_type(File::Type type)
            {
                m_file->type = type;
            }
            void set_first_cluster(uint32_t first_cluster)
            {
                m_file->first_cluster = first_cluster;
            }
            void set_size(uint32_t size)
            {
                m_file->size = size;
            }
            void set_entry_offset(uint64_t entry_offset)
            {
                m_file->entry_offset = entry_offset;
            }
            void set_name(std::string name)
            {
                m_file->name = name;
            }
    };
}

#endif // FILEMANAGER_H