/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
***/

#pragma once

#include "base/depot/include/depotStructure.h"
#include "base/app/include/localService.h"

namespace rendering
{
    //---

    class MaterialTechniqueCompiler;

    // local service for compiling techniques for materials, only reason it's a service is that we need to observer the depot
    class  MaterialTechniqueCacheService : public base::app::ILocalService, public base::depot::IDepotObserver
    {
        RTTI_DECLARE_VIRTUAL_CLASS(MaterialTechniqueCacheService, base::app::ILocalService);

    public:
        MaterialTechniqueCacheService();
        virtual ~MaterialTechniqueCacheService();

        /// request compilation of given material technique
        void requestTechniqueCompilation(base::StringView<char> contextName, const MaterialGraphContainerPtr& graph, MaterialTechnique* technique);

    protected:
        // ILocalService
        virtual base::app::ServiceInitializationResult onInitializeService(const base::app::CommandLine& cmdLine) override final;
        virtual void onShutdownService() override final;
        virtual void onSyncUpdate() override final;

        //---

        static uint64_t CalcMergedKey(base::StringView<char> contextName, uint64_t graphKey, const MaterialCompilationSetup& setup);

        //---

        base::depot::DepotStructure* m_depot;

        //---

        struct TechniqueInfo
        {
            uint64_t key = 0;

            base::RefWeakPtr<MaterialTechnique> technique; // may be lost
            base::StringBuf contextName;
            MaterialGraphContainerPtr graph;

            ~TechniqueInfo();
        };

        base::Mutex m_allTechniquesMapLock;
        //base::HashMap<uint64_t, TechniqueInfo*> m_allTechniquesMap;
        base::Array<TechniqueInfo*> m_allTechniques;

        void requestTechniqueCompilation(TechniqueInfo* info);
        void processTechniqueCompilation(TechniqueInfo* owner, MaterialTechniqueCompiler& compiler);

        //--

        struct FileInfo
        {
            uint64_t crc = 0;
            uint64_t timestamp = 0;
            base::StringBuf depotPath;

            base::SpinLock usersLock;
            base::HashSet<TechniqueInfo*> users;
        };

        base::Mutex m_sourceFileMapLock;
        base::HashMap<base::StringBuf, FileInfo*> m_sourceFileMap;

        base::HashSet<FileInfo*> m_changedFiles;
        base::Mutex m_changedFilesLock;

        FileInfo* getFileInfo(base::StringView<char> depotPath);

        //--

        base::Array<MaterialTechniqueCompiler*> m_compilationJobs;
        base::Mutex m_compilationJobsLock;

        //---

        virtual void notifyFileChanged(base::StringView<char> depotFilePath) override final;
        virtual void notifyFileAdded(base::StringView<char> depotFilePath) override final;
        virtual void notifyFileRemoved(base::StringView<char> depotFilePath) override final;
    };

    //---

} // rendering

