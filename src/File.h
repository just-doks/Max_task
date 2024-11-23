#ifndef FILE_H
#define FILE_H

#include <cstdint>
#include <string>

#include "Partition.h"

namespace FileManagment
{

    class File
    {
        public:
            enum Type
            {
                NONE = 0,
                DIR,
                FILE,
                ROOT_DIR
            };

            Partition*              partition     {nullptr};
            uint32_t                partition_sn  {0};
            File::Type              type          {NONE};
            uint32_t                first_cluster {0};
            uint32_t                size          {0};
            uint64_t                entry_offset  {0};
            std::string             name          {};

            File() {}
    };
}

#endif // FILE_H