#include "Table.h"
#include "util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


Table::Table(uint32 rows, uint32 columns, float padding) :
    m_padding(padding)
{
    // allocate initial size, it will include padding
    m_data = allocateArray(rows, columns);
}

Table::Table(uint32 rows, uint32 columns, uint32* data, float padding) :
    m_padding(padding)
{
    m_data = allocateArray(rows, columns);
    size_t nbytes = rows * columns * sizeof(uint32);
    memcpy(m_data, data, nbytes);    
}

Table::~Table()
{
    delete [] m_data;
}

//
// Allocate our main array, with padding
//
uint32* Table::allocateArray(size_t rows, uint32 columns)
{
    if (rows > MAX_TABLE_ROWS)
    {
        throw FormatString("Table::allocateArray(%u, %u) too many rows", 
            rows, columns);
    }    

    m_rows = rows;
    m_columns = columns;
    
    // Could go over 32-bit if we are overflowing table, so use size_t
    size_t allocateRows = m_rows + size_t(m_padding * m_rows + 0.5);
    
    // Limit in case we are near the limit and padding put us over
    m_rowsAllocated = std::min((size_t)MAX_TABLE_ROWS, allocateRows);
    
    // Note even though rows are 32-bit, size could be more than 32-bit
    // because of the number of columns
    size_t size = m_rowsAllocated * m_columns;
    
    uint32* array = new uint32[size];

    for (size_t i = 0; i < size; ++i)
    {
        array[i] = EMPTY_VALUE;
    }

    return array;
}

//
// Import from text, text should have the correct number
// of integers on each line, matching our number of columns.
//
// Import is used only by compilestack when constructing
// and HDF5 the first time. Raveler does not use this, so
// speed is not super important.
//
void Table::import(const std::vector<char *>& lines)
{
    for (uint32 i = 0; i < lines.size(); i++)
    {
        const char* buffer = lines[i];
        const char *next = buffer;

        uint32* row = &m_data[i * m_columns];

        for (uint32 j = 0; j < m_columns; j++)
        {
            char *end = NULL;
            uint32 value = strtol(next, &end, 10);
            next = end;

            row[j] = value;
        }
    }
}

//
// Get a single table value
//
uint32 Table::getValue(uint32 row, uint32 col) const
{
    if (row >= m_rows || col >= m_columns)
    {
        throw FormatString("Table::getValue(%u, %u) not in range", row, col);
    }

    return m_data[row*m_columns + col];
}

//
// Set a single table value
//
void Table::setValue(uint32 row, uint32 col, uint32 value)
{
    if (row >= m_rows || col >= m_columns)
    {
        throw FormatString("Table::setValue(%u, %u) not in range", row, col);
    }

    m_data[row*m_columns + col] = value;
}

//
// Add more rows to the table.  Will only cause an allocation if
// we don't have any extra rows (padding) available.
//
void Table::addRows(uint32 rows)
{    
    if (rows == 0)
    {
        // Doesn't really make sense but allow it, do nothing
        return;
    } 
    else if (m_rows + rows <= m_rowsAllocated)
    {
        // There is space available, note they were already
        // initialized with EMPTY_VALUE when created.
        m_rows += rows;
    }
    else
    {
        // Need to allocate
        uint32 old_rows = m_rows;

        // Compute new size, padding (if any) is added by allocateArray
        // Use size_t because could be over 32-bit if we are about to
        // be out of space.
        size_t new_rows = (size_t)m_rows + rows;        
        
        // Rows are initialized with EMPTY_VALUE
        uint32* new_data = allocateArray(new_rows, m_columns);

        // Copy old stuff, free it, use the new stuff
        size_t nbytes = sizeof(uint32) * (size_t)old_rows * m_columns;
        
        memcpy(new_data, m_data, nbytes);
        delete m_data;
        m_data = new_data;
    }
}

void Table::truncateRows(uint32 rows)
{
    if (rows > m_rowsAllocated)
    {
        throw FormatString(
            "Table::truncateRows(%u) can't make table bigger", rows);
    }
    
    // Mark the now unused portion as empty
    for (uint32 i = rows; i < m_rows; ++i)
    {
        for (uint32 j = 0; j < m_columns; ++j)
        {
            m_data[i*m_columns + j] = EMPTY_VALUE;
        }
    }   
    
    m_rows = rows;
}
