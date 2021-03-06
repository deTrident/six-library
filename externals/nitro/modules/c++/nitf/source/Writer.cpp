/* =========================================================================
 * This file is part of NITRO
 * =========================================================================
 *
 * (C) Copyright 2004 - 2014, MDA Information Systems LLC
 *
 * NITRO is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include "nitf/Writer.hpp"

using namespace nitf;

void WriterDestructor::operator()(nitf_Writer *writer)
{
    if (writer && writer->record)
    {
        // this tells the handle manager that the Record is no longer managed
        nitf::Record rec(writer->record);
        rec.setManaged(false);
    }
    if (writer && writer->output)
    {
        // this tells the handle manager that the IOInterface is no longer managed
        nitf::IOInterface io(writer->output);
        io.setManaged(false);
    }
    nitf_Writer_destruct(&writer);
}

Writer::Writer(const Writer & x)
{
    setNative(x.getNative());
}

Writer & Writer::operator=(const Writer & x)
{
    if (&x != this)
        setNative(x.getNative());
    return *this;
}

Writer::Writer(nitf_Writer * x)
{
    setNative(x);
    getNativeOrThrow();
}

Writer::Writer() throw (nitf::NITFException)
{
    setNative(nitf_Writer_construct(&error));
    getNativeOrThrow();
    setManaged(false);
}

Writer::~Writer()
{
//    for (std::vector<nitf::WriteHandler*>::iterator it = mWriteHandlers.begin(); it
//            != mWriteHandlers.end(); ++it)
//    {
//        delete *it;
//    }
}

void Writer::write()
{
    NITF_BOOL x = nitf_Writer_write(getNativeOrThrow(), &error);
    if (!x)
        throw nitf::NITFException(&error);
}

void Writer::prepare(nitf::IOHandle & io, nitf::Record & record)
        throw (nitf::NITFException)
{
    prepareIO(io, record);
}

void Writer::prepareIO(nitf::IOInterface & io, nitf::Record & record)
        throw (nitf::NITFException)
{
    NITF_BOOL x = nitf_Writer_prepareIO(getNativeOrThrow(), record.getNative(),
                                        io.getNative(), &error);

    // It's possible prepareIO() failed but actually took ownership of one
    // or both of these objects.  So we need to call setManaged() on them
    // properly regardless of if the function succeeded.
    if (getNativeOrThrow()->record == record.getNative())
    {
        record.setManaged(true);
    }

    if (getNativeOrThrow()->output == io.getNative())
    {
        io.setManaged(true);
    }

    if (!x)
    {
        throw nitf::NITFException(&error);
    }
}

void Writer::setWriteHandlers(nitf::IOHandle& io, nitf::Record& record)
{
    setImageWriteHandlers(io, record);
    setGraphicWriteHandlers(io, record);
    setTextWriteHandlers(io, record);
    setDEWriteHandlers(io, record);
}

void Writer::setImageWriteHandlers(nitf::IOHandle& io, nitf::Record& record)
{
    nitf::List images = record.getImages();
    const size_t numImages = record.getNumImages();
    for (size_t ii = 0; ii < numImages; ++ii)
    {
        nitf::ImageSegment segment = images[ii];
        const size_t offset = segment.getImageOffset();
        mem::SharedPtr<nitf::WriteHandler> handler(
                new nitf::StreamIOWriteHandler(
                    io, offset, segment.getImageEnd() - offset));
        setImageWriteHandler(ii, handler);
    }
}

void Writer::setGraphicWriteHandlers(nitf::IOHandle& io, nitf::Record& record)
{
    nitf::List graphics = record.getGraphics();
    const size_t numGraphics = record.getNumGraphics();
    for (size_t ii = 0; ii < numGraphics; ++ii)
    {
       nitf::GraphicSegment segment = graphics[ii];
       long offset = segment.getOffset();
       mem::SharedPtr< ::nitf::WriteHandler> handler(
           new nitf::StreamIOWriteHandler (
               io, offset, segment.getEnd() - offset));
       setGraphicWriteHandler(ii, handler);
    }
}

void Writer::setTextWriteHandlers(nitf::IOHandle& io, nitf::Record& record)
{
    nitf::List texts = record.getTexts();
    const size_t numTexts = record.getNumTexts();
    for (size_t ii = 0; ii < numTexts; ++ii)
    {
       nitf::TextSegment segment = texts[ii];
       const size_t offset = segment.getOffset();
       mem::SharedPtr< ::nitf::WriteHandler> handler(
           new nitf::StreamIOWriteHandler (
               io, offset, segment.getEnd() - offset));
       setTextWriteHandler(ii, handler);
    }
}

void Writer::setDEWriteHandlers(nitf::IOHandle& io, nitf::Record& record)
{
    nitf::List dataExtensions = record.getDataExtensions();
    const size_t numDEs = record.getNumDataExtensions();
    for (size_t ii = 0; ii < numDEs; ++ii)
    {
       nitf::DESegment segment = dataExtensions[ii];
       const size_t offset = segment.getOffset();
       mem::SharedPtr< ::nitf::WriteHandler> handler(
           new nitf::StreamIOWriteHandler (
               io, offset, segment.getEnd() - offset));
       setDEWriteHandler(ii, handler);
    }
}

void Writer::setImageWriteHandler(int index,
                                  mem::SharedPtr<WriteHandler> writeHandler)
        throw (nitf::NITFException)
{
    if (!nitf_Writer_setImageWriteHandler(getNativeOrThrow(), index,
                                          writeHandler->getNative(), &error))
        throw nitf::NITFException(&error);
    writeHandler->setManaged(true);
    mWriteHandlers.push_back(writeHandler);
}

void Writer::setGraphicWriteHandler(int index,
                                    mem::SharedPtr<WriteHandler> writeHandler)
        throw (nitf::NITFException)
{
    if (!nitf_Writer_setGraphicWriteHandler(getNativeOrThrow(), index,
                                            writeHandler->getNative(), &error))
        throw nitf::NITFException(&error);
    writeHandler->setManaged(true);
    mWriteHandlers.push_back(writeHandler);
}

void Writer::setTextWriteHandler(int index,
                                 mem::SharedPtr<WriteHandler> writeHandler)
        throw (nitf::NITFException)
{
    if (!nitf_Writer_setTextWriteHandler(getNativeOrThrow(), index,
                                         writeHandler->getNative(), &error))
        throw nitf::NITFException(&error);
    writeHandler->setManaged(true);
    mWriteHandlers.push_back(writeHandler);
}

void Writer::setDEWriteHandler(int index,
                               mem::SharedPtr<WriteHandler> writeHandler)
        throw (nitf::NITFException)
{
    if (!nitf_Writer_setDEWriteHandler(getNativeOrThrow(), index,
                                       writeHandler->getNative(), &error))
        throw nitf::NITFException(&error);
    writeHandler->setManaged(true);
    mWriteHandlers.push_back(writeHandler);
}

nitf::ImageWriter Writer::newImageWriter(int imageNumber)
        throw (nitf::NITFException)
{
    nitf_SegmentWriter * x = nitf_Writer_newImageWriter(getNativeOrThrow(),
                                                        imageNumber, NULL, &error);
    if (!x)
        throw nitf::NITFException(&error);

    //manage the writer
    nitf::ImageWriter writer(x);
    writer.setManaged(true);
    return writer;
}

nitf::ImageWriter Writer::newImageWriter(int imageNumber,
                                         const std::map<std::string, void*>& options)
    throw (nitf::NITFException)
{
    nitf::HashTable userOptions;
    nrt_HashTable* userOptionsNative = NULL;

    if (!options.empty())
    {
        userOptions.setPolicy(NRT_DATA_RETAIN_OWNER);
        for (std::map<std::string, void*>::const_iterator iter =
                     options.begin();
            iter != options.end();
            ++iter)
        {
            userOptions.insert(iter->first, iter->second);
        }
        userOptionsNative = userOptions.getNative();
    }

    nitf_SegmentWriter* x = nitf_Writer_newImageWriter(getNativeOrThrow(),
                                                       imageNumber,
                                                       userOptionsNative,
                                                       &error);

    if (!x)
    {
        throw nitf::NITFException(&error);
    }

    //manage the writer
    nitf::ImageWriter writer(x);
    writer.setManaged(true);
    return writer;
}

nitf::SegmentWriter Writer::newGraphicWriter(int graphicNumber)
        throw (nitf::NITFException)
{
    nitf_SegmentWriter * x =
            nitf_Writer_newGraphicWriter(getNativeOrThrow(), graphicNumber,
                                         &error);
    if (!x)
        throw nitf::NITFException(&error);

    //manage the writer
    nitf::SegmentWriter writer(x);
    writer.setManaged(true);
    return writer;
}

nitf::SegmentWriter Writer::newTextWriter(int textNumber)
        throw (nitf::NITFException)
{
    nitf_SegmentWriter * x = nitf_Writer_newTextWriter(getNativeOrThrow(),
                                                       textNumber, &error);
    if (!x)
        throw nitf::NITFException(&error);

    //manage the writer
    nitf::SegmentWriter writer(x);
    writer.setManaged(true);
    return writer;
}

nitf::SegmentWriter Writer::newDEWriter(int deNumber)
        throw (nitf::NITFException)
{
    nitf_SegmentWriter * x = nitf_Writer_newDEWriter(getNativeOrThrow(),
                                                     deNumber, &error);
    if (!x)
        throw nitf::NITFException(&error);

    //manage the writer
    nitf::SegmentWriter writer(x);
    writer.setManaged(true);
    return writer;
}

//! Get the warningList
nitf::List Writer::getWarningList()
{
    return nitf::List(getNativeOrThrow()->warningList);
}

void Writer::writeHeader(nitf::Off& fileLenOff, nitf::Uint32& hdrLen)
{
    if (!nitf_Writer_writeHeader(getNativeOrThrow(),
                                 &fileLenOff,
                                 &hdrLen,
                                 &error))
    {
        throw nitf::NITFException(&error);
    }
}

void Writer::writeImageSubheader(nitf::ImageSubheader subheader,
                                 nitf::Version version,
                                 nitf::Off& comratOff)
{
    if (!nitf_Writer_writeImageSubheader(getNativeOrThrow(),
                                         subheader.getNativeOrThrow(),
                                         version,
                                         &comratOff,
                                         &error))
    {
        throw nitf::NITFException(&error);
    }
}

void Writer::writeDESubheader(nitf::DESubheader subheader,
                              nitf::Uint32& userSublen,
                              nitf::Version version)
{
    if (!nitf_Writer_writeDESubheader(getNativeOrThrow(),
                                      subheader.getNativeOrThrow(),
                                      &userSublen,
                                      version,
                                      &error))
    {
        throw nitf::NITFException(&error);
    }
}

void Writer::writeInt64Field(nitf::Uint64 field,
                             nitf::Uint32 length,
                             char fill,
                             nitf::Uint32 fillDir)
{
    if (!nitf_Writer_writeInt64Field(getNativeOrThrow(),
                                     field,
                                     length,
                                     fill,
                                     fillDir,
                                     &error))
    {
        throw nitf::NITFException(&error);
    }
}
