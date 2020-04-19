/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        core/CCustomCollision.cpp
 *  PURPOSE:     Per element custom collision class
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/
#pragma once

#include <core/CCustomCollisionInterface.h>

#include "../game_sa/CColModelSA.h"

struct SCustomCollision
{
    CColModelSAInterface* pCol;
};
class OriginalCollisions
{
public:
    CColSphereSA*     spheres;
    CColBoxSA*        boxes;
    CompressedVector* vertices;
    ushort            spheresCount;
    ushort            boxesCount;
    ushort            verticesCount;
    CBoundingBoxSA    boundingBox;
};

class ScaledCollision
{
public:
    CVector*          scale;
    CColSphereSA*     spheres;
    CColBoxSA*        boxes;
    CompressedVector* vertices;
    ushort            spheresCount;
    ushort            boxesCount;
    ushort            verticesCount;
    CBoundingBoxSA    boundingBox;
};

class CCustomCollision : public CCustomCollisionInterface
{
public:
    CCustomCollision();
    ~CCustomCollision();
    CColModelSAInterface* GetCurrentCollision(CObject* pObject, bool bSkipScaled = false);
    CColModelSAInterface* GetCurrentCollision(CEntitySAInterface* pEntitySA, bool bSkipScaled = false);
    void                  SetObjectScale(CObject* pObject, CVector scale);
    bool                  SetObjectCollision(CEntitySAInterface* pEntitySA, CColModelSAInterface* pColModelSA);
    bool                  SetObjectScaledCollision(CEntitySAInterface* pEntitySA, CColModelSAInterface* pColModelSA);
    void DrawCollisionDebug(CObject* pObject);

private:

    CColModelSAInterface*  GetOriginalColModel(CEntitySAInterface* pEntity);
    bool                  SaveOriginalCollision(WORD model);
    CColModelSAInterface* CopyColModel(CColModelSAInterface* pColModel);
    void                   ScaleCollision(CColModelSAInterface* pColModel, const CVector& scale);

    bool                   HasCustomCollision(CEntitySAInterface* pEntitySA);
    bool                   RemoveCustomCollision(CEntitySAInterface* pEntitySA);


    std::map<DWORD, CColModelSAInterface*>         m_mapOriginalCollisions; // model = coldata
    std::map<DWORD, CColModelSAInterface*>         m_mapCustomScaledCollisions; // object = coldata
    std::map<DWORD, CColModelSAInterface*>         m_mapCustomCollisions; // object = coldata

    // collision => scale = scaled collision
    std::map<CColModelSAInterface*, std::map<CVector, CColModelSAInterface*>> m_mapScaledCollisions =
        std::map<CColModelSAInterface*, std::map<CVector, CColModelSAInterface*>>();
};
