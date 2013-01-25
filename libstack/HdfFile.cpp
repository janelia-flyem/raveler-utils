#include "HdfFile.h"
#include "Table.h"
#include "util.h"

//
// VERSION is written as an attribute on the root dir 
// so we can identify our own files.
//
const char * HdfFile::s_VERSION_NAME = "hdf-stack-version";
const uint32 HdfFile::s_VERSION_NUMBER = 1;

HdfFile::HdfFile() :
    m_file(-1)
{
}

HdfFile::~HdfFile()
{
    if (m_file >= 0)
    {
	if (H5Fclose(m_file) < 0)
	{
	    throw std::string("Cannot close file");
	}
    }
}

void HdfFile::openForWrite(const std::string& path)
{
    m_file = H5Fcreate(path.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    
    if (m_file < 0)
    {
	throw std::string("Cannot open output for writing");
    }
    
    // write our version as an attribute on the root
    hid_t root = H5Gopen(m_file, "/", H5P_DEFAULT);
    
    if (root < 0)
    {
	throw std::string("Cannot open root group");
    }
    
    hid_t space = H5Screate(H5S_SCALAR);    
    
    if (space < 0)
    {
	throw std::string("Cannot create scaler space");
    }
        
    hid_t attr = H5Acreate(root,
	s_VERSION_NAME, H5T_NATIVE_UINT, space, H5P_DEFAULT, H5P_DEFAULT);    
	
    if (attr < 0)
    {
	throw std::string("Cannot create attribute on root");
    }
	
    if (H5Awrite(attr, H5T_NATIVE_UINT, &s_VERSION_NUMBER) < 0)
    {
	throw std::string("Cannot write attribute to root");
    }
	
    if (H5Aclose(attr) < 0)
    {
	throw std::string("Cannot close attribute");
    }
	
    if (H5Sclose(space) < 0)
    {
	throw std::string("Cannot close datascape");
    }
    
    if (H5Gclose(root) < 0)
    {	
	throw std::string("Cannot close root group");
    }
}

void HdfFile::openForRead(const std::string& path)
{
    m_file = H5Fopen(path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    
    if (m_file < 0)
    {
	throw std::string("Cannot open HDF5");
    }
    
    checkVersion();
}

// check the version attribute on the root group
void HdfFile::checkVersion()
{       
    uint32 version;
        
    hid_t root = H5Gopen(m_file, "/", H5P_DEFAULT);    

    if (root < 0)
    {
	throw std::string("Cannot open root group");
    }
    
    hid_t attr = H5Aopen(root, s_VERSION_NAME, H5P_DEFAULT);
    
    if (attr < 0)
    {
	throw FormatString("Cannot find attribute '%s'", s_VERSION_NAME);
    }	
    
    herr_t ret = H5Aread(attr, H5T_NATIVE_INT, &version);
    
    if (ret < 0)
    {
	throw std::string("Cannot read version attribute");
    }
        
    if (version != s_VERSION_NUMBER)
    {
	throw std::string("Version mismatch");
    }	

    ret = H5Aclose(attr);
    
    if (ret < 0)
    {
	throw std::string("Error closing attribute");
    }
    
    ret = H5Gclose(root);

    if (ret < 0)
    {
	throw std::string("Error closing root");
    }
}

void HdfFile::writeDataset(const std::string& name, const Table& table)
{
    int rank = 2;    
    hsize_t dims[2] = { table.getRows(), table.getColumns() };
    
    writeDataset(name, rank, dims, table.getData());
}
    
void HdfFile::writeDataset(const std::string& name, 
    int rank, hsize_t *dims, uint32* data)
{
    hid_t dataspace = H5Screate_simple(rank, dims, NULL);
    
    // Little endian unsigned ints
    hid_t datatype = H5Tcopy(H5T_NATIVE_UINT);
    herr_t status = H5Tset_order(datatype, H5T_ORDER_LE);    
   
    if (status < 0)
    {
	printf("ERROR from H5Tset_order: %d\n", status);
	return;
    }
    
    hid_t dataset = H5Dcreate2(m_file, name.c_str(), datatype, dataspace,
			H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

    status = H5Dwrite(dataset,
        H5T_NATIVE_UINT,
        H5S_ALL, H5S_ALL,
        H5P_DEFAULT,
        data);
	
    if (status < 0)
    {
	printf("ERROR from H5Dwrite: %d\n", status);
	return;
    }

    status = H5Sclose(dataspace);
    
    if (status < 0)
    {
	printf("ERROR from H5Sclose: %d\n", status);
	return;
    }
    
    status = H5Tclose(datatype);

    if (status < 0)
    {
	printf("ERROR from H5Tclose: %d\n", status);
	return;
    }
    
    status = H5Dclose(dataset);
    
    if (status < 0)
    {
	printf("ERROR from H5Dclose: %d\n", status);
	return;
    }    
}

void HdfFile::createGroup(const std::string& name)
{
    hid_t group = H5Gcreate(m_file, 
	name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	
    H5Gclose(group);
}

Table* HdfFile::readTable(const std::string& path)
{
    hid_t dataset = H5Dopen2(m_file, path.c_str(), H5P_DEFAULT);
    
    if (dataset < 0)
    {
	throw std::string("Cannot open dataset");
    }
    
    hid_t datatype = H5Dget_type(dataset);
    
    if (datatype < 0)
    {
	throw std::string("Cannot get datatype");
    }
	    
    H5T_class_t t_class = H5Tget_class(datatype);
    
    if (t_class != H5T_INTEGER)
    {
	throw std::string("Expected integer data.");
    }
    
    H5T_order_t order = H5Tget_order(datatype);
    
    if (order != H5T_ORDER_LE)
    {
	throw std::string("Expected little endian data.");
    }    
    
    size_t size = H5Tget_size(datatype);
    
    if (size != 4)
    {
	throw std::string("Expected 32-bit data.");
    }
	
    hid_t dataspace = H5Dget_space(dataset);
    
    if (dataspace < 0)
    {
	throw std::string("Cannot get dataspace.");
    }
    
    int rank = H5Sget_simple_extent_ndims(dataspace);
    
    if (rank != 1 && rank != 2)
    {
	throw std::string("Unexpected rank");
    }
    
    hsize_t dims_out[2];
    dims_out[1] = 1;
    int status_n = H5Sget_simple_extent_dims(dataspace, dims_out, NULL);
    
    if (status_n < 0)
    {
	throw std::string("Error getting dims");
    }
    
    hsize_t rows = dims_out[0];
    hsize_t cols = dims_out[1];
    
    //printf("rank %d, dimensions %lu x %lu \n", rank,
	//   (unsigned long)(dims_out[0]), (unsigned long)(dims_out[1]));
	   
    Table* table = new Table(rows, cols);
	   
    herr_t status = H5Dread(dataset, H5T_NATIVE_UINT, H5S_ALL, 
	H5S_ALL, H5P_DEFAULT, table->getData());
	
    if (status < 0)
    {
	delete table;
	throw std::string("Read failed");
    }
    
    if (H5Dclose(dataset) < 0)
    {
	throw std::string("Cannot close dataset");
    }
       
    return table;
}

//
// For findfile callback
///
StringList* gFilenames = NULL;

//
// Callback used by listDatasets
//
static herr_t findfile(hid_t loc_id, 
    const char *name, const H5L_info_t *linfo, void *opdata)
{
    /* avoid compiler warnings */
    loc_id = loc_id;
    opdata = opdata;
    linfo = linfo;
    
    gFilenames->push_back(name);
    
    return 0;
}

//
// Find all the datasets in a given group.
//
void HdfFile::listDatasets(const std::string& path, StringList& result)
{
    gFilenames = &result;
    
    herr_t it = H5Literate_by_name(
	m_file, path.c_str(), H5_INDEX_NAME, H5_ITER_INC, NULL,
	findfile, NULL, H5P_DEFAULT);   
	
    gFilenames = NULL;
	
    if (it < 0)
    {
	throw std::string("Iterate failed");
    }
}
