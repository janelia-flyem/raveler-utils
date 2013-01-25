#include "common.h"
#include "hdf5.h"

//
// Interface to read from or write to a single HDF5 file.
//
class HdfFile
{
public: 
    HdfFile();
    ~HdfFile();
    
    // Open HDF5 for writing, writes the "version" property
    // Return true on success
    void openForWrite(const std::string& path);
    
    // Open HDF5 for reading, checks the version is okay
    // Return true on success
    void openForRead(const std::string& path);
    
    // Write a table
    void writeDataset(const std::string& name, const Table& table);

    // Create a group off the root
    void createGroup(const std::string& name);
    
    // Read a dataset, allocates a new Table
    Table* readTable(const std::string& name);
    
    // Get all datasets in a group
    void listDatasets(const std::string& path, StringList& result);
    
private:

    void checkVersion();

    void writeDataset(const std::string& name,
        int rank, hsize_t *dims, uint32* data);

    // Our open HDF5 file
    hid_t m_file;
    
    static const char* s_VERSION_NAME;
    static const uint32 s_VERSION_NUMBER;
};
