// Image: a plain pixel buffer of height rows, each width * bytesPerPixel bytes, allocated row by row.
// Workspace::Screenshot reads the frame buffer into one (3 bytes per pixel) and the export code renders into them
// (3 or 4 bytes per pixel). ExportScreenshot writes them through FreeImage.
#pragma once

class Image {
public:
    Image(int width, int height, int bytesPerPixel);
    virtual ~Image();

    void exportScreenshot();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getBytesPerPixel() const { return m_bytesPerPixel; }

    unsigned char **m_rows; // m_height row pointers.
    int m_width;
    int m_height;
    int m_bytesPerPixel;
};
