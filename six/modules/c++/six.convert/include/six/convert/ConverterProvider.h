#ifndef __CONVERT_CONVERTER_PROVIDER_H__
#define __CONVERT_CONVERTER_PROVIDER_H__

#include <string>
#include <vector>

#include <six/sicd/ComplexData.h>
#include <sys/Conf.h>
#include <types/RowCol.h>

namespace six
{
namespace convert
{
class ConverterProvider
{
public:
    virtual ~ConverterProvider() {}

    //! Return true if converter supports given file
    virtual bool supports(const std::string& pathname) const = 0;

    //! Initialize converter
    virtual void load(const std::string& pathname) = 0;

    //! Parse loaded file and return a ComplexData object
    virtual std::auto_ptr<six::sicd::ComplexData> convert() = 0;

    /*!
     * Read given region from image
     * \param startingLocations Starting row and column
     * \param readLengths Number of rows and columns to reade
     * \param buffer Pre-allocated buffer for storing image data
     */
    virtual void readData(const types::RowCol<size_t>& startingLocations,
            const types::RowCol<size_t>& readLengths, UByte* buffer) = 0;

    /*!
     * Read entire image
     * Use getDataSizeInBytes() to determine buffer size
     * \param buffer Pre-allocated buffer for storing image data
     */
    virtual void readData(six::UByte* buffer) = 0;

    /*!
     * \return size of image data in bytes
     */
    virtual size_t getDataSizeInBytes() const = 0;

    /*
     * \return type of file being converted
     */
    virtual std::string getFileType() const = 0;
};
}
}

#endif

