/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: messages #]
***/

#pragma once

#include "base/memory/include/linearAllocator.h"
#include "base/containers/include/hashMap.h"
#include "base/replication/include/replicationDataModel.h"

namespace base
{
    namespace net
    {

        //--

        /// sink for the messages generated by te knowledge updated
        class IKnowledgeUpdaterSink : public NoCopy
        {
        public:
            virtual ~IKnowledgeUpdaterSink();

            /// a new string was given ID was added to the DB
            virtual void reportNewString(const replication::DataMappedID id, StringView<char> txt) = 0;

            /// a new path was given ID and added to the DB
            virtual void reportNewPath(const replication::DataMappedID id, const replication::DataMappedID textId, const replication::DataMappedID parentPathId) = 0;
        };

        //--

        /// knowledge updater - maps requested data to shared indices, generates update messages
        /// TODO: maybe we should use some "message sink" instead of directly writing to TCP socket :P
        class KnowledgeUpdater : public replication::IDataModelMapper
        {
        public:
            KnowledgeUpdater(MessageKnowledgeBase& base, IKnowledgeUpdaterSink* sink, MessageObjectRepository& objectRepository);

            virtual replication::DataMappedID mapString(StringView<char> txt) override final;
            virtual replication::DataMappedID mapPath(StringView<char> path, const char* pathSeparators) override final;
            virtual replication::DataMappedID mapObject(const IObject* obj) override final;

        private:
            MessageKnowledgeBase& m_knowledge;
            IKnowledgeUpdaterSink* m_updateSink;
            MessageObjectRepository& m_objectRepository;
        };

        //--

        /// knowledge based data resolver
        class KnowledgeResolver : public replication::IDataModelResolver
        {
        public:
            KnowledgeResolver(const MessageKnowledgeBase& base, const MessageObjectRepository& objectRepository);

            virtual bool resolveString(const replication::DataMappedID id, IFormatStream& f) override final;
            virtual bool resolvePath(const replication::DataMappedID id, const char* pathSeparator, IFormatStream& f) override final;
            virtual bool resolveObject(const replication::DataMappedID id, ObjectPtr& outObject) override final;

        private:
            const MessageKnowledgeBase& m_knowledge;
            const MessageObjectRepository& m_objectRepository;
        };

        //--

    } // msg
} // base