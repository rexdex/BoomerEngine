/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: bitmapFont #]
***/

#pragma once

#include "base/resources/include/resource.h"

namespace base
{
    namespace font
    {

        /// char set
        struct BASE_FONT_API BitmapFontCharset
        {
            RTTI_DECLARE_NONVIRTUAL_CLASS(BitmapFontCharset);

        public:
            bool enabled = false;
            uint32_t firstCharCode = 0;
            uint32_t lastCharCode = 0;

            void collectCharCodes(base::HashSet<uint32_t>& outCharCodes) const;
        };

        /// manifest for cooking textures
        class BASE_FONT_API BitmapFontManifest : public base::res::IResourceManifest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(BitmapFontManifest, base::res::IResourceManifest);

        public:
            BitmapFontManifest();

            //--

            base::StringBuf m_sourceFile;

            //--

            uint32_t m_size = 12;
            bool m_bold = false;
            bool m_italic = false;

            //--

            BitmapFontCharset m_setASCII;

            base::Array<BitmapFontCharset> m_additionalCharSets;

            //--

            void collectCharCodes(base::Array<uint32_t>& outCharCodes) const;
        };

    } // font
} // base
