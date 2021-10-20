/*
 * =====================================================================================
 *
 *       Filename: processlogin.hpp
 *        Created: 08/14/2015 02:47:30 PM
 *    Description:
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */
#pragma once

#include <cstdint>
#include <SDL2/SDL.h>

#include "strf.hpp"
#include "totype.hpp"
#include "process.hpp"
#include "message.hpp"
#include "inputline.hpp"
#include "labelboard.hpp"
#include "passwordbox.hpp"
#include "notifyboard.hpp"
#include "tritexbutton.hpp"

class ProcessLogin: public Process
{
    private:
        TritexButton    m_button1;
        TritexButton    m_button2;
        TritexButton    m_button3;
        TritexButton    m_button4;

        InputLine       m_idBox;
        PasswordBox     m_passwordBox;

    private:
        LabelBoard m_buildSignature;

    private:
        NotifyBoard m_notifyBoard;

    public:
        ProcessLogin();
        virtual ~ProcessLogin() = default;

    public:
        int ID() const override
        {
            return PROCESSID_LOGIN;
        }

    public:
        void update(double) override;
        void draw() override;
        void processEvent(const SDL_Event &) override;

    private:
        void doLogin();
        void doCreateAccount();

    public:
        void addLog(const char8_t *format, ...)
        {
            std::u8string s;
            str_format(format, s);
            m_notifyBoard.addLog(u8"%s", to_cstr(s));
        }
};
