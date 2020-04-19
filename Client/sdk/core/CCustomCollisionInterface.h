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
class CColModelSAInterface;
class CObject;
class CCustomCollisionInterface
{
public:
    virtual CColModelSAInterface*       GetCurrentCollision(CObject* pObject, bool bSkipScaled = false) = 0;
    virtual CColModelSAInterface*       GetCurrentCollision(CEntitySAInterface* pObject, bool bSkipScaled = false) = 0;
    virtual void                        SetObjectScale(CObject* pObject, CVector scale) = 0;
    virtual bool                        SetObjectCollision(CEntitySAInterface* pEntitySA, CColModelSAInterface* pColModelSA) = 0;
    virtual bool                        SetObjectScaledCollision(CEntitySAInterface* pEntitySA, CColModelSAInterface* pColModelSA) = 0;
    virtual bool                        HasCustomCollision(CEntitySAInterface* pEntitySA) = 0;
    virtual bool                        RemoveCustomCollision(CEntitySAInterface* pEntitySA) = 0;
    virtual void                        DrawCollisionDebug(CObject* pObject) = 0;
};
