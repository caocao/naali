// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "NoteCardModule.h"
#include "NoteCardManager.h"
#include "EventManager.h"
#include "SceneEvents.h"
#include "NetworkEvents.h"
#include "SceneManager.h"
#include "EC_NoteCard.h"
#include "ConsoleCommandServiceInterface.h"

namespace NoteCard
{
    std::string NoteCardModule::name_static_ = "NoteCardModule";
    
    NoteCardModule::NoteCardModule() :
        ModuleInterfaceImpl(name_static_),
        manager_(0),
        scene_event_category_(0),
        framework_event_category_(0),
        input_event_category_(0),
        network_state_event_category_(0)
    {
    }

    NoteCardModule::~NoteCardModule()
    {
    }
    
    void NoteCardModule::Load()
    {
        DECLARE_MODULE_EC(EC_NoteCard);
    }

    void NoteCardModule::Initialize()
    {
        manager_ = new NoteCardManager(GetFramework());
        event_manager_ = framework_->GetEventManager();
    }

    void NoteCardModule::PostInitialize()
    {
        RegisterConsoleCommand(Console::CreateCommand("NoteCardManager", 
            "Shows the notecard manager.",
            Console::Bind(this, &NoteCardModule::ShowWindow)));
        
        scene_event_category_ = event_manager_->QueryEventCategory("Scene");
        framework_event_category_ = event_manager_->QueryEventCategory("Framework");
        input_event_category_ = event_manager_->QueryEventCategory("Input");
    }

    void NoteCardModule::SubscribeToNetworkEvents()
    {
        network_state_event_category_ = event_manager_->QueryEventCategory("NetworkState");
    }

    void NoteCardModule::Uninitialize()
    {
        if (manager_)
        {
            manager_->deleteLater();
            manager_ = 0;
        }
    }

    void NoteCardModule::Update(f64 frametime)
    {
        RESETPROFILER;
        
        if (manager_)
            manager_->Update(frametime);
    }

    Console::CommandResult NoteCardModule::ShowWindow(const StringVector &params)
    {
        if (manager_)
            manager_->BringToFront();
        
        return Console::ResultSuccess();
    }
    
    bool NoteCardModule::HandleEvent(event_category_id_t category_id, event_id_t event_id, Foundation::EventDataInterface* data)
    {
        if (category_id == framework_event_category_)
        {
            if (event_id == Foundation::NETWORKING_REGISTERED)
                SubscribeToNetworkEvents();
            
            if (event_id == Foundation::WORLD_STREAM_READY)
            {
                ProtocolUtilities::WorldStreamReadyEvent *event_data = checked_static_cast<ProtocolUtilities::WorldStreamReadyEvent *>(data);
                if (event_data && manager_)
                    manager_->SetWorldStream(event_data->WorldStream);
            }
        }
        
        if (category_id == network_state_event_category_ && event_id == ProtocolUtilities::Events::EVENT_SERVER_DISCONNECTED)
        {
            if (manager_)
                manager_->ClearList();
        }
        
        if (category_id == scene_event_category_)
        {
            if (event_id == Scene::Events::EVENT_ENTITY_ECS_RECEIVED)
            {
                Scene::Events::SceneEventData* event_data = dynamic_cast<Scene::Events::SceneEventData*>(data);
                if (event_data && manager_)
                    manager_->OnEntityModified(event_data->localID);
            }
            if (event_id == Scene::Events::EVENT_ENTITY_DELETED)
            {
                Scene::Events::SceneEventData* event_data = dynamic_cast<Scene::Events::SceneEventData*>(data);
                if (data && manager_)
                    manager_->OnEntityRemoved(event_data->localID);
            }
            if (event_id == Scene::Events::EVENT_ENTITY_ADDED)
            {
                Scene::Events::SceneEventData* event_data = dynamic_cast<Scene::Events::SceneEventData*>(data);
                if (data && manager_)
                    manager_->OnEntityAdded(event_data->localID);
            }
        }
        return false;
    }

}

extern "C" void POCO_LIBRARY_API SetProfiler(Foundation::Profiler *profiler);
void SetProfiler(Foundation::Profiler *profiler)
{
    Foundation::ProfilerSection::SetProfiler(profiler);
}

using namespace NoteCard;

POCO_BEGIN_MANIFEST(Foundation::ModuleInterface)
    POCO_EXPORT_CLASS(NoteCardModule)
POCO_END_MANIFEST 

