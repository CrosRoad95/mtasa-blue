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
    CColModelSAInterface* GetCustomCollision(CObject* pObject);
    void                  SetObjectScale(CEntitySAInterface* pEntitySA, CVector scale);
    bool                  SetObjectCollision(CEntitySAInterface* pEntitySA, CColModelSAInterface* pColModelSA);

private:
    CColModelSAInterface*  GetOriginalColModel(CEntitySAInterface* pEntity);
    void SaveOriginalCollisionIfDoesNotExists(WORD model, CColModelSAInterface* colModel);
    ScaledCollision* GetScaledCollision(WORD model, CVector* targetScale);
    CColModelSAInterface* GetScaled(WORD model, CColModelSAInterface* colModel, CVector* scale);
    CColModelSAInterface* GetScaledCollision(CObject* pObject);
    bool                  SaveOriginalCollision(WORD model);
    CColModelSAInterface* CopyColModel(CColModelSAInterface* pColModel);
    bool                  HasCustomCollision(CEntitySAInterface* pEntitySA);

    std::map<DWORD, CColModelSAInterface*>         m_mapOriginalCollisions; // model = coldata
    std::map<DWORD, CColModelSAInterface*>          m_mapCustomCollisions; // object = coldata
    std::map<WORD, std::vector<ScaledCollision*>> scaledCollisions = std::map<WORD, std::vector<ScaledCollision*>>();
    std::map<WORD, OriginalCollisions*>           originalCollisions = std::map<WORD, OriginalCollisions*>();
};
