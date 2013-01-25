#include "common.h"

//
// Simple table of uint32 values.
//
// We load the HDF5 data into Tables.  A Table is what
// we hold in memory, run queries out of and modify.
// We write Tables out to an HDF5 file when the session
// is saved. Our HdfFile class reads and writes Tables.
//
// Tables can have padding. Padding indicates how many extra
// rows should allocated. Padding of 0.1 means 10% extra rows
// are allocated.
//
// The addRows() command will use the extra rows, so adding
// rows in general is fast.  If no extra rows are available
// then a full allocate/copy step is performed.  The newly
// allocated array will again have the full amount of
// padding.
//
class Table
{
public:
    // Create an empty table of the given size, with optional padding
    Table(uint32 rows, uint32 columns, float padding = 0.1);
    
    // Create table from existing data, copy the given memory
    Table(uint32 rows, uint32 columns, uint32* data, float padding = 0.1);

    // Destructor
    ~Table();

    // Import from text.  Each line of text should have
    // N columns of integers, where N is our m_columns value.
    void import(const std::vector<char *>& lines);
    
    // Import from raw data, memcpy
    void importdata(const uint32* data);

    // Get table dimensions (padding not included)
    uint32 getRows() const { return m_rows; }
    uint32 getColumns() const { return m_columns; }

    // Get a single table value
    // throws std::string if out-of-bounds
    uint32 getValue(uint32 row, uint32 col) const;

    // Set a single table value
    // throws std::string if out-of-bounds
    void setValue(uint32 row, uint32 col, uint32 value);

    // Add rows.  Uses padding if available, otherwise will
    // do a full allocate/copy of existing data.
    // Return previous number of rows, before adding any
    void addRows(uint32 rows);
    
    // Set the number of rows in the table, likely to truncate
    // it down to a smaller size.  Does not affect allocated 
    // size but will affect size of table written to HDF5.
    void truncateRows(uint32 rows);

    // Get pointer to all the data
    uint32* getData() const { return m_data; }

private:
    // allocate our data array, including padding, save off sizes.
    uint32* allocateArray(size_t rows, uint32 columns);

    // size of table (without padding)
    uint32 m_rows;
    uint32 m_columns;

    // number of rows including padding, total allocated size
    uint32 m_rowsAllocated;

    // Padding of 0.1 means include 10% extra rows
    float m_padding;

    uint32* m_data;
};
