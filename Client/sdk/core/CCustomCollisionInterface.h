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

class CEntitySAInterface;
class CCustomCollisionInterface
{
public:
    virtual class CColModelSAInterface* GetCustomCollision(class CObject* pObject) = 0;
    virtual void                        SetObjectScale(CEntitySAInterface* pEntitySA, CVector scale) = 0;
    virtual bool                        SetObjectCollision(CEntitySAInterface* pEntitySA, CColModelSAInterface* pColModelSA) = 0;
    virtual bool                        HasCustomCollision(CEntitySAInterface* pEntitySA) = 0;
};
