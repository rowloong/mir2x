/*
 * =====================================================================================
 *
 *       Filename: inventoryboard.cpp
 *        Created: 10/08/2017 19:22:30
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

#include "pngtexdb.hpp"
#include "sdldevice.hpp"
#include "processrun.hpp"
#include "inventoryboard.hpp"

extern PNGTexDB *g_progUseDB;
extern PNGTexDB *g_progUseDB;
extern PNGTexDB *g_commonItemDB;
extern SDLDevice *g_SDLDevice;

InventoryBoard::InventoryBoard(int nX, int nY, ProcessRun *pRun, Widget *pwidget, bool bAutoFree)
    : Widget(nX, nY, 0, 0, pwidget, bAutoFree)
    , m_goldBoard
      {
          0, // reset by new width
          0,
          "0",

          1,
          12,
          0,

          colorf::RGBA(0XFF, 0XFF, 0X00, 0X00),
          this,
      }
    , m_closeButton
      {
          242,
          422,
          {SYS_TEXNIL, 0X0000001C, 0X0000001D},

          nullptr,
          [this]()
          {
              show(false);
          },

          0,
          0,
          0,
          0,

          true,
          this,
      }
    , m_processRun(pRun)
{
    show(false);
    if(auto pTexture = g_progUseDB->Retrieve(0X0000001B)){
        SDL_QueryTexture(pTexture, nullptr, nullptr, &m_w, &m_h);
    }
    m_goldBoard.moveTo(105 - m_goldBoard.w() / 2, 401);
}

void InventoryBoard::drawItem(int nDstX, int nDstY, const PackBin &rstBin)
{
    if(true
            && rstBin
            && rstBin.X >= 0
            && rstBin.Y >= 0
            && rstBin.W >  0
            && rstBin.H >  0){

        if(auto pTexture = g_commonItemDB->Retrieve(rstBin.ID)){

            int nItemPW = -1;
            int nItemPH = -1;
            if(!SDL_QueryTexture(pTexture, nullptr, nullptr, &nItemPW, &nItemPH)){

                const int nInvGridX0 = 18;
                const int nInvGridY0 = 59;

                g_SDLDevice->DrawTexture(pTexture, 
                        nDstX + nInvGridX0 + rstBin.X * SYS_INVGRIDPW + (rstBin.W * SYS_INVGRIDPW - nItemPW) / 2,
                        nDstY + nInvGridY0 + rstBin.Y * SYS_INVGRIDPH + (rstBin.H * SYS_INVGRIDPH - nItemPH) / 2);
            }
        }
    }
}

void InventoryBoard::drawEx(int nDstX, int nDstY, int, int, int, int)
{
    if(auto pTexture = g_progUseDB->Retrieve(0X0000001B)){
        g_SDLDevice->DrawTexture(pTexture, nDstX, nDstY);
    }

    if(auto pMyHero = m_processRun->GetMyHero()){
        m_goldBoard.setText("%s", getGoldStr().c_str());
        m_goldBoard.moveTo(105 - m_goldBoard.w() / 2, 401);

        // 2. draw all items
        for(auto &rstBin: pMyHero->getInvPack().GetPackBinList()){
            drawItem(nDstX, nDstY, rstBin);
        }
    }

    m_goldBoard.draw();
    m_closeButton.draw();
}

bool InventoryBoard::processEvent(const SDL_Event &event, bool valid)
{
    if(!valid){
        return false;
    }

    if(!show()){
        return false;
    }

    if(m_closeButton.processEvent(event, valid)){
        return true;
    }

    switch(event.type){
        case SDL_MOUSEMOTION:
            {
                if(in(event.motion.x, event.motion.y) && (event.motion.state & SDL_BUTTON_LMASK)){
                    moveBy(event.motion.xrel, event.motion.yrel);
                    return true;
                }
                return false;
            }
        default:
            {
                return false;
            }
    }
}

std::string InventoryBoard::getGoldStr() const
{
    std::string result;
    std::string goldStr = std::to_string([this]() -> int
    {
        if(auto p = m_processRun->GetMyHero()){
            return p->GetGold();
        }
        return 0;
    }());

    std::reverse(goldStr.begin(), goldStr.end());
    for(size_t i = 0; i < goldStr.size(); ++i){
        result.push_back(goldStr[i]);
        if(i % 3 == 2){
            result.push_back(',');
        }
    }

    if(result.back() == ','){
        result.pop_back();
    }

    std::reverse(result.begin(), result.end());
    return result;
}
