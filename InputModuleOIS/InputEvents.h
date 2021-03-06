// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_Input_InputEvents_h
#define incl_Input_InputEvents_h

#include "EventDataInterface.h"

namespace Input
{
    //! Contains some fo the input events in generic fashion. Does not depend on any one input method
    /*!
        To add new event, first add event id. For continuos events declare a 'start' and 'end' event ids.
        
        \ingroup Input_group
    */
    namespace Events
    {
        // Note: Continous events should go in pairs and in sequence; for every
        //       'start' event (MOVE_FORWARD_PRESSED) there should be an
        //       'end' event (MOVE_FORWARD_RELEASED) and (MOVE_FORWARD_PRESSED == MOVE_FORWARD_RELEASED - 1)
        //       Yhis is for simplicity, so one can get the 'end' event easily from the 'start' event.
        //! Sent as notification for buffered keyboard input, when any key is pressed. Uses BufferedKey event data class.
        static const event_id_t BUFFERED_KEY_PRESSED = 1;  // only when input state is buffered
        //! Sent as notification for buffered keyboard input, when any key is released. Uses BufferedKey event data class.
        static const event_id_t BUFFERED_KEY_RELEASED = 2; // only when input state is buffered
        //! Generic scroll input event. Uses SingleAxisMovement event data class.
        static const event_id_t SCROLL = 3;
        //! Generic move forward event
        static const event_id_t MOVE_FORWARD_PRESSED = 4;
        static const event_id_t MOVE_FORWARD_RELEASED = 5;
        //! Generic move backward event
        static const event_id_t MOVE_BACK_PRESSED = 6;
        static const event_id_t MOVE_BACK_RELEASED = 7;
        //! Generic move left event
        static const event_id_t MOVE_LEFT_PRESSED = 8;
        static const event_id_t MOVE_LEFT_RELEASED = 9;
        //! Generic move right event
        static const event_id_t MOVE_RIGHT_PRESSED = 10;
        static const event_id_t MOVE_RIGHT_RELEASED = 11;
        //! Generic move up event
        static const event_id_t MOVE_UP_PRESSED = 12;
        static const event_id_t MOVE_UP_RELEASED = 13;
        //! Generic move down event
        static const event_id_t MOVE_DOWN_PRESSED = 14;
        static const event_id_t MOVE_DOWN_RELEASED = 15;
        //! Generic rotate left event
        static const event_id_t ROTATE_LEFT_PRESSED = 16;
        static const event_id_t ROTATE_LEFT_RELEASED = 17;
        //! Generic rotate right event
        static const event_id_t ROTATE_RIGHT_PRESSED = 18;
        static const event_id_t ROTATE_RIGHT_RELEASED = 19;
        //! Generic rotate up event
        static const event_id_t ROTATE_UP_PRESSED = 20;
        static const event_id_t ROTATE_UP_RELEASED = 21;
        //! Generic rotate down event
        static const event_id_t ROTATE_DOWN_PRESSED = 22;
        static const event_id_t ROTATE_DOWN_RELEASED = 23;
        //! Generic roll left event
        static const event_id_t ROLL_LEFT_PRESSED = 24;
        static const event_id_t ROLL_LEFT_RELEASED = 25;
        //! Generic roll right event
        static const event_id_t ROLL_RIGHT_PRESSED = 26;
        static const event_id_t ROLL_RIGHT_RELEASED = 27;

        //! Input event for showing the debug console
        static const event_id_t SHOW_DEBUG_CONSOLE = 28;
        static const event_id_t SHOW_DEBUG_CONSOLE_REL = 29;

        //! Input event for switching camera states
        static const event_id_t SWITCH_CAMERA_STATE = 30;
        static const event_id_t SWITCH_CAMERA_STATE_REL = 31;

        //! Mouse dragged input event. Not used currently.
        static const event_id_t MOUSELOOK = 32;
        static const event_id_t MOUSELOOK_STOPPED = 33;

        //! Input event for changing input state
        static const event_id_t INPUTSTATE_FIRSTPERSON = 34;
        static const event_id_t INPUTSTATE_THIRDPERSON = 35;
        static const event_id_t INPUTSTATE_FREECAMERA = 36;
        
        //! switch to fly mode
        static const event_id_t TOGGLE_FLYMODE = 37;
        static const event_id_t TOGGLE_FLYMODE_REL = 38;

        //! Custom key released input event. Can be used to catch any key on the keyboard, contains optional modifier key.
        //! Uses Key event data class.
        static const event_id_t KEY_PRESSED = 39;
        static const event_id_t KEY_RELEASED = 40;

        //! user clicked mouse over the viewport, usable with f.ex. raycasting. Movement event data class is used.
        static const event_id_t INWORLD_CLICK = 41;
        static const event_id_t INWORLD_CLICK_REL = 42;

        //! The following mouse click events are only used by the Python API at the moment!
        static const event_id_t LEFT_MOUSECLICK_PRESSED = 43;
        static const event_id_t LEFT_MOUSECLICK_RELEASED = 44;
        static const event_id_t RIGHT_MOUSECLICK_PRESSED = 45;
        static const event_id_t RIGHT_MOUSECLICK_RELEASED = 46;

        /// Test events for rezzing prims.
        static const event_id_t INWORLD_CLICK_BUILD = 47;
        static const event_id_t INWORLD_CLICK_BUILD_REL = 48;

        //! Event for buffered key input. 
        /*! Do not use for any continous input such as avatar movement, it will probably cause input lag
            General use case is text input.

            \ingroup Input_group
        */
        class BufferedKey : public Foundation::EventDataInterface
        {
            BufferedKey();
        public:
            BufferedKey(int code, uint text) : code_(code), text_(text)  {}
            virtual ~BufferedKey() {}
        
            //! OIS::KeyCode, include OISKeyboard.h for the codes
            const int code_;
            //! The key that was pressed as a character
            const uint text_;
        };

        //! Raw keycode event for unbuffered input
        /*! Avoid using unless you are sure this is what you should be using.

            \ingroup Input_group
        */
        class Key : public Foundation::EventDataInterface
        {
            Key();
        public:
            Key(int code, int modifiers) : code_(code), modifiers_(modifiers)  {}
            virtual ~Key() {}
        
            //! OIS::KeyCode, include OISKeyboard.h for the codes
            const int code_;
            
            //! OIS modifiers (OIS::Keyboard::Ctrl, OIS::Keyboard::Shift, OIS::Keyboard::Alt)
            const int modifiers_;
        };

        //! Movement along axis. \ingroup Input_group
        struct Axis
        {
            Axis() : rel_(0), abs_(0) {}
            Axis(const Axis &other) : rel_(other.rel_), abs_(other.abs_) {}
            ~Axis() {}

            Axis &operator = (const Axis &other)
            {
                if (this != &other)
                {
                    rel_ = other.rel_;
                    abs_ = other.abs_;
                }
                return *this;
            }
            bool operator == (const Axis &other) const { return (rel_ == other.rel_ && abs_ == other.abs_); }
            bool operator != (const Axis &other) const { return !(*this == other); }


            //! relative movement
            int rel_;
            //! absolute movement
            int abs_;
        };

        //! input position that contains both relative and absolute movement for single axis. \ingroup Input_group
        class SingleAxisMovement : public Foundation::EventDataInterface
        {
        public:
            SingleAxisMovement() {}
            SingleAxisMovement(const SingleAxisMovement &other) : z_(other.z_) {}
            ~SingleAxisMovement() {}
            SingleAxisMovement &operator = (const SingleAxisMovement &other)
            {
                if (this != &other)
                {
                    z_ = other.z_;
                }
                return *this;
            }
            bool operator == (const SingleAxisMovement &other) const { return (z_ == other.z_); }
            bool operator != (const SingleAxisMovement &other) const { return !(*this == other); }

            Axis z_;
        };

        //! input position that contains both relative and absolute movement for several axis. \ingroup Input_group
        class Movement : public Foundation::EventDataInterface
        {
        public:
            Movement() {}
            Movement(const Movement &other) : x_(other.x_), y_(other.y_), z_(other.z_) {}
            ~Movement() {}
            Movement &operator = (const Movement &other)
            {
                if (this != &other)
                {
                    x_ = other.x_;
                    y_ = other.y_;
                    z_ = other.z_;
                }
                return *this;
            }
            bool operator == (const Movement &other) const { return (x_ == other.x_ && y_ == other.y_ && z_ == other.z_); }
            bool operator != (const Movement &other) const { return !(*this == other); }

            Axis x_;
            Axis y_;
            Axis z_;
        };
    }
}

#endif
