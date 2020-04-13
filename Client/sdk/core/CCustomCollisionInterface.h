/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        sdk/core/CCustomCollisionInterface.h
 *  PURPOSE:     Per element custom collision class interface
 *
 *****************************************************************************/
#pragma once

class CCustomCollisionInterface
{
public:
    virtual class CColModelSAInterface* GetCustomCollision(class CObject* pObject) = 0;
};
