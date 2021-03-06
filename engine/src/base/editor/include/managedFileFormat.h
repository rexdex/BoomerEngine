/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: depot #]
***/

#pragma once

#include "base/io/include/absolutePath.h"
#include "versionControl.h"
#include "base/object/include/objectObserver.h"

namespace ed
{
    class ManagedFileFormatRegistry;

    ///----

    // output cooked format
    struct BASE_EDITOR_API ManagedFileCookableOutput : public NoCopy
    {
        base::SpecificClassType<base::res::IResource> resoureClass; // output resource class
        base::Array< base::SpecificClassType<base::res::IResourceManifest>> manifestClasses; // manifest files used during cooking
    };

    // resource tag for a managed file
    struct BASE_EDITOR_API ManagedFileTag
    {
        base::StringBuf name;
        base::Color color;
        bool baked = false;
    };

    // format information
    class BASE_EDITOR_API ManagedFileFormat : public NoCopy
    {
    public:
        ManagedFileFormat(StringView<char> extension);
        ~ManagedFileFormat();

        /// should we show this format in asset browser
        INLINE bool showInBrowser() const { return m_showInBrowser; }

        /// get the file extension
        INLINE const StringBuf& extension() const { return m_extension; }

        /// get the user readable description
        INLINE const StringBuf& description() const { return m_description; }

        /// get the name of the resource class linked to this format (if known)
        /// NOTE: this is set only for engine-serialized resources as non-engine files may be transformed into many different resource types
        INLINE SpecificClassType<res::IResource> nativeResourceClass() const { return m_nativeResourceClass; }

        /// do we have custom type thumbnail ?
        INLINE bool hasTypeThumbnail() const { return m_hasTypeThumbnail; }

        /// can this format be created by user ?
        INLINE bool canUserCreate() const { return !m_factory.empty(); }

        /// what kind of classes can we cook from this file format, lists all cookers
        /// NOTE: even for native files we may be able to cook something out of them that is of difference class
        INLINE const Array<ManagedFileCookableOutput>& cookableOutputs() const { return m_cookableOutputs; }

        /// get file format tags
        INLINE const Array<ManagedFileTag>& tags() const { return m_tags; }

        //--

        /// create empty resource
        base::res::ResourcePtr createEmpty() const;

        //--

        // is this file lodable/cookable as given type ?
        bool loadableAsType(base::ClassType resourceClass) const;

        /// get the default thumbnail image for this file type
        const image::ImageRef& thumbnail() const;

    private:
        StringBuf m_extension;
        StringBuf m_description;
        mutable image::ImageRef m_thumbnail;

        SpecificClassType<res::IResource> m_nativeResourceClass;
        Array<ManagedFileCookableOutput> m_cookableOutputs;

        Array<ManagedFileTag> m_tags;

        base::RefPtr<base::res::IFactory> m_factory;

        bool m_showInBrowser = false;
        mutable bool m_hasTypeThumbnail = false;
        mutable bool m_thumbnailLoadAttempted = false;

        friend class ManagedFileFormatRegistry;
    };

    ///----

    // registry of file formats
    class BASE_EDITOR_API ManagedFileFormatRegistry : public ISingleton
    {
        DECLARE_SINGLETON(ManagedFileFormatRegistry);

    public:
        ManagedFileFormatRegistry();

        //--

        /// get list of formats creatable by the user
        INLINE const Array<ManagedFileFormat*>& creatableFormats() const { return m_userCreatableFormats; }

        /// get list of all formats
        INLINE const Array<ManagedFileFormat*>& allFormats() const { return m_allFormats; }

        //--

        /// cache formats
        void cacheFormats();

        /// get file format description for given extension
        /// NOTE: we always return a valid object here (except for empty extension)
        const ManagedFileFormat* format(StringView<char> extension);

    private:
        SpinLock m_lock;

        Array<ManagedFileFormat*> m_allFormats;
        Array<ManagedFileFormat*> m_userCreatableFormats;            

        HashMap<uint64_t, ManagedFileFormat*> m_formatMap;

        virtual void deinit() override;
    };

    ///----

} // depot

