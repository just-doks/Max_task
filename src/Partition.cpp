#include "Partition.h"

bool Partition::is_open() const
{
    if (pbr.is_fat() && drive.is_open())
        return true;
    return false;
}

void Partition::init(const std::string& path)
{
    std::string instruction = "umount ";
    instruction += path;
    system(instruction.c_str());
    drive.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (drive.is_open())
    {
        pbr.set(drive);
        FAT.resize(pbr.get_parameters().fat_size);
        drive.seekg(pbr.get_parameters().fat_offset, drive.beg);
        drive.read(FAT, pbr.get_parameters().fat_size);
    }
}