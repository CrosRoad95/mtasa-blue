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
    CColSphereSA*        spheres;
    CColBoxSA*           boxes;
    CCompressedVectorSA* vertices;
    ushort               spheresCount;
    ushort               boxesCount;
    ushort               verticesCount;
    CBoundingBoxSA       boundingBox;
};

class ScaledCollision
{
public:
    CVector*             scale;
    CColSphereSA*        spheres;
    CColBoxSA*           boxes;
    CCompressedVectorSA* vertices;
    ushort               spheresCount;
    ushort               boxesCount;
    ushort               verticesCount;
    CBoundingBoxSA       boundingBox;
};

class CCustomCollision : public CCustomCollisionInterface
{
public:
    CCustomCollision();
    ~CCustomCollision();
    CColModel*            GetCurrentCollision(CObject* pObject, bool bSkipScaled = false);
    void                  SetObjectScale(CObject* pObject, CVector scale);
    bool                  SetObjectCollision(CObject* pObject, CColModel* pColModelSA);
    bool                  SetObjectScaledCollision(CObject* pObject, CColModel* pColModelSA);
    bool                  HasCustomCollision(CEntity* pEntity);
    bool                  RemoveCustomCollision(CEntity* pEntity);
    void                  DrawCollisionDebug(CObject* pObject);

private:
    CColModel*            GetOriginalColModel(CEntity* pEntity);
    bool                  SaveOriginalCollision(WORD model);
    CColModel*            CopyColModel(CColModel* pColModel);
    void                  ScaleCollision(CColModel* pColModel, const CVector& scale);

    std::map<DWORD, CColModel*>            m_mapOriginalCollisions;                // model = coldata
    std::map<DWORD, CColModel*>            m_mapCustomScaledCollisions;            // object = coldata
    std::map<DWORD, CColModel*>            m_mapCustomCollisions;                  // object = coldata

    // collision => scale = scaled collision
    std::map<CColModel*, std::map<CVector, CColModel*>> m_mapScaledCollisions = std::map<CColModel*, std::map<CVector, CColModel*>>();
};
