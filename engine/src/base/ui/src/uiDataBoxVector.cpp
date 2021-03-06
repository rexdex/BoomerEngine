/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: elements\controls\data #]
***/

#include "build.h"
#include "uiDataBoxNumber.h"
#include "uiTextLabel.h"
#include "base/object/include/rttiDataView.h"

namespace ui
{

    //--

    struct CompInfo
    {
        base::StringView<char> path;
        base::StringView<char> caption;
    };

    static CompInfo VECTOR_COMPS[4] = { {"x", " X "}, {"y", " Y "}, {"z", " Z "}, {"w", " W "} };
    static CompInfo ANGLE_COMPS[3] = { {"roll", " R "}, {"pitch", " P "}, {"yaw", " Y "} };

    /// Vector3 editor
    class DataBoxVector : public IDataBox
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxVector, IDataBox);

    public:
        DataBoxVector(base::Type type, int numComps, int numDigits, bool rangeEnabled, double rangeMin, double rangeMax, bool dragger, base::StringView<char> units, const CompInfo* comps, bool vertical)
            : m_comps(comps)
        {
            if (vertical)
            {
                layoutVertical();

                if (numComps >= 1)
                {
                    auto inner = createChild<ui::IElement>();
                    inner->layoutHorizontal();

                    inner->createNamedChild<TextLabel>("X"_id, comps[0].caption);
                    m_componentX = inner->createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, units);
                }

                if (numComps >= 2)
                {
                    auto inner = createChild<ui::IElement>();
                    inner->layoutHorizontal();

                    inner->createNamedChild<TextLabel>("Y"_id, comps[1].caption);
                    m_componentY = inner->createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, units);
                }

                if (numComps >= 3)
                {
                    auto inner = createChild<ui::IElement>();
                    inner->layoutHorizontal();

                    inner->createNamedChild<TextLabel>("Z"_id, comps[2].caption);
                    m_componentZ = inner->createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, units);
                }

                if (numComps >= 4)
                {
                    auto inner = createChild<ui::IElement>();
                    inner->layoutHorizontal();

                    inner->createNamedChild<TextLabel>("W"_id, comps[3].caption);
                    m_componentW = inner->createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, units);
                }
            }
            else
            {
                layoutHorizontal();

                if (numComps >= 1)
                {
                    createNamedChild<TextLabel>("X"_id, comps[0].caption);
                    m_componentX = createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, units);
                }

                if (numComps >= 2)
                {
                    createNamedChild<TextLabel>("Y"_id, comps[1].caption);
                    m_componentY = createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, units);
                }

                if (numComps >= 3)
                {
                    createNamedChild<TextLabel>("Z"_id, comps[2].caption);
                    m_componentZ = createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, units);
                }

                if (numComps >= 4)
                {
                    createNamedChild<TextLabel>("W"_id, comps[3].caption);
                    m_componentW = createChild<DataBoxNumberText>(type, numDigits, rangeEnabled, rangeMin, rangeMax, dragger, units);
                }
            }
        }

        virtual void bind(const base::DataProxyPtr& data, const base::StringBuf& path, bool readOnly /*= false*/) override
        {
            if (m_componentX)
                m_componentX->bind(data, base::TempString("{}.{}", path, m_comps[0].path), readOnly);
            if (m_componentY)
                m_componentY->bind(data, base::TempString("{}.{}", path, m_comps[1].path), readOnly);
            if (m_componentZ)
                m_componentZ->bind(data, base::TempString("{}.{}", path, m_comps[2].path), readOnly);
            if (m_componentW)
                m_componentW->bind(data, base::TempString("{}.{}", path, m_comps[3].path), readOnly);

            TBaseClass::bind(data, path, readOnly);
        }

        virtual void enterEdit() override
        {
            if (m_componentX)
                m_componentX->enterEdit();
        }

        virtual void cancelEdit() override
        {
            if (m_componentX)
                m_componentX->cancelEdit();
            if (m_componentY)
                m_componentY->cancelEdit();
            if (m_componentZ)
                m_componentZ->cancelEdit();
        }

        virtual void handleValueChange() override
        {
            if (m_componentX)
                m_componentX->handleValueChange();
            if (m_componentY)
                m_componentY->handleValueChange();
            if (m_componentZ)
                m_componentZ->handleValueChange();
            if (m_componentW)
                m_componentW->handleValueChange();
        }

        virtual bool canExpandChildren() const override
        {
            return false;
        }

    protected:
        DataBoxPtr m_componentX;
        DataBoxPtr m_componentY;
        DataBoxPtr m_componentZ;
        DataBoxPtr m_componentW;

        const CompInfo* m_comps;
    };

    RTTI_BEGIN_TYPE_NATIVE_CLASS(DataBoxVector);
        RTTI_METADATA(ui::ElementClassNameMetadata).name("DataBoxVector");
    RTTI_END_TYPE();

    //--

    class DataBoxVectorFactory : public IDataBoxFactory
    {
        RTTI_DECLARE_VIRTUAL_CLASS(DataBoxVectorFactory, IDataBoxFactory);

    public:
        virtual DataBoxPtr tryCreate(const base::rtti::DataViewInfo& info) const override
        {
            const auto verticalLayout = info.flags.test(base::rtti::DataViewInfoFlagBit::VerticalEditor);

            if (info.dataType == base::reflection::GetTypeObject<base::Vector2>() 
                || info.dataType == base::reflection::GetTypeObject<base::Vector3>() 
                || info.dataType == base::reflection::GetTypeObject<base::Vector4>())
            {
                int numComps = 2;

                if (info.dataType == base::reflection::GetTypeObject<base::Vector3>())
                    numComps = 3;
                else if (info.dataType == base::reflection::GetTypeObject<base::Vector4>())
                    numComps = 4;

                bool rangeEnabled = false;
                bool dragEnabled = false;
                bool dragWrapEnabled = false;
                int numDigits = 2;
                double rangeMin = -DBL_MAX;
                double rangeMax = DBL_MAX;
                base::StringBuf units = "";

                for (const auto* metadata : info.metadata)
                {
                    if (const auto dragMeta = base::rtti_cast<base::PropertyHasDragMetadata>(metadata))
                    {
                        dragEnabled = true;
                        dragWrapEnabled = dragMeta->wrap;
                    }
                    else if (const auto rangeMeta = base::rtti_cast<base::PropertyNumberRangeMetadata>(metadata))
                    {
                        rangeEnabled = true;
                        rangeMin = rangeMeta->min;
                        rangeMax = rangeMeta->max;
                    }
                    else if (const auto digitsMeta = base::rtti_cast<base::PropertyPrecisionDigitsMetadata>(metadata))
                    {
                        numDigits = digitsMeta->digits;
                    }
                    else if (const auto unitsData = base::rtti_cast<base::PropertyUnitsMetadata>(metadata))
                    {
                        units = unitsData->text;
                    }
                }

                return base::CreateSharedPtr<DataBoxVector>(base::reflection::GetTypeObject<float>(), numComps, numDigits, rangeEnabled, rangeMin, rangeMax, dragEnabled, units, VECTOR_COMPS, verticalLayout);
            }
            else if (info.dataType == base::reflection::GetTypeObject<base::Point>())
            {
                bool rangeEnabled = false;
                bool dragEnabled = false;
                bool dragWrapEnabled = false;
                double rangeMin = -DBL_MAX;
                double rangeMax = DBL_MAX;
                base::StringBuf units = "";

                for (const auto* metadata : info.metadata)
                {
                    if (const auto dragMeta = base::rtti_cast<base::PropertyHasDragMetadata>(metadata))
                    {
                        dragEnabled = true;
                        dragWrapEnabled = dragMeta->wrap;
                    }
                    else if (const auto rangeMeta = base::rtti_cast<base::PropertyNumberRangeMetadata>(metadata))
                    {
                        rangeEnabled = true;
                        rangeMin = rangeMeta->min;
                        rangeMax = rangeMeta->max;
                    }
                    else if (const auto unitsData = base::rtti_cast<base::PropertyUnitsMetadata>(metadata))
                    {
                        units = unitsData->text;
                    }
                }

                return base::CreateSharedPtr<DataBoxVector>(base::reflection::GetTypeObject<int>(), 2, 0, rangeEnabled, rangeMin, rangeMax, dragEnabled, units, VECTOR_COMPS, verticalLayout);
            }
            else if (info.dataType == base::reflection::GetTypeObject<base::Angles>())
            {
                bool rangeEnabled = true;
                double rangeMin = -360.0f;
                double rangeMax = 360.0f;
                bool dragEnabled = true;
                bool dragWrapEnabled = true;
                base::StringBuf units = " deg";
                int numDigits = 2;

                for (const auto* metadata : info.metadata)
                {
                    if (const auto dragMeta = base::rtti_cast<base::PropertyHasDragMetadata>(metadata))
                    {
                        dragEnabled = true;
                        dragWrapEnabled = dragMeta->wrap;
                    }
                    else if (const auto rangeMeta = base::rtti_cast<base::PropertyNumberRangeMetadata>(metadata))
                    {
                        rangeEnabled = true;
                        rangeMin = rangeMeta->min;
                        rangeMax = rangeMeta->max;
                    }
                    else if (const auto digitsMeta = base::rtti_cast<base::PropertyPrecisionDigitsMetadata>(metadata))
                    {
                        numDigits = digitsMeta->digits;
                    }
                    else if (const auto unitsData = base::rtti_cast<base::PropertyUnitsMetadata>(metadata))
                    {
                        units = unitsData->text;
                    }
                }

                return base::CreateSharedPtr<DataBoxVector>(base::reflection::GetTypeObject<float>(), 3, numDigits, rangeEnabled, rangeMin, rangeMax, dragEnabled, units, ANGLE_COMPS, verticalLayout);
            }

            return nullptr;
        }
    };

    RTTI_BEGIN_TYPE_CLASS(DataBoxVectorFactory);
    RTTI_END_TYPE();

    //--


} // ui