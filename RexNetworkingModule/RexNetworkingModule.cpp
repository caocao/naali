// For conditions of distribution and use, see copyright notice in license.txt

//#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "Framework.h"
#include "ConfigurationManager.h"
#include "EventManager.h"
#include "SessionManager.h"

#include "RexNetworkingModule.h"


namespace RexNetworking
{
    enum
    {
        REXNETWORKING_NULL,
        REXNETWORKING_INITIALIZED,
        REXNETWORKING_EVENTS_REGISTERED
    };

    Real LL_THROTTLE_MAX_BPS;

    RexNetworkingModule::RexNetworkingModule()		
        : ModuleInterfaceImpl(Foundation::Module::MT_Networking),
        local_state_ (REXNETWORKING_NULL)

    {
    }

    RexNetworkingModule::~RexNetworkingModule()
    {
    }

    void RexNetworkingModule::Initialize()
    {
        using std::auto_ptr;

        LL_THROTTLE_MAX_BPS = framework_->GetDefaultConfig().DeclareSetting
            ("RexLogicModule", "max_bits_per_second", 1000000.0f);
    
        // Create local session handlers
        LLSessionHandler *llhandler = new LLSessionHandler;
        
        // Register handlers with session manager
        framework_-> GetSessionManager()-> 
            Register (auto_ptr <Foundation::SessionHandler> (llhandler), "OpenSim/LLUDP");

        // Get valid session objects so streams can be pumped
        active_.push_back (llhandler-> GetSession());

        local_state_ = REXNETWORKING_INITIALIZED;
    }

    void RexNetworkingModule::PostInitialize()
    {
    }

    void RexNetworkingModule::Update(f64 frametime)
    {
        if (local_state_ == REXNETWORKING_INITIALIZED)
        {
            // Register event categories.
            GetFramework()->GetEventManager()->RegisterEventCategory("NetworkState");

            // Send event that other modules can query above
            GetFramework()->GetEventManager()->SendEvent
                (GetFramework()->GetEventManager()->QueryEventCategory("Framework"), Foundation::NETWORKING_REGISTERED, 0);

            LogInfo("Network events [NetworkState] registered");

            local_state_ = REXNETWORKING_EVENTS_REGISTERED;
        }

        Foundation::Session *session;
        SessionList::iterator i (active_.begin());
        SessionList::iterator e (active_.end());
        for (; i != e; ++i)
        {
            session = *i;
            if (session-> IsConnected())
                session-> GetStream().Pump();
        }
    }
    
    void RexNetworkingModule::Uninitialize()
    {
    }

}


#include <Poco/ClassLibrary.h>

extern "C" void POCO_LIBRARY_API SetProfiler(Foundation::Profiler *profiler);
void SetProfiler(Foundation::Profiler *profiler)
{
    Foundation::ProfilerSection::SetProfiler(profiler);
}

using namespace RexNetworking;

    POCO_BEGIN_MANIFEST(Foundation::ModuleInterface)
POCO_EXPORT_CLASS(RexNetworkingModule)
    POCO_END_MANIFEST