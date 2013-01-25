 #include <string>
#include <map>

//
// Parses and holds metadata from tiles/metadata.txt
//
class Metadata
{
public:
    Metadata(std::string root);

    int getIntValue(std::string key);

private:
    std::map<std::string, std::string> m_values;
};

//
// Provides paths for tiles in stack. Reads and store metadata.
//
class Stack
{
public:
    Stack(const std::string& root, int tilesize);

    int getMetadataValue(std::string key);

    int getNumRows(int lod);
    int getNumCols(int lod);

    std::string getSuperpixelBoundsPath();

    std::string getTilePath(int lod, int row, int col, char channel, int section);

private:
    int getNumTiles(int imagesize, int lod);

    std::string m_root;
    int m_tilesize;

    Metadata m_metadata;
};
