/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: resource\static #]
***/

#include "build.h"
#include "resource.h"
#include "resourceStaticResource.h"
#include "resourceLoader.h"

namespace base
{
    namespace res
    {

        //--

        namespace prv
        {
            class StaticResourceRegistry : public ISingleton
            {
                DECLARE_SINGLETON(StaticResourceRegistry);

            public:
                INLINE StaticResourceRegistry()
                    : m_loader(nullptr)
                    , m_listHead(nullptr)
                {
                }

                INLINE void registerStaticResource(IStaticResource* resource)
                {
                    auto lock = CreateLock(m_lock);

                    if (m_listHead)
                        m_listHead->m_prev = resource;
                    resource->m_next = m_listHead;
                    m_listHead = resource;
                }

                INLINE void unregisterStaticResource(IStaticResource* resource)
                {
                    auto lock = CreateLock(m_lock);

                    if (resource->m_prev != nullptr)
                        resource->m_prev->m_next = resource->m_next;
                    else
                        m_listHead = resource->m_next;

                    if (resource->m_next != nullptr)
                        resource->m_next->m_prev = resource->m_prev;
                }

                INLINE void collect(Array<IStaticResource*>& outResources)
                {
                    auto lock = CreateLock(m_lock);

                    for (auto cur  = m_listHead; cur; cur = cur->m_next)
                        outResources.pushBack(cur);
                }

                INLINE void reload(res::IResource* currentResource, res::IResource* newResource)
                {
                    auto lock = CreateLock(m_lock);

                    for (auto cur = m_listHead; cur; cur = cur->m_next)
                        cur->reload(currentResource, newResource);
                }

                INLINE void loader(IResourceLoader* loader)
                {
                    if (m_loader != loader)
                    {
                        // unload all resources
                        auto lock = CreateLock(m_lock);
                        for (auto cur  = m_listHead; cur; cur = cur->m_next)
                            cur->unload();

                        // change loader
                        m_loader = loader;
                    }
                }

                INLINE ResourceHandle load(ClassType resourceClass, const ResourcePath& path)
                {
                    ASSERT(!path.empty());

                    if (!m_loader)
                    {
                        TRACE_ERROR("Trying to load static resource '{}' without global static loader", path);
                        return nullptr;
                    }

                    ResourceKey key(path, resourceClass.cast<IResource>());
                    return m_loader->loadResource(key);
                }

            private:
                SpinLock m_lock;
                IStaticResource* m_listHead;
                IResourceLoader* m_loader;

                virtual void deinit() override
                {
                    // unload all resources
                    auto lock = CreateLock(m_lock);
                    for (auto cur  = m_listHead; cur; cur = cur->m_next)
                        cur->unload();
                }
            };
        } // prv

        IStaticResource::IStaticResource(const char* path, bool preload)
            : m_path(path)
            , m_isPreloaded(preload)
            , m_next(nullptr)
            , m_prev(nullptr)
        {
            ASSERT_EX(m_path != nullptr && *m_path != 0, "Static resource must always have valid path");
            prv::StaticResourceRegistry::GetInstance().registerStaticResource(this);
        }

        IStaticResource::~IStaticResource()
        {
            prv::StaticResourceRegistry::GetInstance().unregisterStaticResource(this);
        }

        void IStaticResource::unload()
        {
            if (m_handle)
            {
                TRACE_INFO("Unloaded static resource '{}'", m_path);
                m_handle.reset();
            }
        }

        void IStaticResource::reload(res::IResource* currentResource, res::IResource* newResource)
        {
            auto lock = CreateLock(m_lock);
            if (m_handle == currentResource)
            {
                m_handle = res::ResourcePtr(AddRef(newResource));
                TRACE_INFO("Reloaded static resource '{}'", m_path);
            }
        }

        ResourceHandle IStaticResource::loadAndGet()
        {
            {
                auto lock = CreateLock(m_lock);
                if (m_handle)
                    return m_handle;
            }

            auto path = ResourcePath(m_path);
            auto handle = prv::StaticResourceRegistry::GetInstance().load(resourceClass(), path);

            if (handle)
            {
                auto lock = CreateLock(m_lock);
                m_handle = handle;
            }

            return handle;
        }

        void IStaticResource::BindGlobalLoader(IResourceLoader* loader)
        {
            prv::StaticResourceRegistry::GetInstance().loader(loader);
        }

        void IStaticResource::CollectAllResources(Array<IStaticResource*>& outResources)
        {
            prv::StaticResourceRegistry::GetInstance().collect(outResources);
        }

        void IStaticResource::ApplyReload(res::IResource* currentResource, res::IResource* newResource)
        {
            prv::StaticResourceRegistry::GetInstance().reload(currentResource, newResource);
        }

    } // res
} // base

