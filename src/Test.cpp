#include "Test.h"
#include "File.h"
#include "FileUtilities.h"

#include <cstdlib> // system()

void Test::create_file(std::string name, uint32_t size_in_clusters, uint32_t cl_s)
{
    std::ofstream outfile(name);
    uint32_t limit = size_in_clusters * cl_s;
    for (uint32_t i = 0U; i < limit; ++i)
    {
        if ((i & 0x3FU) == 0x3FU)
            outfile << '\n';
        else
            outfile << '8';
    }
    outfile.close();
}

void Test::start()
{
    int num = -1;
    std::cout   << "open_partition()\t- 1\n"
                << "create_file()\t\t- 2\n";
    std::cout << "Answer: ";
    std::cin >> num;
    switch(num)
    {
        case 1:
        {
            open_partition();
            break;
        }
        case 2:
        {
            std::string name;
            uint32_t cl;
            uint32_t cl_size;
            std::cout << "Enter name, cluster and cluster_size: ";
            std::cin >> name;
            std::cin >> cl;
            std::cin >> cl_size;
            create_file(name, cl, cl_size);
            break;
        }
    }
}

void Test::open_partition()
{
    std::cout << "Path: ";
    std::string path;
    std::cin >> path;
    TPart p(path);
    if (p.is_open())
    {
        p_options(p);
    }
}

void Test::p_options(TPart& p)
{
    int num = -1;
    std::cout   << "copy clusters\t\t\t- 1\n"
                << "print PBR\t\t\t- 2\n"
                << "Create file fragmentation\t- 3\n";
    std::cout << "Answer: ";
    std::cin >> num;
    switch(num)
    {
        case 1: 
        {
            uint32_t src;
            uint32_t dest;
            char ch;
            do
            {
                std::cout << "Source cluster: ";
                std::cin >> src;
                std::cout << "Destination cluster: ";
                std::cin >> dest;
                p.copy_cluster(src, dest);
                std::cout << "Repeat?\n(y/n): ";
                std::cin >> ch;
            } while (ch == 'y');
            break;
        }
        case 2:
        {
            p.print_pbr();
            break;
        }
        case 3:
        {
            namespace FM = FileManagment;
            std::cout << "Enter path to file: ";
            std::string path;
            std::cin >> path;
            FM::File file = FM::get_file(path, p);
            f_options(file, p);
            break;
        }
    }
}

void print_clusters(std::vector<uint32_t> list);

void Test::f_options(FileManagment::File& file, TPart& p)
{
    namespace FM = FileManagment;
    namespace MM = FM::MemoryManagment;

    int num = -1;
    std::vector<uint32_t> cluster_list;
    //std::vector<uint32_t> new_list;

    system("clear");
    do
    {
        num = -1;
        std::cout   << "Get File clusters\t- 1\n"
                    << "Set new clusters chain\t- 2\n"
                    << "Move cluster\t\t- 3\n"
                    << "Launch fragmentation\t- 4\n"
                    << "Quit\t\t\t- 0\n";
        std::cout << "Answer: ";
        std::cin >> num;
        switch(num)
        {
            case 1: 
            {
                cluster_list = MM::get_file_clusters(file);
                print_clusters(cluster_list);
                break;
            }
            case 2:
            {
                uint32_t c;
                cluster_list.clear();
                for (unsigned int i = 0; i < MM::count_file_clusters(file); ++i)
                {
                    std::cout << "cluster[" << i << "] = ";
                    std::cin >> c;
                    cluster_list.push_back(c);
                }
                break;
            }
            case 4:
            {
                if (cluster_list.size() == 0)
                {
                    std::cout << "New cluster's chain doesn't set\n";
                } else {
                    if (p.fragment_file(file, cluster_list))
                        std::cout << "Done\n";
                    else
                        std::cout << "Something went wrong\n";
                }
                break;
            }
            case 3:
            {
                std::cout << "Select cluster number: ";
                int n = -1;
                std::cin >> n;
                if (n > cluster_list.size())
                {
                    std::cout << "Invalid number\n";
                    break;
                }
                uint32_t a {0};
                std::cout << "Enter new address: ";
                std::cin >> a;
                cluster_list[n] = a;
                break;
            }
        }
    } while (num != 0);

    
}

void print_clusters(std::vector<uint32_t> list)
{
    for (int i = 0; i < 10; ++i)
        std::cout << '\t' << i;
    std::cout << "\n\n0\t";
    for (int i = 0; i < list.size(); ++i)
    {
        std::cout << list[i] << "\t";
        if ((i + 1) % 10 == 0) 
            std::cout << "\n" << ((i + 1) / 10) << '\t';
    }
    std::cout << '\n';
}