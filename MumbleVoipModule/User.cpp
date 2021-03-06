// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "User.h"
#include "PCMAudioFrame.h"
#include "MumbleVoipModule.h"
#include "stdint.h"
#include "MumbleDefines.h"
#include "PCMAudioFrame.h"
#include <QTimer>
#include "Channel.h"
#include <mumbleclient/user.h>
#include <mumbleclient/channel.h>
#include "MemoryLeakCheck.h"

namespace MumbleVoip
{
    User::User(const MumbleClient::User& user, MumbleVoip::Channel* channel)
        : user_(user),
          speaking_(false),
          position_known_(false),
          position_(0,0,0),
          left_(false),
          channel_(channel),
          received_voice_packet_count_(0),
          voice_packet_drop_count_(0)
    {
        last_audio_frame_time_.start(); // initialize time state so that restart is possible later
    }

    User::~User()
    {
        foreach(PCMAudioFrame* audio, playback_queue_)
            SAFE_DELETE(audio);

        playback_queue_.clear();
    }

    QString User::Name() const
    {
        return QString(user_.name.c_str());
    }
    
    //QString User::Hash()
    //{
    //    return QString(user_.hash.c_str());
    //}
    
    int User::Session() const
    {
        return user_.session;
    }
    
    int User::Id() const
    {
        return user_.user_id;
    }
    
    Channel* User::Channel() const
    {
        return channel_;
    }
    
    QString User::Comment() const
    {
        return QString(user_.comment.c_str());
    }

    bool User::IsSpeaking() const
    {
        return speaking_;
    }

    void User::AddToPlaybackBuffer(PCMAudioFrame* frame)
    {
        received_voice_packet_count_++;
        if (PlaybackBufferLengthMs() > PLAYBACK_BUFFER_MAX_LENGTH_MS_)
        {
            foreach(PCMAudioFrame* frame, playback_queue_)
            {
                delete frame;
                voice_packet_drop_count_++;
            }
            playback_queue_.clear();
        }

        playback_queue_.push_back(frame);
        last_audio_frame_time_.restart();

        if (!speaking_)
        {
            speaking_ = true;
            emit StartReceivingAudio();
        }
    }

    void User::UpdatePosition(Vector3df position)
    {
        position_known_ = true;
        position_ = position;

        emit PositionUpdated();
    }

    Vector3df User::Position() const
    {
        return position_;
    }

    int User::PlaybackBufferLengthMs() const
    {
        return 1000 * playback_queue_.size() * SAMPLES_IN_FRAME / SAMPLE_RATE;
    }
    
    PCMAudioFrame* User::GetAudioFrame()
    {
        if (playback_queue_.size() == 0)
            return 0;

        return playback_queue_.takeFirst();
    }

    double User::VoicePacketDropRatio() const
    {
        if (received_voice_packet_count_ == 0)
            return 0;
        return static_cast<double>(voice_packet_drop_count_)/received_voice_packet_count_;
    }

    void User::CheckSpeakingState()
    {
        bool was_speaking = speaking_;

        if (speaking_ && last_audio_frame_time_.elapsed() > SPEAKING_TIMEOUT_MS)
        {
            speaking_ = false;
            if (was_speaking && !speaking_)
                emit StopReceivingAudio();
        }
    }

    void User::SetChannel(MumbleVoip::Channel* channel)
    {
        if (channel_ != channel)
        {
            channel_ = channel;
            emit ChangedChannel(this);
        }
    }

    int User::CurrentChannelID() const
    {
        if (left_)
            return -1;
        boost::shared_ptr<MumbleClient::Channel> mumble_channel = user_.channel.lock();
        if (mumble_channel)
            return user_.channel.lock()->id;
        else
            return -1;
    }

} // namespace MumbleVoip
