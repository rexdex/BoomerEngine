/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: rtti\types #]
***/

#include "build.h"
#include "rttiCustomType.h"

#include "streamTextReader.h"
#include "streamTextWriter.h"
#include "streamBinaryWriter.h"
#include "streamBinaryReader.h"

namespace base
{
    namespace rtti
    {

        //--

        CustomType::CustomType(const char* name, uint32_t size, uint32_t alignment, uint64_t nativeHash)
            : IType(StringID(name))
        {
            m_traits.nativeHash = nativeHash;
            m_traits.size = size;
            m_traits.alignment = alignment;
            m_traits.metaType = MetaType::Simple;
            m_traits.initializedFromZeroMem = true;
            m_traits.requiresConstructor = false;
            m_traits.requiresDestructor = false;
            m_traits.simpleCopyCompare = true;
        }

        CustomType::~CustomType()
        {}

        void CustomType::construct(void* object) const
        {
            if (funcConstruct)
                funcConstruct(object);
            else
                memset(object, 0, m_traits.size);
        }

        void CustomType::destruct(void* object) const
        {
            if (funcDestruct)
                funcDestruct(object);
        }

        bool CustomType::compare(const void* data1, const void* data2) const
        {
            if (funcComare)
                return funcComare(data1, data2);
            else
                return 0 == memcmp(data1, data2, m_traits.size);
        }

        void CustomType::copy(void* dest, const void* src) const
        {
            if (funcCopy)
                return funcCopy(dest, src);
            else
                memcpy(dest, src, m_traits.size);
        }

        void CustomType::calcCRC64(CRC64& crc, const void* data) const
        {
            if (funcHash)
                funcHash(crc, data);
            else
                crc.append(data, size());
        }

        bool CustomType::writeBinary(const TypeSerializationContext& typeContext, stream::IBinaryWriter& file, const void* data, const void* defaultData) const
        {
            if (funcWriteBinary)
                return funcWriteBinary(typeContext, file, data, defaultData);
            
            file.write(data, m_traits.size);
            return true;
        }

        bool CustomType::readBinary(const TypeSerializationContext& typeContext, stream::IBinaryReader& file, void* data) const
        {
            if (funcReadBinary)
                return funcReadBinary(typeContext, file, data);

            file.read(data, m_traits.size);
            return true;
        }

        bool CustomType::writeText(const TypeSerializationContext& typeContext, stream::ITextWriter& stream, const void* data, const void* defaultData) const
        {
            if (funcWriteText)
                return funcWriteText(typeContext, stream, data, defaultData);

            StringBuilder txt;
            printToText(txt, data, true);
            stream.writeValue(txt.view());
            return true;
        }

        bool CustomType::readText(const TypeSerializationContext& typeContext, stream::ITextReader& stream, void* data) const
        {
            if (funcReadText)
                return funcReadText(typeContext, stream, data);

            StringView<char> txt;
            if (!stream.readValue(txt))
                return false;

            return parseFromString(txt, data, true);
        }

        void CustomType::printToText(IFormatStream& f, const void* data, uint32_t flags) const
        {
            if (funcPrintToText)
                funcPrintToText(f, data, flags);
        }

        bool CustomType::parseFromString(StringView<char> txt, void* data, uint32_t flags) const
        {
            if (funcParseFromText)
                funcParseFromText(txt, data, flags);
            return false;
        }

        bool CustomType::describeDataView(StringView<char> viewPath, const void* viewData, DataViewInfo& outInfo) const
        {
            if (funcDescribeView)
                return funcDescribeView(viewPath, viewData, outInfo);

            return IType::describeDataView(viewPath, viewData, outInfo);
        }

        bool CustomType::readDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, const void* viewData, void* targetData, Type targetType) const
        {
            if (funcReadDataView)
                return funcReadDataView(viewPath, viewData, targetData, targetType);

            return IType::readDataView(context, rootView, rootViewPath, viewPath, viewData, targetData, targetType);
        }

        bool CustomType::writeDataView(IObject* context, const IDataView* rootView, StringView<char> rootViewPath, StringView<char> viewPath, void* viewData, const void* sourceData, Type sourceType) const
        {
            if (funcWriteDataView)
                return funcWriteDataView(viewPath, viewData, sourceData, sourceType);

            return IType::writeDataView(context, rootView, rootViewPath, viewPath, viewData, sourceData, sourceType);
        }

        //--

    } // rtti
} // base