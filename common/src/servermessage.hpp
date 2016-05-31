/*
 * =====================================================================================
 *
 *       Filename: servermessage.hpp
 *        Created: 01/24/2016 19:30:45
 *  Last Modified: 05/30/2016 18:08:09
 *
 *    Description: net message used by client and mono-server
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

enum: uint8_t
{
    SM_OK,
    SM_ERROR,
    SM_REFUSE,
    SM_PING,
    SM_LOGINOK,
    SM_LOGINFAIL,
    SM_SERVERFULL,
};

#pragma pack(push, 1)

// TODO
// for this kind of package with str, enable compress
// 1. send byte HC =  SMLoginOK
// 2. send byte Compress flag:
//      0: uncompressed
//  not 0: compressed, then check the length
//          0X80 & byte == 0: length = (0X7F & byte)
//                      != 0: length = (length << 7) + (0X7F & byte)
// 3. read message body
//
// currently just always use uncompressed package
typedef struct{
    uint32_t GUID;
    uint32_t JobID;
    uint32_t Level;
    uint32_t MapX;
    uint32_t MapY;
    uint32_t Direction;
}SMLoginOK;
#pragma pack(pop)
