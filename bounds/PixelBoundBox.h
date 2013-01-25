class PixelBoundBox
{
public:
    PixelBoundBox();
    PixelBoundBox(int x, int y);
    PixelBoundBox(int x0, int y0, int x1, int y1);
    
    int getX() const { return m_x0; }
    int getY() const { return m_y0; }

    int getWidth() const { return m_x1 - m_x0 + 1; }
    int getHeight() const { return m_y1 - m_y0 + 1; }

    void unionPoint(int x, int y);
    void unionBounds(const PixelBoundBox& other);

private:
    int m_x0;
    int m_x1;
    int m_y0;
    int m_y1;
};