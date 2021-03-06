// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_MumbleVoipModule_Provider_h
#define incl_MumbleVoipModule_Provider_h

#include <QObject>
#include "CommunicationsService.h"
#include "ServerInfo.h"

namespace Foundation
{
    class Framework;
    class EventDataInterface;
}

namespace MumbleVoip
{
    class ServerInfoProvider;

    namespace InWorldVoice
    {
        class Session;

        class Provider : public Communications::InWorldVoice::ProviderInterface
        {
            Q_OBJECT
        public:
            Provider(Foundation::Framework* framework);
            virtual ~Provider();
            virtual Communications::InWorldVoice::SessionInterface* Session();
            virtual QString& Description();
            virtual void Update(f64 frametime);
            virtual bool HandleEvent(event_category_id_t category_id, event_id_t event_id, Foundation::EventDataInterface* data);
        private:
            void CloseSession();
            Foundation::Framework* framework_;
            QString description_;
            MumbleVoip::InWorldVoice::Session* session_;

            //! \todo Use shared ptr ...
            //Session* session_;
            
            ServerInfoProvider* server_info_provider_;
            ServerInfo* server_info_;
            event_category_id_t networkstate_event_category_;

        private slots:
            void OnMumbleServerInfoReceived(ServerInfo info);
        };

    } // InWorldVoice

} // MumbleVoip

#endif // incl_MumbleVoipModule_Provider_h
