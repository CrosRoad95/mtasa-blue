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
class CColModel;
class CCustomCollisionInterface
{
public:
    virtual CColModel*                  GetCurrentCollision(CObject* pObject, bool bSkipScaled = false) = 0;
    virtual void                        SetObjectScale(CObject* pObject, CVector scale) = 0;
    virtual bool                        SetObjectCollision(CColModel* pEntitySA, CColModel* pColModelSA) = 0;
    virtual bool                        SetObjectScaledCollision(CColModel* pEntitySA, CColModel* pColModelSA) = 0;
    virtual bool                        HasCustomCollision(CColModel* pEntitySA) = 0;
    virtual bool                        RemoveCustomCollision(CColModel* pEntitySA) = 0;
    virtual void                        DrawCollisionDebug(CObject* pObject) = 0;
};
